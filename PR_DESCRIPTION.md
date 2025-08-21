# PR: Implement multipass rendering system with shader pass registry

## Overview

This PR implements a comprehensive multipass rendering system for Kart Afterhours that replaces the previous string-based shader system with a type-safe, enum-based approach.

## Key Features

- **Multipass Rendering**: Organized render passes with clear priority system
- **Shader Pass Registry**: Centralized management of render passes and shaders
- **Type Safety**: Enum-based shader types and uniform locations
- **Performance**: Optimized memory allocation and reduced complexity
- **Maintainability**: Clean separation of concerns and comprehensive documentation

## Technical Improvements

- Replaced magic numbers with clear `RenderPriority` enums
- Enhanced `HasShader` component to support multiple shaders and priorities
- Implemented `MultipassRenderer` for organized rendering pipeline
- Added `ShaderPassRegistry` for pass management and entity filtering
- Created `UpdateShaderValues` system for uniform management
- Fixed namespace structure issues in systems.h

## Code Quality

- Improved const correctness in template parameters
- Refactored complex methods into focused, single-purpose functions
- Enhanced string building logic in debug methods
- Optimized memory allocations with pre-allocation
- Added comprehensive inline documentation
- Consistent formatting and naming conventions

## Breaking Changes

None - this is a pure enhancement that maintains backward compatibility.

## Testing

- Builds successfully with xmake
- All existing systems continue to work
- New multipass system is opt-in and doesn't affect current rendering

## Files Changed

- `src/multipass_integration.h` - Integration examples and systems
- `src/multipass_renderer.h` - Core multipass rendering logic
- `src/shader_pass_registry.h` - Pass management and entity filtering
- `src/shader_types.h` - Type definitions and enums
- `src/systems.h` - Fixed namespace structure
- `src/shader_library.h` - Added UpdateShaderValues system
- `src/components.h` - Enhanced HasShader component

## Commit History

- `ff0beb9` - fix const correctness in template parameters
- `7d82fb3` - refactor complex template methods into smaller focused functions  
- `f01f342` - improve string building logic in debug methods
- `855a2fe` - improve brace formatting consistency in render pass definitions
- `4923d63` - add comprehensive inline documentation for complex logic sections
- `23ac1ed` - optimize memory allocations in registry methods with pre-allocation
- `abd60a3` - add documentation explaining priority number gaps for future extensibility
- `0a8b933` - remove rfc multipass document

## How to Create PR

1. Go to https://github.com/gabeochoa/kart-afterhours
2. Click "Compare & pull request" for the `feature/multipass-shader-system` branch
3. Copy the content above into the PR description
4. Set the title to: "Implement multipass rendering system with shader pass registry"
5. Set the base branch to `main`
6. Create the pull request
