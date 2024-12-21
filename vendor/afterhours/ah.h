
#pragma once

#include <array>
#include <bitset>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace afterhours {

#if defined(AFTER_HOURS_MAX_COMPONENTS)
#define AFTER_HOURS_MAX_COMPONENTS 128
#endif

#include "src/entity.h"
#include "src/entity_helper.h"
#include "src/entity_query.h"
#include "src/system.h"

} // namespace afterhours
