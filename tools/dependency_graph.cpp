#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../vendor/nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

struct SystemModel {
  std::string name;
  std::string file_path;
  std::string base_type;
  std::vector<std::string> declared_components;
  std::unordered_set<std::string> read_components;
  std::unordered_set<std::string> write_components;
  std::string stage{"unknown"};
  int order{-1};
};

static std::string read_text_file(const fs::path &p) {
  std::ifstream f(p, std::ios::in | std::ios::binary);
  if (!f)
    return {};
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static std::string strip_comments(std::string s) {
  // remove // ... and /* ... */
  s = std::regex_replace(s, std::regex(R"(//.*)"), "");
  s = std::regex_replace(s, std::regex(R"(/\*.*?\*/)"), "");
  return s;
}

static std::string normalize_type(std::string t) {
  // Remove references, pointers, const, redundant spaces
  std::string out;
  out.reserve(t.size());
  for (char c : t) {
    if (c == '&' || c == '*')
      continue;
    out.push_back(c);
  }
  out = std::regex_replace(out, std::regex("\\bconst\\s*"), "");
  out = std::regex_replace(out, std::regex("\\s+"), " ");
  // trim
  while (!out.empty() && isspace(out.front()))
    out.erase(out.begin());
  while (!out.empty() && isspace(out.back()))
    out.pop_back();
  return out;
}

static std::vector<std::string> split_top_level_commas(const std::string &s) {
  std::vector<std::string> parts;
  std::string cur;
  int depth_lt = 0, depth_paren = 0;
  for (size_t i = 0; i < s.size(); ++i) {
    char ch = s[i];
    if (ch == '<')
      depth_lt++;
    else if (ch == '>')
      depth_lt = std::max(0, depth_lt - 1);
    else if (ch == '(')
      depth_paren++;
    else if (ch == ')')
      depth_paren = std::max(0, depth_paren - 1);
    if (ch == ',' && depth_lt == 0 && depth_paren == 0) {
      // flush
      std::string trimmed = cur;
      // trim
      while (!trimmed.empty() && isspace(trimmed.front()))
        trimmed.erase(trimmed.begin());
      while (!trimmed.empty() && isspace(trimmed.back()))
        trimmed.pop_back();
      if (!trimmed.empty())
        parts.push_back(trimmed);
      cur.clear();
    } else {
      cur.push_back(ch);
    }
  }
  if (!cur.empty()) {
    std::string trimmed = cur;
    while (!trimmed.empty() && isspace(trimmed.front()))
      trimmed.erase(trimmed.begin());
    while (!trimmed.empty() && isspace(trimmed.back()))
      trimmed.pop_back();
    if (!trimmed.empty())
      parts.push_back(trimmed);
  }
  return parts;
}

static std::optional<std::pair<size_t, size_t>>
find_brace_block(const std::string &s, size_t start_brace) {
  if (start_brace >= s.size())
    return std::nullopt;
  if (s[start_brace] != '{') {
    size_t i = s.find('{', start_brace);
    if (i == std::string::npos)
      return std::nullopt;
    start_brace = i;
  }
  int depth = 0;
  for (size_t i = start_brace; i < s.size(); ++i) {
    if (s[i] == '{')
      depth++;
    else if (s[i] == '}') {
      depth--;
      if (depth == 0)
        return std::make_pair(start_brace + 1, i);
    }
  }
  return std::nullopt;
}

struct StructInfo {
  std::string name, base_decl, base_type, body;
  std::vector<std::string> template_args;
  std::string file_path;
};

static std::vector<StructInfo>
find_structs_with_system_bases(const std::string &text_in,
                               const std::string &file_path) {
  std::vector<StructInfo> out;
  std::string text = strip_comments(text_in);
  std::regex struct_re(R"(\bstruct\s+([A-Za-z_]\w*)\s*:\s*([^\{;]+)\{)");
  for (auto it = std::sregex_iterator(text.begin(), text.end(), struct_re);
       it != std::sregex_iterator(); ++it) {
    std::smatch m = *it;
    std::string name = m[1];
    std::string bases = m[2];
    size_t body_begin = m.position(0) + m.length(0) - 1; // points at '{'
    auto blk = find_brace_block(text, body_begin);
    if (!blk)
      continue;
    std::string body = text.substr(blk->first, blk->second - blk->first);
    // find template base like Something<...>
    std::regex templ_re(R"(([A-Za-z_:]+)\s*<\s*([^>]*)\s*>)");
    std::smatch tm;
    std::string base_type;
    std::string base_decl;
    std::vector<std::string> template_args;
    bool found = false;
    for (auto tit = std::sregex_iterator(bases.begin(), bases.end(), templ_re);
         tit != std::sregex_iterator(); ++tit) {
      std::smatch mm = *tit;
      std::string btype = mm[1];
      std::string simple = btype;
      size_t pos = simple.rfind("::");
      if (pos != std::string::npos)
        simple = simple.substr(pos + 1);
      if (simple == "System" || simple == "PausableSystem" ||
          simple == "SystemWithUIContext") {
        base_type = simple;
        std::string argstr = mm[2];
        auto args = split_top_level_commas(argstr);
        for (auto &a : args)
          template_args.push_back(normalize_type(a));
        base_decl = btype + "<" + mm[2].str() + ">";
        found = true;
        break;
      }
    }
    if (!found)
      continue;
    out.push_back(
        StructInfo{name, base_decl, base_type, body, template_args, file_path});
  }
  return out;
}

static void parse_for_each_params(const std::string &body, SystemModel &sys) {
  std::regex fe_re(
      R"(for_each_with(?:_derived)?\s*\((.*?)\)\s*(?:const\s*)?(?:override\b)?)");
  for (auto it = std::sregex_iterator(body.begin(), body.end(), fe_re);
       it != std::sregex_iterator(); ++it) {
    std::smatch m = *it;
    std::string params = m[1];
    auto parts = split_top_level_commas(params);
    for (auto &p : parts) {
      if (p.find("float") != std::string::npos)
        continue;
      std::regex pm_re(R"((const\s+)?([A-Za-z_][\w:<>]*)\s*&)");
      std::smatch pm;
      if (!std::regex_search(p, pm, pm_re))
        continue;
      bool is_const = pm[1].matched;
      std::string tname = normalize_type(pm[2]);
      std::string simple = tname;
      size_t pos = simple.rfind("::");
      if (pos != std::string::npos)
        simple = simple.substr(pos + 1);
      bool is_entity = (simple == "Entity");
      if (is_entity)
        continue;
      if (is_const)
        sys.read_components.insert(tname);
      else {
        sys.read_components.insert(tname);
        sys.write_components.insert(tname);
      }
    }
  }
}

static void analyze_dynamic_component_usage(const std::string &body, SystemModel &sys) {
  auto add_read = [&](const std::string &raw) {
    std::string t = normalize_type(raw);
    if (!t.empty()) sys.read_components.insert(t);
  };
  auto add_write = [&](const std::string &raw) {
    std::string t = normalize_type(raw);
    if (!t.empty()) sys.write_components.insert(t);
  };

  // Reads via queries and getters
  {
    std::regex re(R"(whereHasComponent<\s*([^>]+)\s*>)");
    for (auto it = std::sregex_iterator(body.begin(), body.end(), re); it != std::sregex_iterator(); ++it) add_read((*it)[1]);
  }
  {
    std::regex re(R"(whereMissingComponent<\s*([^>]+)\s*>)");
    for (auto it = std::sregex_iterator(body.begin(), body.end(), re); it != std::sregex_iterator(); ++it) add_read((*it)[1]);
  }
  {
    std::regex re(R"(\.has<\s*([^>]+)\s*>\s*\()");
    for (auto it = std::sregex_iterator(body.begin(), body.end(), re); it != std::sregex_iterator(); ++it) add_read((*it)[1]);
  }
  {
    std::regex re(R"(\.get_with_child<\s*([^>]+)\s*>)");
    for (auto it = std::sregex_iterator(body.begin(), body.end(), re); it != std::sregex_iterator(); ++it) add_read((*it)[1]);
  }
  {
    std::regex re(R"(\.get<\s*([^>]+)\s*>\s*\()");
    for (auto it = std::sregex_iterator(body.begin(), body.end(), re); it != std::sregex_iterator(); ++it) add_read((*it)[1]);
  }

  // Writes via add/remove
  {
    std::regex re(R"(\.addComponent(?:IfMissing)?<\s*([^>]+)\s*>)");
    for (auto it = std::sregex_iterator(body.begin(), body.end(), re); it != std::sregex_iterator(); ++it) add_write((*it)[1]);
  }
  {
    std::regex re(R"(\.removeComponent<\s*([^>]+)\s*>)");
    for (auto it = std::sregex_iterator(body.begin(), body.end(), re); it != std::sregex_iterator(); ++it) add_write((*it)[1]);
  }
}

static std::pair<std::unordered_map<std::string, std::string>,
                 std::unordered_map<std::string, int>>
parse_stage_orders(const std::string &text) {
  std::unordered_map<std::string, std::string> stage;
  std::unordered_map<std::string, int> order;
  struct Item {
    size_t pos;
    std::string stage;
    std::string token;
  };
  std::vector<Item> items;
  std::regex re_fixed(
      R"(register_fixed_update_system\s*\(\s*std::make_unique<\s*([A-Za-z_][\w:<>]*)\s*>\s*\()");
  std::regex re_update(
      R"(register_update_system\s*\(\s*std::make_unique<\s*([A-Za-z_][\w:<>]*)\s*>\s*\()");
  std::regex re_render(
      R"(register_render_system\s*\(\s*std::make_unique<\s*([A-Za-z_][\w:<>]*)\s*>\s*\()");
  for (auto it = std::sregex_iterator(text.begin(), text.end(), re_fixed);
       it != std::sregex_iterator(); ++it)
    items.push_back(
        {static_cast<size_t>((*it).position()), "fixed_update", (*it)[1]});
  for (auto it = std::sregex_iterator(text.begin(), text.end(), re_update);
       it != std::sregex_iterator(); ++it)
    items.push_back(
        {static_cast<size_t>((*it).position()), "update", (*it)[1]});
  for (auto it = std::sregex_iterator(text.begin(), text.end(), re_render);
       it != std::sregex_iterator(); ++it)
    items.push_back(
        {static_cast<size_t>((*it).position()), "render", (*it)[1]});
  std::sort(items.begin(), items.end(),
            [](const Item &a, const Item &b) { return a.pos < b.pos; });
  std::unordered_map<std::string, int> counters{
      {"fixed_update", 0}, {"update", 0}, {"render", 0}};
  for (auto &it : items) {
    std::string tok = it.token;
    size_t p = tok.rfind("::");
    if (p != std::string::npos)
      tok = tok.substr(p + 1);
    size_t lt = tok.find('<');
    if (lt != std::string::npos)
      tok = tok.substr(0, lt);
    if (!stage.count(tok))
      stage[tok] = it.stage;
    if (!order.count(tok))
      order[tok] = counters[it.stage]++;
  }
  return {stage, order};
}

static void write_file(const fs::path &p, const std::string &content) {
  std::filesystem::create_directories(p.parent_path());
  std::ofstream f(p, std::ios::out | std::ios::binary);
  f << content;
}

static std::string systems_html_template() {
  // Reuse the HTML from Python generator; [[SUMMARY_JSON]] placeholder to
  // replace
  return R"HTML(<!doctype html>
<html lang="en">
<meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>System Subscriptions</title>
<style>
  body{font-family:sans-serif;margin:20px}
  .layout{display:grid;grid-template-columns:300px 1fr;gap:16px;align-items:start}
  .panel{border:1px solid #ddd;border-radius:6px;background:#fafafa}
  .panel h2{margin:0;padding:10px 12px;border-bottom:1px solid #ddd;font-size:16px}
  .panel .content{padding:10px 12px}
  .sections{display:grid;grid-template-rows:repeat(4,minmax(0,1fr));gap:10px;height:70vh}
  .section{display:flex;flex-direction:column;min-height:0}
  .list{flex:1;overflow:auto}
  .system{padding:6px 8px;border-radius:4px;cursor:pointer}
  .system:hover{background:#eee}
  .system.active{background:#dfefff}
  .badge{display:inline-block;font-size:11px;padding:2px 6px;border-radius:10px;background:#eaeaea;margin-left:6px}
  .controls{display:flex;gap:8px}
  .controls input{flex:1;padding:6px 8px;border:1px solid #ccc;border-radius:4px}
  .graph-wrap{height:70vh}
  .graph{width:100%;height:100%;border:1px solid #ddd;background:#fff}
</style>
<h1>System Subscriptions</h1>
<div class="layout">
  <div class="panel">
    <h2>Systems</h2>
    <div class="content">
      <div class="controls"> 
        <input id="search-sys" type="text" placeholder="Filter systems" />
      </div>
      <div class="sections"> 
        <div class="section"> 
          <div class="badge">fixed_update</div>
          <div id="systems-fixed_update" class="list"></div>
        </div>
        <div class="section"> 
          <div class="badge">update</div>
          <div id="systems-update" class="list"></div>
        </div>
        <div class="section"> 
          <div class="badge">render</div>
          <div id="systems-render" class="list"></div>
        </div>
        <div class="section"> 
          <div class="badge">unknown</div>
          <div id="systems-unknown" class="list"></div>
        </div>
      </div>
    </div>
  </div>
  <div class="panel">
    <h2 id="detail-title">Details</h2>
    <div class="content"> 
      <div class="graph-wrap">
        <svg id="graph" class="graph"></svg>
      </div>
    </div>
  </div>
</div>
<script src="https://cdn.jsdelivr.net/npm/d3@7/dist/d3.min.js"></script>
<script>
const summary = [[SUMMARY_JSON]];
const systems = summary.systems.map(s => ({
  name: s.name, stage: s.stage, order: s.order,
  declared: s.declared_components,
  reads: new Set(s.reads), writes: new Set(s.writes)
}));
const byStage = systems.reduce((a,s)=>{(a[s.stage] ||= []).push(s);return a;},{});
for(const st of Object.keys(byStage)) byStage[st].sort((a,b)=>a.order-b.order);
const containers = { 'fixed_update':document.getElementById('systems-fixed_update'), 'update':document.getElementById('systems-update'), 'render':document.getElementById('systems-render'), 'unknown':document.getElementById('systems-unknown') };
const detailTitle = document.getElementById('detail-title');
const svg = d3.select('#graph'); const g = svg.append('g'); const defs = svg.append('defs');
defs.append('marker').attr('id','arrow-black').attr('viewBox','0 0 10 10').attr('refX',10).attr('refY',5).attr('markerWidth',6).attr('markerHeight',6).attr('orient','auto-start-reverse').append('path').attr('d','M 0 0 L 10 5 L 0 10 z').attr('fill','#000');
defs.append('marker').attr('id','arrow-red').attr('viewBox','0 0 10 10').attr('refX',10).attr('refY',5).attr('markerWidth',6).attr('markerHeight',6).attr('orient','auto-start-reverse').append('path').attr('d','M 0 0 L 10 5 L 0 10 z').attr('fill','#c00');
function renderSystemList(filter=''){
  const stages = ['fixed_update','update','render','unknown'];
  const filtered = systems.filter(s => s.name.toLowerCase().includes(filter));
  for(const stage of stages){
    const parent = containers[stage]; if(!parent) continue; parent.innerHTML='';
    filtered.filter(s => s.stage===stage).sort((a,b)=>a.order-b.order).forEach(s=>{
      const div = document.createElement('div'); div.className='system'; div.dataset.name=s.name; div.textContent = `${s.order>=0?String(s.order).padStart(2,'0')+': ':''}${s.name}`; div.addEventListener('click',()=>selectSystem(s.name)); parent.appendChild(div);
    });
  }
}
function selectSystem(name){
  const s = systems.find(x=>x.name===name); if(!s) return; detailTitle.textContent = `Details — ${s.name}`;
  drawSystemGraph(s);
  document.querySelectorAll('.system').forEach(el=> el.classList.toggle('active', el.dataset.name===name));
}
function drawSystemGraph(s){
  const width = svg.node().clientWidth || 800; const height = svg.node().clientHeight || 500; svg.attr('viewBox',`0 0 ${width} ${height}`); g.selectAll('*').remove();
  const margin=40, sysW=180, sysH=70, centerX=width/2, centerY=height/2, leftX=margin+100, rightX=width-margin-100, bottomY=height-margin-60, vGap=48, hGap=180, nodeR=16;
  const all = new Set([...s.reads, ...s.writes]);
  const reads=[...all].filter(c=>s.reads.has(c) && !s.writes.has(c)).sort();
  const writes=[...all].filter(c=>s.writes.has(c) && !s.reads.has(c)).sort();
  const both=[...all].filter(c=>s.reads.has(c) && s.writes.has(c)).sort();
  const nodes=[]; nodes.push({id:s.name,type:'system',x:centerX,y:centerY,tx:centerX,ty:centerY});
  const leftStartY=centerY-((reads.length-1)*vGap)/2; reads.forEach((c,i)=>nodes.push({id:c,type:'read',x:leftX,y:leftStartY+i*vGap,tx:leftX,ty:leftStartY+i*vGap}));
  const rightStartY=centerY-((writes.length-1)*vGap)/2; writes.forEach((c,i)=>nodes.push({id:c,type:'write',x:rightX,y:rightStartY+i*vGap,tx:rightX,ty:rightStartY+i*vGap}));
  const bottomStartX=centerX-((both.length-1)*hGap)/2; both.forEach((c,i)=>nodes.push({id:c,type:'both',x:bottomStartX+i*hGap,y:bottomY,tx:bottomStartX+i*hGap,ty:bottomY}));
  const nodeById=new Map(nodes.map(n=>[n.id,n]));
  g.append('rect').attr('x',centerX-sysW/2).attr('y',centerY-sysH/2).attr('width',sysW).attr('height',sysH).attr('rx',8).attr('ry',8).attr('fill','#eee').attr('stroke','#999');
  g.append('text').attr('x',centerX).attr('y',centerY).attr('text-anchor','middle').attr('dominant-baseline','middle').attr('font-size',14).text(s.name);
  const links=[]; reads.forEach(c=>links.push({from:c,to:s.name,color:'#000',dir:'in'})); writes.forEach(c=>links.push({from:s.name,to:c,color:'#c00',dir:'out'})); both.forEach(c=>{links.push({from:c,to:s.name,color:'#000',dir:'in'});links.push({from:s.name,to:c,color:'#c00',dir:'out'});});
  const linkSel=g.selectAll('path.link').data(links).enter().append('path').attr('class','link').attr('fill','none').attr('stroke',d=>d.color).attr('stroke-width',2).attr('stroke-linecap','round').attr('marker-end',d=>d.color==='#c00'?'url(#arrow-red)':'url(#arrow-black)');
  const compNodes=nodes.filter(n=>n.type!=='system'); const nodeSel=g.selectAll('g.node').data(compNodes, d=>d.id).enter().append('g').attr('class','node');
  nodeSel.append('circle').attr('r',16).attr('fill','#f7f7f7').attr('stroke',d=>d.type==='write'?'#c00':(d.type==='read'?'#000':'#555'));
  nodeSel.append('text').attr('text-anchor','middle').attr('font-size',12);
  function edgePoints(l){ const a=nodeById.get(l.from), b=nodeById.get(l.to); if(!a||!b) return null; let x1=a.x,y1=a.y,x2=b.x,y2=b.y; if(l.dir==='in'){x2=centerX-sysW/2;y2=centerY;} if(l.dir==='out'){x1=centerX+sysW/2;y1=centerY;} return {x1,y1,x2,y2}; }
  function updateLinks(){ linkSel.attr('d', d=>{ const p=edgePoints(d); if(!p) return ''; const dx=p.x2-p.x1, dy=p.y2-p.y1, midx=p.x1+dx*0.5; let curv=Math.min(80.0, Math.abs(dx)/3.0); if(d.color==='#000') curv=-curv; const c1y=p.y1+dy*0.25+curv; const c2y=p.y2-dy*0.25+curv; return `M ${p.x1} ${p.y1} C ${midx} ${c1y}, ${midx} ${c2y}, ${p.x2} ${p.y2}`; }); }
  function updateNodes(){ nodeSel.attr('transform', d=>`translate(${d.x},${d.y})`); nodeSel.selectAll('text').attr('y', d=> d.type==='both' ? (16+14) : (-16-10)).text(d=>d.id); }
  updateNodes(); updateLinks();
}
document.getElementById('search-sys').addEventListener('input', e=>{ renderSystemList(e.target.value.trim().toLowerCase()); });
renderSystemList(''); const first = systems.slice().sort((a,b)=> (a.stage.localeCompare(b.stage)) || (a.order-b.order))[0]; if(first) selectSystem(first.name);
</script>
</html>)HTML";
}

int main(int argc, char **argv) {
  fs::path src = "src";
  fs::path main_cpp = "src/main.cpp";
  fs::path outdir = "output";
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    auto next = [&](fs::path &p) {
      if (i + 1 < argc) {
        p = argv[++i];
      }
    };
    if (a == "--src")
      next(src);
    else if (a == "--main")
      next(main_cpp);
    else if (a == "--outdir")
      next(outdir);
  }

  std::unordered_map<std::string, std::string> stage;
  std::unordered_map<std::string, int> order;
  {
    std::string main_txt = read_text_file(main_cpp);
    auto pr = parse_stage_orders(main_txt);
    stage = std::move(pr.first);
    order = std::move(pr.second);
  }

  std::vector<SystemModel> systems;
  for (auto const &entry : fs::recursive_directory_iterator(src)) {
    if (!entry.is_regular_file())
      continue;
    const fs::path &p = entry.path();
    std::string ext = p.extension().string();
    if (ext != ".h" && ext != ".hpp" && ext != ".cpp" && ext != ".cc" &&
        ext != ".cxx")
      continue;
    std::string text = read_text_file(p);
    auto structs = find_structs_with_system_bases(text, p.string());
    for (auto &st : structs) {
      SystemModel sm;
      sm.name = st.name;
      sm.file_path = st.file_path;
      sm.base_type = st.base_type;
      sm.declared_components = st.template_args;
      parse_for_each_params(st.body, sm);
      analyze_dynamic_component_usage(st.body, sm);
      for (auto &t : sm.declared_components) {
        if (!t.empty() && sm.read_components.count(t) == 0 &&
            sm.write_components.count(t) == 0) {
          if (t != "afterhours::ui::UIContext<InputAction>")
            sm.read_components.insert(t);
        }
      }
      auto itS = stage.find(sm.name);
      if (itS != stage.end())
        sm.stage = itS->second;
      else
        sm.stage = "unknown";
      auto itO = order.find(sm.name);
      sm.order = (itO != order.end() ? itO->second : -1);
      systems.push_back(std::move(sm));
    }
  }

  // Build summary JSON
  json summary;
  summary["systems"] = json::array();
  for (auto &s : systems) {
    json j;
    j["name"] = s.name;
    j["file"] = fs::relative(s.file_path, src).string();
    j["stage"] = s.stage;
    j["order"] = s.order;
    std::vector<std::string> decl = s.declared_components;
    std::sort(decl.begin(), decl.end());
    j["declared_components"] = decl;
    std::vector<std::string> reads(s.read_components.begin(),
                                   s.read_components.end());
    std::sort(reads.begin(), reads.end());
    j["reads"] = reads;
    std::vector<std::string> writes(s.write_components.begin(),
                                    s.write_components.end());
    std::sort(writes.begin(), writes.end());
    j["writes"] = writes;
    summary["systems"].push_back(std::move(j));
  }
  fs::create_directories(outdir);
  write_file(outdir / "dependency_summary.json", summary.dump(2));

  // Write systems.html
  std::string html = systems_html_template();
  std::string placeholder = "[[SUMMARY_JSON]]";
  std::string json_str = summary.dump();
  if (auto pos = html.find(placeholder); pos != std::string::npos) {
    html.replace(pos, placeholder.size(), json_str);
  }
  write_file(outdir / "systems.html", html);

  std::cout << "Wrote: " << (outdir / "dependency_summary.json") << " and "
            << (outdir / "systems.html") << "\n";
  return 0;
}
