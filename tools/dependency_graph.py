#!/usr/bin/env python3

"""
Dependency Graph Generator for ECS Systems

What this does
- Parses C++ ECS systems (structs deriving from System<...>/PausableSystem<...>) in src/
- Infers per-system read/write sets for components, singletons, and globals
- Maps each system to a stage (fixed_update/update/render) via src/main.cpp
- Emits Graphviz DOT graphs and a JSON summary
- Optionally renders SVG and writes a simple HTML viewer

Quick start
- Generate graphs and JSON summary into build/:
  $ python3 tools/dependency_graph.py --src src --main src/main.cpp --outdir build

- Also render SVGs (requires Graphviz 'dot'):
  $ python3 tools/dependency_graph.py --outdir build  # dot is auto-detected

- Serve artifacts over HTTP and view in a browser:
  $ (cd build && python3 -m http.server 8080)
  Then open http://localhost:8080/ in a browser. If SVGs exist, open:
    - systems_components.svg
    - system_conflicts.svg
  Or run with --write-html to emit an index.html that links to these.

- If you do not have Graphviz, use --write-html to generate index.html using DOT files;
  you can paste DOT files into online viewers or install graphviz.

Config to refine analysis
- You can mark known read-only or write-only resources to refine conflicts with --config:
  $ python3 tools/dependency_graph.py --config tools/dep_config.json
- You can also add per-system overrides in the config under a "systems" section.

Example dep_config.json
{
  "components": {
    "Transform": "rw",              // rw | r | w
    "HasColor": "r"
  },
  "singletons": {
    "ShaderLibrary": "r",
    "SoundLibrary": "w"
  },
  "globals": {
    "mainRT": "w"
  },
  "systems": {
    "RenderSpritesWithShaders": {
      "singletons": { "ShaderLibrary": "r" }
    },
    "UpdateShaderValues": {
      "singletons": { "ShaderLibrary": "w" }
    }
  }
}

CI integration (suggested)
- Create a baseline once:
  $ python3 tools/dependency_graph.py --outdir build && cp build/dependency_summary.json tools/dependency_baseline.json
- In CI, regenerate and compare against the baseline to fail on changes.

"""

import argparse
import os
import re
import json
import shutil
import subprocess
from collections import defaultdict, namedtuple
from typing import List, Dict, Tuple, Set

# ------------------- Helpers -------------------

def read_text(path: str) -> str:
    with open(path, 'r', encoding='utf-8', errors='ignore') as f:
        return f.read()


def iter_sources(root: str) -> List[str]:
    files = []
    for dirpath, _, filenames in os.walk(root):
        for fn in filenames:
            if fn.endswith(('.h', '.hpp', '.cpp', '.cc', '.cxx')):
                files.append(os.path.join(dirpath, fn))
    files.sort()
    return files


def split_top_level_commas(s: str) -> List[str]:
    parts = []
    cur = []
    depth_lt = 0
    depth_paren = 0
    i = 0
    while i < len(s):
        ch = s[i]
        if ch == '<':
            depth_lt += 1
            cur.append(ch)
        elif ch == '>':
            depth_lt = max(0, depth_lt - 1)
            cur.append(ch)
        elif ch == '(':
            depth_paren += 1
            cur.append(ch)
        elif ch == ')':
            depth_paren = max(0, depth_paren - 1)
            cur.append(ch)
        elif ch == ',' and depth_lt == 0 and depth_paren == 0:
            parts.append(''.join(cur).strip())
            cur = []
        else:
            cur.append(ch)
        i += 1
    if cur:
        parts.append(''.join(cur).strip())
    return parts


def normalize_type(t: str) -> str:
    # Remove references, pointers, const, whitespace redundancies
    t = t.strip()
    t = re.sub(r'\bconst\b\s*', '', t)
    t = t.replace('&', '').replace('*', '')
    t = re.sub(r'\s+', ' ', t)
    return t.strip()


StructInfo = namedtuple('StructInfo', 'name base_decl base_type template_args body file_path')


def find_structs_with_system_bases(text: str, file_path: str) -> List[StructInfo]:
    # Pattern to capture: struct Name : Base<...> { ... };
    # We will iterate over all struct declarations and pick those with System-like bases
    results = []
    # Simplify by removing comments (roughly)
    text_wo_comments = re.sub(r'//.*', '', text)
    text_wo_comments = re.sub(r'/\*.*?\*/', '', text_wo_comments, flags=re.DOTALL)

    struct_iter = re.finditer(r'\bstruct\s+(?P<name>[A-Za-z_]\w*)\s*:\s*(?P<bases>[^\{;]+)\{', text_wo_comments)
    for m in struct_iter:
        name = m.group('name')
        bases = m.group('bases').strip()
        start_index = m.end() - 1  # points at '{'
        # Find matching closing brace for this struct
        body, end_index = extract_brace_block(text_wo_comments, start_index)
        if body is None:
            continue
        # Identify a base that looks like System<...> or PausableSystem<...> or SystemWithUIContext<...>
        base_type = None
        template_args = []
        base_decl = ''
        # Bases may be comma-separated; search for template invocation
        base_candidates = re.findall(r'([A-Za-z_:]+)\s*<\s*([^>]*)\s*>', bases)
        chosen = None
        for btype, bargs in base_candidates:
            simple = btype.split('::')[-1]
            if simple in ('System', 'PausableSystem', 'SystemWithUIContext'):
                chosen = (btype, bargs)
                break
        if chosen:
            base_type, argstr = chosen
            template_args = [normalize_type(arg) for arg in split_top_level_commas(argstr)] if argstr.strip() else []
            base_decl = f"{base_type}<{', '.join(template_args)}>"
            results.append(StructInfo(name=name, base_decl=base_decl, base_type=base_type.split('::')[-1], template_args=template_args, body=body, file_path=file_path))
    return results


def extract_brace_block(text: str, brace_index: int) -> Tuple[str, int]:
    if brace_index >= len(text) or text[brace_index] != '{':
        # Move forward to next '{'
        i = text.find('{', brace_index)
        if i == -1:
            return None, -1
        brace_index = i
    depth = 0
    i = brace_index
    while i < len(text):
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0:
                # include body between the outermost braces
                return text[brace_index + 1:i], i
        i += 1
    return None, -1


ParamInfo = namedtuple('ParamInfo', 'type_name is_const is_entity')


def parse_for_each_params(body: str) -> List[ParamInfo]:
    # Find all for_each_with(...) parameter lists in this struct body
    params: List[ParamInfo] = []
    for m in re.finditer(r'for_each_with(?:_derived)?\s*\((.*?)\)\s*(?:const\s*)?(?:override\b)?', body, flags=re.DOTALL):
        paramlist = m.group(1)
        # Split at top-level commas
        parts = split_top_level_commas(paramlist)
        for p in parts:
            p = p.strip()
            if not p:
                continue
            # Ignore float or unnamed dt-like params
            if re.search(r'\bfloat\b', p):
                continue
            # Expect patterns like: const Type &name OR Type &name OR const Type& OR Type&
            pm = re.match(r'(?P<const>const\s+)?(?P<type>[A-Za-z_][\w:<>]*)\s*&', p)
            if not pm:
                # Could be value params; skip
                continue
            is_const = bool(pm.group('const'))
            tname = normalize_type(pm.group('type'))
            is_entity = (tname.split('::')[-1] == 'Entity')
            params.append(ParamInfo(type_name=tname, is_const=is_const, is_entity=is_entity))
    return params


DynamicUsage = namedtuple('DynamicUsage', 'reads writes read_singletons write_singletons read_globals write_globals')


def analyze_dynamic_usage(body: str) -> DynamicUsage:
    reads: Set[str] = set()
    writes: Set[str] = set()
    read_singletons: Set[str] = set()
    write_singletons: Set[str] = set()
    read_globals: Set[str] = set()
    write_globals: Set[str] = set()

    # Component reads via queries, has<>, get<>, get_with_child<>
    for pattern in [r'whereHasComponent<\s*([^>]+)\s*>',
                    r'whereMissingComponent<\s*([^>]+)\s*>',
                    r'\.has<\s*([^>]+)\s*>\s*\(',
                    r'\.get_with_child<\s*([^>]+)\s*>',
                    r'\.get<\s*([^>]+)\s*>\s*\(']:
        for m in re.finditer(pattern, body):
            typ = normalize_type(m.group(1))
            if typ:
                reads.add(typ)

    # Component writes via add/remove
    for pattern in [r'\.addComponent(?:IfMissing)?<\s*([^>]+)\s*>',
                    r'\.removeComponent<\s*([^>]+)\s*>']:
        for m in re.finditer(pattern, body):
            typ = normalize_type(m.group(1))
            if typ:
                writes.add(typ)

    # Singletons via EntityHelper::get_singleton_cmp<...>
    for m in re.finditer(r'EntityHelper::get_singleton_cmp<\s*([^>]+)\s*>', body):
        typ = normalize_type(m.group(1))
        if typ:
            # Assume read by default
            read_singletons.add(typ)

    # RoundManager / GameStateManager usage heuristics
    rm_calls = re.findall(r'RoundManager::get\(\)\.(\w+)', body)
    for method in rm_calls:
        # Treat end_game and setters as writes
        if method in ('end_game', 'set_round', 'set_active_round_type', 'reset_round_time', 'reset_countdown'):
            write_singletons.add('RoundManager')
        else:
            read_singletons.add('RoundManager')

    gsm_calls = re.findall(r'GameStateManager::get\(\)\.(\w+)', body)
    for method in gsm_calls:
        if method.startswith('set_') or method in ('end_game', 'pause', 'resume', 'set_screen'):
            write_singletons.add('GameStateManager')
        else:
            read_singletons.add('GameStateManager')

    # ShaderLibrary / SoundLibrary / MusicLibrary
    if re.search(r'ShaderLibrary::get\(\)', body):
        # Update values likely writes internal state
        if re.search(r'update_values\s*\(', body):
            write_singletons.add('ShaderLibrary')
        read_singletons.add('ShaderLibrary')
    if re.search(r'SoundLibrary::get\(\)', body):
        write_singletons.add('SoundLibrary')
    if re.search(r'MusicLibrary::get\(\)', body):
        write_singletons.add('MusicLibrary')

    # MapManager / Settings / Map manipulation
    if re.search(r'MapManager::get\(\)', body):
        # Assume both
        read_singletons.add('MapManager')
    if re.search(r'Settings::get\(\)', body):
        read_singletons.add('Settings')

    # Global mainRT usage
    if re.search(r'\bmainRT\b', body):
        if re.search(r'\b(mainRT)\s*=|UnloadRenderTexture\s*\(\s*mainRT\s*\)', body):
            write_globals.add('mainRT')
        read_globals.add('mainRT')

    return DynamicUsage(reads=reads, writes=writes,
                        read_singletons=read_singletons, write_singletons=write_singletons,
                        read_globals=read_globals, write_globals=write_globals)


class SystemModel:
    def __init__(self, name: str, file_path: str, base_type: str, declared_components: List[str], body: str):
        self.name = name
        self.file_path = file_path
        self.base_type = base_type  # System, PausableSystem, etc.
        self.declared_components = declared_components[:]  # template args
        self.read_components: Set[str] = set()
        self.write_components: Set[str] = set()
        self.read_singletons: Set[str] = set()
        self.write_singletons: Set[str] = set()
        self.read_globals: Set[str] = set()
        self.write_globals: Set[str] = set()
        self.stage: str = 'unknown'
        # analyze body
        self._parse_body(body)

    def _parse_body(self, body: str):
        # Params
        params = parse_for_each_params(body)
        for p in params:
            if p.is_entity:
                # Entity non-const suggests possible writes to entity (component adds/removes)
                if not p.is_const:
                    self.write_singletons.add('EntityStorage')  # abstract write to ECS storage
                continue
            # Otherwise it's a component param
            if p.is_const:
                self.read_components.add(p.type_name)
            else:
                self.read_components.add(p.type_name)
                self.write_components.add(p.type_name)
        # Declared template args that don't show up in params (e.g., System<>)
        # Treat as read by default
        for t in self.declared_components:
            if t and t not in self.read_components and t not in self.write_components:
                if t != 'afterhours::ui::UIContext<InputAction>':
                    self.read_components.add(t)
        # Dynamic usage
        dyn = analyze_dynamic_usage(body)
        self.read_components.update(dyn.reads)
        self.write_components.update(dyn.writes)
        self.read_singletons.update(dyn.read_singletons)
        self.write_singletons.update(dyn.write_singletons)
        self.read_globals.update(dyn.read_globals)
        self.write_globals.update(dyn.write_globals)

    def resources_read(self) -> Set[str]:
        r = set(self.read_components)
        r.update(f"singleton:{s}" for s in self.read_singletons)
        r.update(f"global:{g}" for g in self.read_globals)
        return r

    def resources_written(self) -> Set[str]:
        w = set(self.write_components)
        w.update(f"singleton:{s}" for s in self.write_singletons)
        w.update(f"global:{g}" for g in self.write_globals)
        return w


# ------------------- Stage parsing -------------------

def parse_stages(main_text: str) -> Dict[str, str]:
    stage_by_system: Dict[str, str] = {}
    # Fixed update
    for m in re.finditer(r'register_fixed_update_system\s*\(\s*std::make_unique<\s*([A-Za-z_][\w:]*)\s*>\s*\(', main_text):
        stage_by_system[m.group(1).split('::')[-1]] = 'fixed_update'
    # Update
    for m in re.finditer(r'register_update_system\s*\(\s*std::make_unique<\s*([A-Za-z_][\w:]*)\s*>\s*\(', main_text):
        stage_by_system[m.group(1).split('::')[-1]] = 'update'
    # Render
    for m in re.finditer(r'register_render_system\s*\(\s*std::make_unique<\s*([A-Za-z_][\w:]*)\s*>\s*\(', main_text):
        stage_by_system[m.group(1).split('::')[-1]] = 'render'
    return stage_by_system


# ------------------- Conflict graph -------------------

def compute_conflicts(systems: List[SystemModel]) -> Dict[Tuple[str, str], Set[str]]:
    # Return map (A,B) -> reasons (resource names) for conflicts; symmetric pairs name-ordered
    conflicts: Dict[Tuple[str, str], Set[str]] = {}
    for i in range(len(systems)):
        for j in range(i + 1, len(systems)):
            a = systems[i]
            b = systems[j]
            # Only consider systems in same stage for parallelization
            if a.stage != b.stage:
                continue
            # Check resource conflicts
            reasons: Set[str] = set()
            # Components
            overlap = a.resources_written() & (b.resources_written() | b.resources_read())
            reasons.update(overlap)
            overlap = b.resources_written() & (a.resources_written() | a.resources_read())
            reasons.update(overlap)
            if reasons:
                key = (a.name, b.name) if a.name < b.name else (b.name, a.name)
                conflicts.setdefault(key, set()).update(reasons)
    return conflicts


def colorize_batches_per_stage(systems: List[SystemModel], conflicts: Dict[Tuple[str, str], Set[str]]) -> Dict[str, List[List[str]]]:
    # Greedy graph coloring per stage to produce parallel batches
    by_stage: Dict[str, List[SystemModel]] = defaultdict(list)
    for s in systems:
        by_stage[s.stage].append(s)

    batches_by_stage: Dict[str, List[List[str]]] = {}
    for stage, syss in by_stage.items():
        # Build adjacency
        syss_sorted = sorted(syss, key=lambda s: s.name)
        idx = {s.name: k for k, s in enumerate(syss_sorted)}
        adj = [set() for _ in syss_sorted]
        for (a, b), _ in conflicts.items():
            if a in idx and b in idx:
                ia, ib = idx[a], idx[b]
                adj[ia].add(ib)
                adj[ib].add(ia)
        # Order by degree desc, tie-break by name
        order = sorted(range(len(syss_sorted)), key=lambda i: (len(adj[i]), syss_sorted[i].name), reverse=True)
        color_of = [-1] * len(syss_sorted)
        for u in order:
            used = {color_of[v] for v in adj[u] if color_of[v] != -1}
            c = 0
            while c in used:
                c += 1
            color_of[u] = c
        max_color = max(color_of) if color_of else -1
        batches: List[List[str]] = [[] for _ in range(max_color + 1)]
        for i, c in enumerate(color_of):
            if c >= 0:
                batches[c].append(syss_sorted[i].name)
        batches_by_stage[stage] = [sorted(b) for b in batches if b]
    return batches_by_stage


# ------------------- DOT emitters -------------------

def emit_system_component_dot(systems: List[SystemModel], out_path: str):
    # Create DOT graph showing components and singletons as resources
    lines = []
    lines.append('digraph G {')
    lines.append('  rankdir=LR;')
    # Clusters per stage for system nodes
    stages = defaultdict(list)
    for s in systems:
        stages[s.stage].append(s)
    for stage, syss in stages.items():
        lines.append(f'  subgraph cluster_{stage} {{')
        lines.append(f'    label="{stage}";')
        for s in sorted(syss, key=lambda s: s.name):
            lines.append(f'    "{s.name}" [shape=box, style=filled, fillcolor="lightgray"];')
        lines.append('  }')
    # Resource nodes (components, singletons, globals)
    comp_nodes: Set[str] = set()
    sing_nodes: Set[str] = set()
    glob_nodes: Set[str] = set()
    for s in systems:
        comp_nodes.update(s.read_components)
        comp_nodes.update(s.write_components)
        sing_nodes.update(s.read_singletons)
        sing_nodes.update(s.write_singletons)
        glob_nodes.update(s.read_globals)
        glob_nodes.update(s.write_globals)
    for c in sorted(comp_nodes):
        lines.append(f'  "comp:{c}" [label="{c}", shape=ellipse, color=darkgreen];')
    for c in sorted(sing_nodes):
        lines.append(f'  "sing:{c}" [label="{c}", shape=hexagon, color=blue];')
    for c in sorted(glob_nodes):
        lines.append(f'  "glob:{c}" [label="{c}", shape=diamond, color=purple];')

    # Edges: reads (resource -> system), writes (system -> resource)
    for s in sorted(systems, key=lambda s: s.name):
        for r in sorted(s.read_components):
            lines.append(f'  "comp:{r}" -> "{s.name}" [color=darkgreen, label="R"];')
        for r in sorted(s.write_components):
            lines.append(f'  "{s.name}" -> "comp:{r}" [color=red, label="W"];')
        for r in sorted(s.read_singletons):
            lines.append(f'  "sing:{r}" -> "{s.name}" [color=darkgreen, label="R"];')
        for r in sorted(s.write_singletons):
            lines.append(f'  "{s.name}" -> "sing:{r}" [color=red, label="W"];')
        for r in sorted(s.read_globals):
            lines.append(f'  "glob:{r}" -> "{s.name}" [color=darkgreen, label="R"];')
        for r in sorted(s.write_globals):
            lines.append(f'  "{s.name}" -> "glob:{r}" [color=red, label="W"];')

    lines.append('}')
    with open(out_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))


def emit_system_conflict_dot(systems: List[SystemModel], conflicts: Dict[Tuple[str, str], Set[str]], out_path: str):
    lines = []
    lines.append('graph G {')
    lines.append('  rankdir=LR;')
    # Nodes grouped by stage
    stages = defaultdict(list)
    for s in systems:
        stages[s.stage].append(s)
    for stage, syss in stages.items():
        lines.append(f'  subgraph cluster_{stage} {{')
        lines.append(f'    label="{stage}";')
        for s in sorted(syss, key=lambda s: s.name):
            lines.append(f'    "{s.name}" [shape=box, style=filled, fillcolor="lightyellow"];')
        lines.append('  }')
    # Conflict edges within stages
    for (a, b), reasons in sorted(conflicts.items(), key=lambda kv: kv[0]):
        label = ','.join(sorted(reasons))
        lines.append(f'  "{a}" -- "{b}" [color=red, label="{label}"];')
    lines.append('}')
    with open(out_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))


def apply_config_constraints(systems: List[SystemModel], config: Dict):
    comp_cfg = {k: v for k, v in (config.get('components') or {}).items()}
    sing_cfg = {k: v for k, v in (config.get('singletons') or {}).items()}
    glob_cfg = {k: v for k, v in (config.get('globals') or {}).items()}

    for s in systems:
        # global type-level constraints
        for name, mode in comp_cfg.items():
            if mode == 'r':
                s.write_components.discard(name)
            elif mode == 'w':
                s.read_components.discard(name)
        for name, mode in sing_cfg.items():
            if mode == 'r':
                s.write_singletons.discard(name)
            elif mode == 'w':
                s.read_singletons.discard(name)
        for name, mode in glob_cfg.items():
            if mode == 'r':
                s.write_globals.discard(name)
            elif mode == 'w':
                s.read_globals.discard(name)

    # per-system overrides
    systems_cfg = config.get('systems') or {}
    for s in systems:
        sc = systems_cfg.get(s.name)
        if not sc:
            continue
        for name, mode in (sc.get('components') or {}).items():
            if mode == 'r':
                s.write_components.discard(name)
                s.read_components.add(name)
            elif mode == 'w':
                s.read_components.discard(name)
                s.write_components.add(name)
            elif mode == 'rw':
                s.read_components.add(name)
                s.write_components.add(name)
        for name, mode in (sc.get('singletons') or {}).items():
            if mode == 'r':
                s.write_singletons.discard(name)
                s.read_singletons.add(name)
            elif mode == 'w':
                s.read_singletons.discard(name)
                s.write_singletons.add(name)
            elif mode == 'rw':
                s.read_singletons.add(name)
                s.write_singletons.add(name)
        for name, mode in (sc.get('globals') or {}).items():
            if mode == 'r':
                s.write_globals.discard(name)
                s.read_globals.add(name)
            elif mode == 'w':
                s.read_globals.discard(name)
                s.write_globals.add(name)
            elif mode == 'rw':
                s.read_globals.add(name)
                s.write_globals.add(name)


def write_html_index(outdir: str):
    comp_svg_path = os.path.join(outdir, 'systems_components.svg')
    conf_svg_path = os.path.join(outdir, 'system_conflicts.svg')
    try:
        with open(comp_svg_path, 'r', encoding='utf-8', errors='ignore') as fsvg:
            comp_svg = fsvg.read()
    except Exception:
        comp_svg = '<p>systems_components.svg not found. Install graphviz and rerun generation.</p>'
    try:
        with open(conf_svg_path, 'r', encoding='utf-8', errors='ignore') as fsvg:
            conf_svg = fsvg.read()
    except Exception:
        conf_svg = '<p>system_conflicts.svg not found. Install graphviz and rerun generation.</p>'

    html = """<!doctype html>
<html lang=\"en\">
<meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">
<title>Dependency Graphs</title>
<style>body{font-family:sans-serif;margin:20px} .grid{display:grid;grid-template-columns:1fr;gap:20px} .controls{display:flex;gap:8px;align-items:center;margin:8px 0} .controls input{padding:6px 8px;border:1px solid #ccc;border-radius:4px;min-width:260px} .viewport{position:relative;overflow:hidden;border:1px solid #ddd;background:#fafafa;height:70vh} .svg-container{width:100%;height:100%;} .dimmed{opacity:0.2} .node{cursor:pointer}</style>
<h1>Dependency Graphs</h1>
<p>Drag to pan. Scroll to zoom.</p>
<div class=\"grid\">
  <div>
    <h2>Systems ↔ Resources</h2>
    <div class=\"controls\">
      <input id=\"search-comp\" type=\"text\" placeholder=\"Search system or resource (Enter to focus)\" />
    </div>
    <div class=\"viewport\">
      <div id=\"comp-container\" class=\"svg-container\">[[COMP_SVG]]</div>
    </div>
    <p>DOT: <a href=\"systems_components.dot\">systems_components.dot</a></p>
  </div>
  <div>
    <h2>System Conflicts</h2>
    <div class=\"controls\">
      <input id=\"search-conf\" type=\"text\" placeholder=\"Search system (Enter to focus)\" />
    </div>
    <div class=\"viewport\">
      <div id=\"conf-container\" class=\"svg-container\">[[CONF_SVG]]</div>
    </div>
    <p>DOT: <a href=\"system_conflicts.dot\">system_conflicts.dot</a></p>
  </div>
  
</div>
<script src=\"https://cdn.jsdelivr.net/npm/d3@7/dist/d3.min.js\"></script>
<script src=\"https://cdn.jsdelivr.net/npm/fuse.js@6.6.2/dist/fuse.min.js\"></script>
<script>
function initZoomD3(containerId, searchInputId){
  const container = document.getElementById(containerId);
  const svg = d3.select(container).select('svg');
  if(svg.empty()) return;
  svg.attr('width', null).attr('height', null).style('width','auto').style('height','auto');
  const svgNode = svg.node();
  const wrap = svg.append('g');
  const children = Array.from(svgNode.childNodes);
  for(const n of children){ if(n !== wrap.node()) wrap.node().appendChild(n); }
  let currentTransform = d3.zoomIdentity;
  const zoom = d3.zoom().scaleExtent([0.1,20]).on('zoom', (event)=>{ wrap.attr('transform', event.transform); currentTransform = event.transform; });
  svg.call(zoom);
  let focused = null;

  // Build adjacency from Graphviz output
  const nodeGroups = wrap.selectAll('g.node');
  const edgeGroups = wrap.selectAll('g.edge');
  const getNodeName = (g) => {
    const t = d3.select(g).select('title');
    return t.empty() ? null : t.text();
  };
  const adj = new Map();
  nodeGroups.each(function(){
    const n = getNodeName(this);
    if(n && !adj.has(n)) adj.set(n, new Set());
  });
  edgeGroups.each(function(){
    const title = d3.select(this).select('title').text() || '';
    const m = title.match(/(.+)->(.+)/) || title.match(/(.+)--(.+)/);
    if(m){
      const a = m[1].trim();
      const b = m[2].trim();
      if(!adj.has(a)) adj.set(a, new Set());
      if(!adj.has(b)) adj.set(b, new Set());
      adj.get(a).add(b);
      adj.get(b).add(a);
    }
  });

  const applyFilter = (visibleSet) => {
    nodeGroups.each(function(){
      const name = getNodeName(this);
      const show = name && visibleSet.has(name);
      d3.select(this).classed('dimmed', !show);
    });
    edgeGroups.each(function(){
      const title = d3.select(this).select('title').text() || '';
      const m = title.match(/(.+)->(.+)/) || title.match(/(.+)--(.+)/);
      let show = false;
      if(m && focused){
        const a = m[1].trim();
        const b = m[2].trim();
        show = (a === focused || b === focused);
      }
      d3.select(this).classed('dimmed', !show);
    });
  };

  const resetFilter = () => {
    nodeGroups.classed('dimmed', false);
    edgeGroups.classed('dimmed', false);
    focused = null;
  };

  // Delegate click handling so child elements inside nodes work reliably
  svg.on('click', (event) => {
    const target = event.target;
    if(!target) return;
    const nodeGroup = target.closest ? target.closest('g.node') : null;
    if(!nodeGroup) return;
    event.stopPropagation();
    const name = getNodeName(nodeGroup);
    if(!name) return;
    if (focused === name) {
      resetFilter();
    } else {
      focused = name;
      const neighbors = adj.get(name) || new Set();
      const keep = new Set([name, ...neighbors]);
      applyFilter(keep);
    }
  });

  svg.on('dblclick', () => resetFilter());
  window.addEventListener('keydown', (e) => { if(e.key === 'Escape') resetFilter(); });
  // Search
  const nodeIndex = [];
  nodeGroups.each(function(){ const n = getNodeName(this); if(n) nodeIndex.push({ name: n }); });
  const fuse = new Fuse(nodeIndex, { keys: ['name'], threshold: 0.3, includeScore: true });
  const input = document.getElementById(searchInputId);
  if(input){
    input.addEventListener('keydown', (e) => {
      if(e.key !== 'Enter') return;
      const q = input.value.trim();
      if(!q){ resetFilter(); return; }
      const res = fuse.search(q, { limit: 1 });
      if(!res.length) return;
      const name = res[0].item.name;
      focused = name;
      const neighbors = adj.get(name) || new Set();
      const keep = new Set([name, ...neighbors]);
      applyFilter(keep);
      const ng = Array.from(nodeGroups.nodes()).find(g => getNodeName(g) === name);
      if(ng){
        const bbox = ng.getBBox();
        const vp = container.getBoundingClientRect();
        const k = currentTransform.k || 1;
        const cx = bbox.x + bbox.width/2; const cy = bbox.y + bbox.height/2;
        const tx = vp.width/2 - cx * k; const ty = vp.height/2 - cy * k;
        svg.transition().duration(220).call(zoom.transform, d3.zoomIdentity.translate(tx, ty).scale(k));
      }
    });
  }
}
initZoomD3('comp-container','search-comp');
initZoomD3('conf-container','search-conf');
</script>
</html>
"""
    html = html.replace('[[COMP_SVG]]', comp_svg)
    html = html.replace('[[CONF_SVG]]', conf_svg)
    with open(os.path.join(outdir, 'index.html'), 'w', encoding='utf-8') as f:
        f.write(html)


def print_read_write_summary(systems: List[SystemModel]):
    print('Per-system read/write resources:')
    for s in sorted(systems, key=lambda s: (s.stage, s.name)):
        reads = sorted(list(s.resources_read()))
        writes = sorted(list(s.resources_written()))
        print(f'- [{s.stage}] {s.name}')
        if reads:
            print(f'    reads:  {", ".join(reads)}')
        if writes:
            print(f'    writes: {", ".join(writes)}')


# ------------------- Main -------------------

def main():
    parser = argparse.ArgumentParser(description='Analyze ECS system/component dependencies and emit graphs.')
    parser.add_argument('--src', default='src', help='Source root directory (default: src)')
    parser.add_argument('--main', default='src/main.cpp', help='Path to main.cpp (default: src/main.cpp)')
    parser.add_argument('--outdir', default='build', help='Output directory (default: build)')
    parser.add_argument('--no-svg', action='store_true', help='Do not attempt to render SVG via dot')
    parser.add_argument('--config', default=None, help='Optional JSON config file to constrain resource modes (r/w/rw)')
    parser.add_argument('--write-html', action='store_true', help='Emit a simple index.html in outdir')
    parser.add_argument('--print-rw', action='store_true', help='Print a concise per-system read/write summary')
    args = parser.parse_args()

    src_root = os.path.abspath(args.src)
    main_path = os.path.abspath(args.main)
    outdir = os.path.abspath(args.outdir)
    os.makedirs(outdir, exist_ok=True)

    main_text = read_text(main_path) if os.path.exists(main_path) else ''
    stage_by_system = parse_stages(main_text)

    systems: List[SystemModel] = []

    for fp in iter_sources(src_root):
        try:
            text = read_text(fp)
        except Exception:
            continue
        structs = find_structs_with_system_bases(text, fp)
        for st in structs:
            sys = SystemModel(name=st.name, file_path=st.file_path, base_type=st.base_type,
                              declared_components=st.template_args, body=st.body)
            # Assign stage if known
            sys.stage = stage_by_system.get(sys.name, 'unknown')
            systems.append(sys)

    # Apply optional config constraints
    if args.config and os.path.exists(args.config):
        try:
            with open(args.config, 'r', encoding='utf-8') as f:
                cfg = json.load(f)
            apply_config_constraints(systems, cfg)
        except Exception as e:
            print(f"Warning: failed to load config {args.config}: {e}")

    # Compute conflicts and batches
    conflicts = compute_conflicts(systems)
    batches_by_stage = colorize_batches_per_stage(systems, conflicts)

    # Emit DOTs
    sys_comp_path = os.path.join(outdir, 'systems_components.dot')
    sys_conf_path = os.path.join(outdir, 'system_conflicts.dot')
    emit_system_component_dot(systems, sys_comp_path)
    emit_system_conflict_dot(systems, conflicts, sys_conf_path)

    # Also write JSON summary
    summary = []
    for s in sorted(systems, key=lambda s: s.name):
        summary.append({
            'name': s.name,
            'file': os.path.relpath(s.file_path, start=src_root),
            'stage': s.stage,
            'declared_components': sorted(s.declared_components),
            'reads': sorted(s.read_components),
            'writes': sorted(s.write_components),
            'read_singletons': sorted(s.read_singletons),
            'write_singletons': sorted(s.write_singletons),
            'read_globals': sorted(s.read_globals),
            'write_globals': sorted(s.write_globals),
        })
    with open(os.path.join(outdir, 'dependency_summary.json'), 'w', encoding='utf-8') as f:
        json.dump({'systems': summary, 'batches_by_stage': batches_by_stage}, f, indent=2)

    # Try to render SVGs
    if not args.no_svg and shutil.which('dot'):
        for dot_file in [sys_comp_path, sys_conf_path]:
            svg_out = dot_file.replace('.dot', '.svg')
            try:
                subprocess.run(['dot', '-Tsvg', dot_file, '-o', svg_out], check=True)
            except subprocess.CalledProcessError:
                pass

    if args.write_html:
        write_html_index(outdir)

    # Print batch summary
    print('Safe parallel batches per stage (heuristic):')
    for stage, batches in batches_by_stage.items():
        print(f'  {stage}:')
        for i, batch in enumerate(batches, 1):
            print(f'    batch {i}: {", ".join(batch)}')

    if args.print_rw:
        print_read_write_summary(systems)


if __name__ == '__main__':
    main()