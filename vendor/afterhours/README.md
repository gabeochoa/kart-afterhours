# afterhours


an ecs framework based on the one used in https://github.com/gabeochoa/pharmasea

check the examples folder for how to use it :) 


## compiler options: 

AFTER_HOURS_MAX_COMPONENTS
- sets how big the bitset is for components, defaults to 128

AFTER_HOURS_USE_RAYLIB
- some of the plugins use raylib functions (like raylib::SetWindowSize or raylib::IsKeyPressed). Not used for main library 

AFTER_HOURS_INCLUDE_DERIVED_CHILDREN
- Allows access to for_each_with_derived which will return all entities which match a component or a components children (TODO add an example) 

AFTER_HOURS_REPLACE_LOGGING
- if you want the library to log, implement the four functions and define this

AFTER_HOURS_REPLACE_VALIDATE
- same as logging but assert + log_error


## Plugins

Plugins are basically just helpful things I added to the library so i can help them in all my projects. They arent needed at all and dont provide examples (yet). All plugins (and any you make i hope) should implement the functions mentioned in developer.h 


### input (requires raylib)
a way to collect user input and map it to specific actions 

Components: 
- InputCollector => where all the action data goes
- ProvidesMaxGamepadID => the total gamepads connected
- ProvidesInputMapping => Stores the mapping from Keys => Actions
Update Systems: 
- InputSystem => does all the heavy lifting
Render Systems: 
- RenderConnectedGamepads => renders the number of gamepads connected


### window_manager (requires raylib)
gives access to resolution and window related functions

Components: 
- ProvidesTargetFPS
- ProvidesCurrentResolution
- ProvidesAvailableWindowResolutions
Update Systems: 
- CollectCurrentResolution => runs when ProvidesCurrentResolution.should_refetch = true
- CollectAvailableResolutions => runs when ProvidesAvailableWindowResolutions.should_refetch = true
Render Systems: 
- :)



examples in other repos:
- https://github.com/gabeochoa/tetr-afterhours/
- https://github.com/gabeochoa/wm-afterhours/
- https://github.com/gabeochoa/ui-afterhours/
