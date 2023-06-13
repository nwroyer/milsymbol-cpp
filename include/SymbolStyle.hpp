#pragma once

/*
 * This file contains a definition for symbol rendering styles.
 */

#include "Types.hpp"

namespace milsymbol {

/**
 * @brief Represents a style for rendering symbols. Optionally construction a SymbolStyle
 * object and passing it as a parameter lets you alter the representation of generated symbols.
 */
struct SymbolStyle {
    /**
     * @brief 2525D lets you choose between MEDAL icons (true) and alternate MEDAL icons (false) for Mines; default is set to MEDAL.
     * Since support for mine warfare icons isn't implemented yet, this doesn't do anything.
     */
    bool alternate_MEDAL = false;

    ColorMode color_mode = ColorMode::LIGHT; /// 2525C allows you to use Dark; Medium or Light colors. Unfilled is also included in here as a rendering option
    bool use_civilian_color = true; /// Whether to use a purple fill for friendly, neutral, and unknown civilian units

    real_t frame_stroke_width = 4; /// Numbers less than or equal to 0 will default back to this

    real_t hq_staff_length = 50; // The default length of the HQ staf
    real_t padding = 0; /// Extra padding around the symbol

    bool use_frame = true; /// Should the icon be framed
    bool use_entity_icon = true; /// Whether to show the entity icon
    bool use_modifiers = true; /// Whether to show modifiers
    bool use_amplifiers = true; /// Whether to use graphical amplifiers

    bool use_color_override = false; /// Whether to use a color override
    Color color_override; /// The color override to use

    /**
     * @brief Sets the style to use a color override and the color to override with
     * @param color The color to use for the override
     * @return
     */
    inline constexpr SymbolStyle& with_color_override(const Color& color) noexcept {
        use_color_override = true;
        color_override = color;
        return *this;
    }

    /**
     * @brief Sets the style to not use a color override
     * @return
     */
    inline constexpr SymbolStyle& without_color_override() noexcept {
        use_color_override = false;
        return *this;
    }
};

}
