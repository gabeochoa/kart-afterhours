#!/usr/bin/env python3

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
        idx = {s.name: k for k, s in enumerate(syss)}
        adj = [set() for _ in syss]
        for (a, b), _ in conflicts.items():
            if a in idx and b in idx:
                ia, ib = idx[a], idx[b]
                adj[ia].add(ib)
                adj[ib].add(ia)
        # Order by degree desc
        order = sorted(range(len(syss)), key=lambda i: len(adj[i]), reverse=True)
        color_of = [-1] * len(syss)
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
                batches[c].append(syss[i].name)
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
        for s in syss:
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
    for s in systems:
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
        for s in syss:
            lines.append(f'    "{s.name}" [shape=box, style=filled, fillcolor="lightyellow"];')
        lines.append('  }')
    # Conflict edges within stages
    for (a, b), reasons in conflicts.items():
        label = ','.join(sorted(reasons))
        lines.append(f'  "{a}" -- "{b}" [color=red, label="{label}"];')
    lines.append('}')
    with open(out_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))


# ------------------- Main -------------------

def main():
    parser = argparse.ArgumentParser(description='Analyze ECS system/component dependencies and emit graphs.')
    parser.add_argument('--src', default='src', help='Source root directory (default: src)')
    parser.add_argument('--main', default='src/main.cpp', help='Path to main.cpp (default: src/main.cpp)')
    parser.add_argument('--outdir', default='build', help='Output directory (default: build)')
    parser.add_argument('--no-svg', action='store_true', help='Do not attempt to render SVG via dot')
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
    for s in systems:
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

    # Print batch summary
    print('Safe parallel batches per stage (heuristic):')
    for stage, batches in batches_by_stage.items():
        print(f'  {stage}:')
        for i, batch in enumerate(batches, 1):
            print(f'    batch {i}: {", ".join(batch)}')


if __name__ == '__main__':
    main()