# How to Play: Kart Afterhours

A local multiplayer kart battle game with multiple game modes, weapons, and chaotic fun!

## Quick Start

1. **Launch the game**: `./output/kart.exe`
2. **Main Menu**: Use arrow keys to navigate, Enter to select
3. **Add players**: Select "play" → navigate to empty slots → press Enter to add players
4. **Start game**: Once players are added, navigate and press Enter to start

## Controls

### Menu Navigation
| Key | Action |
|-----|--------|
| Arrow Keys | Navigate menu items |
| Enter | Select/Confirm |
| Escape | Go back |
| Tab / Down | Next widget |
| Up | Previous widget |

### In-Game (Keyboard)
| Key | Action |
|-----|--------|
| ↑ Up Arrow | Accelerate |
| ↓ Down Arrow | Brake/Reverse |
| ← Left Arrow | Steer left |
| → Right Arrow | Steer right |
| Space | Boost |
| Q | Shoot left |
| E | Shoot right |
| H | Honk |
| Escape | Pause/Menu |

### In-Game (Gamepad)
| Input | Action |
|-------|--------|
| Right Stick Y | Accelerate/Brake |
| Left Stick X | Steering |
| Right Trigger | Boost |
| Left Bumper | Shoot left |
| Right Bumper | Shoot right |
| Right Thumb | Honk |
| Start | Pause |

## Game Modes

### 1. Lives Mode
- Each player starts with a set number of lives
- Lose a life when killed by another player
- Last player standing wins
- No time limit

### 2. Kills Mode
- Timed mode (default: 1 minute)
- Score points by eliminating other players
- Most kills when time runs out wins

### 3. Hippo Mode
- Collect hippo items that spawn on the map
- First player to collect the target number wins
- Timed with spawn waves

### 4. Tag and Go Mode
- One player is "IT" (the tagger)
- Tagger is slowed down (70% speed)
- Tag another player to pass "IT" status
- Player with least time as "IT" wins when timer ends
- Brief immunity after being tagged

## Round Settings

Before starting a game, you can configure:

- **Game Mode**: Select between Lives, Kills, Hippo, or Tag and Go
- **Team Mode**: Enable Team A vs Team B gameplay
- **Time Limit**: Unlimited, 10s, 30s, or 1 minute
- **Weapons**: Enable/disable different weapon types
- **Starting Lives**: (Lives mode only) Number of lives per player

## Tips

1. **Master drifting**: Quick direction changes at high speed create skid marks and maintain momentum
2. **Use boost wisely**: Boost is powerful but has cooldown - save it for escaping or chasing
3. **Aim your shots**: Weapons fire left (Q) or right (E) relative to your car's direction
4. **Watch the map**: The arena wraps around - you can shoot through edges
5. **In Tag mode**: The tagger is bigger and slower - use your agility if you're not IT

## Debug/Dev Controls

| Key | Action |
|-----|--------|
| ` (Grave) | Toggle UI debug overlay |
| = (Equal) | Toggle UI layout debug |

## MCP Mode (AI Automation)

The game supports the Model Context Protocol for AI automation:

```bash
# Build with MCP support
make MCP=1

# Run in MCP mode
./output/kart.exe --mcp
```

Available MCP tools:
- `screenshot` - Capture current frame
- `key_press`, `key_down`, `key_up` - Simulate keyboard input
- `mouse_click`, `mouse_move` - Simulate mouse
- `dump_ui_tree` - Get current UI state
- `get_screen_size` - Get window dimensions
- `exit` - Close the game

---

*Cart Chaos - Local multiplayer kart combat!*

