#pragma once

#include "library.h"
#include <afterhours/src/singleton.h>

// Forward declare the new multipass shader system
#include "multipass_shader_system.h"

// The old ShaderLibrary is now replaced by the new one in multipass_shader_system.h
// This file is kept for backward compatibility but the actual implementation
// is now in the multipass system

// Remove the old UpdateShaderValues system since it's now handled by the multipass system
// Remove the old BeginShader system since it's now handled by the multipass system
