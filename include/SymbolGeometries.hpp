#pragma once

/*
 * This file contains geometry functions for drawing symbol frames.
 */

#include <array>

#include "Types.hpp"
#include "BBox.hpp"
#include "DrawCommands.hpp"

namespace milsymbol {

namespace _impl {

static constexpr int SYMBOL_GEOMETRY_SUBINDEX_COUNT = 4; // Number of base affiliations

} // Impl namespace
} // milsymbol namespace
