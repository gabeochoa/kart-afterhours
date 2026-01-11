---
description: "MCP (Model Context Protocol) integration for AI-assisted UI testing and automation"
alwaysApply: false
---

# MCP Server Integration

This project includes an MCP server for AI-assisted testing and automation. See `MCP_GUIDE.md` for full documentation.

## Quick Reference

### Starting MCP Mode
```bash
./output/kart.exe --mcp
./output/kart.exe --mcp --screen=simple_button
```

### Available Tools
| Tool | Description | Arguments |
|------|-------------|-----------|
| `ping` | Check server is responsive | none |
| `screenshot` | Capture frame as PNG | none |
| `get_screen_size` | Get window dimensions | none |
| `mouse_move` | Move cursor | `{x, y}` |
| `mouse_click` | Click at position | `{x, y, button}` |
| `key_press` | Press and release key | `{key}` |
| `key_down` | Hold key down | `{key}` |
| `key_up` | Release held key | `{key}` |
| `dump_ui_tree` | Get UI component hierarchy | none |
| `exit` | Close application | none |

### Common Key Names
- Letters: `a`-`z`
- Numbers: `0`-`9`
- Navigation: `up`, `down`, `left`, `right`, `enter`, `escape`, `tab`, `space`
- Screen cycling: `,` (prev), `.` (next)

### Python Client Pattern
```python
import subprocess
import json

proc = subprocess.Popen(
    ["./output/kart.exe", "--mcp"],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.DEVNULL,
    text=True, bufsize=1
)

def call_tool(name, args=None):
    request = {
        "jsonrpc": "2.0", "id": 1,
        "method": "tools/call",
        "params": {"name": name, "arguments": args or {}}
    }
    proc.stdin.write(json.dumps(request) + "\n")
    proc.stdin.flush()
    return json.loads(proc.stdout.readline())

# Initialize first
proc.stdin.write(json.dumps({
    "jsonrpc": "2.0", "id": 0, "method": "initialize",
    "params": {"protocolVersion": "2024-11-05", "capabilities": {}}
}) + "\n")
proc.stdin.flush()
proc.stdout.readline()

# Now use tools
call_tool("key_press", {"key": ","})  # Previous screen
call_tool("screenshot")                # Capture frame
```

### Implementation Files
- `vendor/afterhours/src/plugins/mcp_server.h` - MCP protocol handler
- `src/engine/input_injector.h/cpp` - Input simulation
- `src/game.cpp` - MCP integration in game loop
- `test_mcp.py` - Example Python client

