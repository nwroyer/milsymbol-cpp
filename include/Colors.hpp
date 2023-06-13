#pragma once

#include <array>
#include <sstream>
#include <assert.h>

#include "Types.hpp"

namespace milsymbol::_impl {

enum class ColorType {
    NONE = -1,
    ICON = 0, /// Foreground color for an icon
    ICON_FILL, /// The background full color for an icon
    WHITE, /// A white color for an icon
    YELLOW /// A yellow color for an icon
};

enum class ColorIndex {
    BLACK = 0,
    FRAME_COLOR,
    LIGHT,
    MEDIUM,
    DARK,
    WHITE
};

enum class ColorSubIndex {
    HOSTILE = 0,
    FRIEND,
    NEUTRAL,
    UNKNOWN,
    CIVILIAN
};

static constexpr int COLOR_SUB_INDEX_COUNT = 5; /// How many color sub-indices
static constexpr int COLOR_INDEX_COUNT = 6; /// How many color types there are defined

/**
 * Color list
 */
static constexpr std::array<std::array<Color, COLOR_SUB_INDEX_COUNT>, COLOR_INDEX_COUNT> COLORS = {
    // Colors::BLACK
    std::array<Color, COLOR_SUB_INDEX_COUNT>{
        Color{0, 0, 0}, // Hostile
        Color{0, 0, 0}, // Friend
        Color{0, 0, 0}, // Neutral
        Color{0, 0, 0}, // Unknown
        Color{0, 0, 0}  // Civilian
    },
    // Colors::FRAME_COLOR
    std::array<Color, COLOR_SUB_INDEX_COUNT>{
        Color{255, 0,   0  }, // Hostile
        Color{0,   255, 255}, // Friend
        Color{0,   255, 0  }, // Neutral
        Color{255, 255, 0  }, // Unknown
        Color{255, 0,   255}  // Civilian
    },
    // Colors::LIGHT
    std::array<Color, COLOR_SUB_INDEX_COUNT>{
        Color{255, 128, 128}, // Hostile
        Color{128, 224, 255}, // Friend
        Color{170, 255, 170}, // Neutral
        Color{255, 255, 128}, // Unknown
        Color{255, 161, 255}  // Civilian
    },
    // Colors::MEDIUM
    std::array<Color, COLOR_SUB_INDEX_COUNT>{
        Color{255, 48,  49 }, // Hostile
        Color{0,   168, 220}, // Friend
        Color{0,   226, 110}, // Neutral
        Color{255, 255, 0  }, // Unknown
        Color{128, 0,   128}  // Civilian
    },
    // Colors::DARK
    std::array<Color, COLOR_SUB_INDEX_COUNT>{
        Color{200, 0,   0  }, // Hostile
        Color{0,   107, 140}, // Friend
        Color{0,   160, 0  }, // Neutral
        Color{225, 220, 0  }, // Unknown
        Color{80,  0,   80 }  // Civilian
    },
    // Colors::WHITE
    std::array<Color, COLOR_SUB_INDEX_COUNT>{
        Color{255, 255, 255}, // Hostile
        Color{255, 255, 255}, // Friend
        Color{255, 255, 255}, // Neutral
        Color{255, 255, 255}, // Unknown
        Color{255, 255, 255}  // Civilian
    }
};

/**
 * @brief get_base_affiliation Returns the base affiliation (e.g. assumed friend for friend) that gives the appropriate
 * affiliation for icon fills and shapes.
 * @param aff The affiliation to return the base of.
 * @return
 */
static constexpr Affiliation get_base_affiliation(Affiliation aff) {
    if (aff == Affiliation::PENDING) {
        return Affiliation::UNKNOWN;
    } if (aff == Affiliation::SUSPECT) {
        return Affiliation::HOSTILE;
    } else if (aff == Affiliation::ASSUMED_FRIEND) {
        return Affiliation::FRIEND;
    } else {
        return aff;
    }
}

/**
 * @brief Returns a color with the specified indices
 * @param index
 * @param sub_index
 * @return
 */
static constexpr Color get_color(ColorType color_type,
                                 Affiliation affiliation,
                                 bool civilian = false,
                                 ColorMode color_mode = ColorMode::LIGHT) {

    // Ensure no civilian fill for hostile symbols
    civilian = civilian && !(affiliation == Affiliation::HOSTILE || affiliation == Affiliation::SUSPECT);

    switch(color_type) {
    case ColorType::ICON_FILL:
        return COLORS[color_mode == ColorMode::UNFILLED ? static_cast<int>(ColorIndex::FRAME_COLOR) : static_cast<int>(color_mode) + 2]
                     [civilian ? static_cast<int>(ColorSubIndex::CIVILIAN) : static_cast<int>(get_base_affiliation(affiliation))];
        break;
    case ColorType::WHITE:
        return color_mode == ColorMode::UNFILLED ? Color{-1, -1, -1} : (COLORS[static_cast<int>(ColorIndex::WHITE)]
                      [civilian ? static_cast<int>(ColorSubIndex::CIVILIAN) : static_cast<int>(get_base_affiliation(affiliation))]);
        break;
    case ColorType::ICON:
        return COLORS[static_cast<int>(color_mode == ColorMode::UNFILLED ? ColorIndex::FRAME_COLOR : ColorIndex::BLACK)]
                     [civilian ? static_cast<int>(ColorSubIndex::CIVILIAN) : static_cast<int>(get_base_affiliation(affiliation))];
        break;
    case ColorType::YELLOW:
        return color_mode == ColorMode::UNFILLED ? Color{-1, -1, -1} : COLORS[static_cast<int>(ColorIndex::LIGHT)][static_cast<int>(ColorSubIndex::UNKNOWN)];
        break;
    case ColorType::NONE:
    default:
        return Color{-1, -1, -1};
        break;
    }
}

}
