#pragma once

/*
 * This file defines enumerations and typedefs for the library.
 */

namespace milsymbol {

using real_t = float;

/**
 * @brief Which color mode to use. Options are light (default), medium, dark, and unfilled.
 */
enum class ColorMode {
    LIGHT = 0,
    MEDIUM,
    DARK,
    UNFILLED
};

/**
 * @brief The affiliation of a symbol. Options are pending, hostile,
 * friend, neutral, unknown, suspect, and assumed friend.
 */
enum class Affiliation {
    PENDING = -1,
    HOSTILE = 0, /// 6 in SIDC
    FRIEND, /// 3 in SIDC
    NEUTRAL, /// 4 in SIDC
    UNKNOWN, /// 1 in SIDC

    SUSPECT, /// 5 in SIDC
    ASSUMED_FRIEND /// 2 in SIDC
};

/**
 * @brief The dimension (air, land, sea, etc.) of a symbol. This doesn't map 1:1 with symbol sets,
 * but generally aligns with the symbol frame shape.
 */
enum class Dimension {
    UNDEFINED = -1,

    AIR = 0,
    LAND, // 1
    LAND_DISMOUNT, // 2
    SEA, // 3
    SUBSURFACE, // 4

    SPACE, // 5
    CYBERSPACE, // 6

    POSITION_MARKER // 7
};

/**
 * @brief Indicates the present/anticipated/planned status of a symbol. For 2525D, there's only really
 * present and non-present as far as rendering goes.
 */
enum class Presence {
    PRESENT = 0, /// 0 in SIDC
    ANTICIPATED, /// 1 in SIDC
    PLANNED /// 1 in SIDC
};

/**
 * @brief The status of a symbol. Usually only applicable for equipment.
 */
enum class Status {
    UNDEFINED = -1,
    FULLY_CAPABLE = 0, /// 2 in SIDC, same as presence
    DAMAGED, /// 3 in SIDC, same as presence
    DESTROYED, /// 4 in SIDC, same as presence
    FULL_TO_CAPACITY /// 5 in SIDC, same as presence
};

/**
 * @brief The context of a symbol - reality (default), exercise, or simulation.
 */
enum class Context {
    REALITY = 0,   /// 0 in SIDC
    EXERCISE = 1,  /// 1 in SIDC
    SIMULATION = 2 /// 2 in SIDC
};

/**
 * @brief The echelon of a symbol. Alternate names for the Marine Corps or foreign
 * echelons (e.g. MEF instead of Corps) aren't provided here.
 */
enum class Echelon {
    UNDEFINED = -1,
    TEAM = 0,
    SQUAD,
    SECTION,
    PLATOON,
    COMPANY,
    BATTALION,
    REGIMENT,
    BRIGADE,
    DIVISION,
    CORPS,
    ARMY,
    ARMY_GROUP,
    REGION,
    COMMAND
};

/**
 * @brief Mobility for equipment. Occupies the same digits as echelon for equipment.
 */
enum class Mobility{
    UNDEFINED = 0,
    WHEELED,
    WHEELED_CROSS_COUNTRY,
    TRACKED,
    WHEELED_AND_TRACKED,
    TOWED,
    RAIL,
    PACK_ANIMALS,

    // On snow
    OVER_SNOW,
    SLED,

    // On water
    BARGE,
    AMPHIBIOUS,

    // Towed sonar
    SHORT_TOWED_ARRAY,
    LONG_TOWED_ARRAY
};

/**
 * @brief Represents an SVG color
 */
struct Color {
    using base_t = short;

    base_t r = 0; /// Red component, from 0-255 inclusive
    base_t g = 0; /// Green component, from 0-255 inclusive
    base_t b = 0; /// Blue component, from 0-255 inclusive

    static constexpr base_t MIN = 0; /// Minimum color value
    static constexpr base_t MAX = 255; /// Max color value

    /// Default constructor (black)
    inline constexpr Color() noexcept : r{0}, g{0}, b{0} {};

    /// Constructs a color with the specified RGB components (0-255)
    inline constexpr Color(base_t r, base_t g, base_t b) noexcept : r{r}, g{g}, b{b} {};
};

}
