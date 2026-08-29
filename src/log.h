
#pragma once

// FMT_HEADER_ONLY now comes from build.zig so every TU agrees. Defining it
// here only covered TUs that include this header.
#include <fmt/args.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#define AFTER_HOURS_REPLACE_LOGGING
#define AFTER_HOURS_LOG_WITH_COLOR
// #define AFTER_HOURS_ENTITY_ALLOC_DEBUG
#include "log/log.h"
