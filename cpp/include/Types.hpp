#pragma once

#include <charconv>
#include <string_view>
#include <iostream>

/*
 * This file defines enumerations and typedefs for the library.
 */

namespace milsymbol {

using real_t = float;

namespace _impl {
static int hex_from_substring(const std::string_view string_view) noexcept {
    int result = 0;
    std::size_t chars_consumed = 0;
    const char* first = string_view.data();
    const char* last = string_view.data() + string_view.length();
    std::from_chars_result res = std::from_chars(first, last, result, 16);

    if (res.ec != std::errc()) {
        std::cerr << "Invalid hexadecimal string \"" << string_view << "\": error " << static_cast<int>(res.ec) << std::endl;
        return 0;
    }

    if (res.ptr != last) {
        std::cerr << "Invalid hexadecimal string \"" << string_view << "\": overflow error " << std::endl;
        return 0;
    }

    return result;
}
}

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
