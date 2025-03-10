#pragma once

/*
 * This file contains a definition of the core symbol object.
 */

#include <vector>
#include <cstdint>
#include <string>
#include <numeric>
#include <string_view>

#include "BBox.hpp"
#include "Constants.hpp"
#include "SymbolStyle.hpp"

namespace milsymbol {

/**
 * @brief Represents a MILSTD-2525D symbol that can be constructed manually
 * or from a SIDC representation.
 */
struct Symbol {

    using entity_t = int32_t;
    using modifier_t = int16_t;

    /**
     * @brief Constructs a default symbol.
     */
    constexpr Symbol() noexcept {}

    /**
     * @brief Creates a new symbol from a SIDC. The SIDC must be convertible to std::string_view and be at least 20 characters
     * in length of all numerals. If a SIDC is less than 20 characters, an empty symbol will be returned. If a SIDC contains
     * non-numeric characters, the behavior will be undefined, but is intended to be exception-safe.
     */
    static Symbol from_sidc(const std::string& sidc) noexcept;

    /**
     * @brief Helper function to set an affiliation inline
     * @param affil The affiliation to set to
     * @return This same object, modified
     */
    inline constexpr Symbol& with_affiliation(Affiliation affil) noexcept {
        this->affiliation = affil;
        return *this;
    }

    /**
     * @brief Helper function to set an echelon inline
     * @param echelon The echelon to set to
     * @return This same object, modified
     */
    inline constexpr Symbol& with_echelon(Echelon echelon) noexcept {
        this->echelon = echelon;
        return *this;
    }

    /**
     * @brief Helper function to set equipment mobility inline
     * @param mobility The mobility to set to
     * @return This same object, modified
     */
    inline constexpr Symbol& with_mobility(Mobility mobility) noexcept {
        this->mobility = mobility;
        return *this;
    }

    /**
     * @brief Helper function to set a context (reality, simulation, etc.) inline
     * @param context The context to set to
     * @return This same object, modified
     */
    inline constexpr Symbol& with_context(Context context) noexcept {
        this->context = context;
        return *this;
    }

    /**
     * @brief Helper function to set a feint/dummy status inline.
     * @param feint_dummy Whether the symbol is a feint/dummy (true) or not (false)
     * @return This same object, modified
     */
    inline constexpr Symbol& as_feint_or_dummy(bool feint_dummy) noexcept {
        this->feint_dummy = feint_dummy;
        return *this;
    }

    /**
     * @brief Helper function to set the symbol's headquarter status inline.
     * @param hq Whether the symbol is a headquarters
     * @return This same object, modified
     */
    inline constexpr Symbol& as_headquarters(bool hq) noexcept {
        this->headquarters = hq;
        return *this;
    }

    /**
     * @brief Helper function to set the symbol's task force status inline.
     * @param tf Whether the symbol is a task force
     * @return This same object, modified
     */
    inline constexpr Symbol& as_task_force(bool tf) noexcept {
        this->task_force = tf;
        return *this;
    }

    /**
     * @brief Helper function to set the symbol's entity.
     * @param entity The entity code to set, as a 7-8 digit integer. The last 6 digits are the
     * actual entity code, while the first 1 or 2 are the symbol set.
     * @return This same object, modifier
     */
    inline constexpr Symbol& with_entity(Entity entity) noexcept {
        // Set invalid entities to zero
        if (entity < ENTITY_SYMBOL_SET_OFFSET) {
            this->entity = 0;
        }

        this->entity = entity;
        return *this;
    }

    inline constexpr Vector2 get_anchor() const noexcept {return symbol_anchor;} /// Returns the symbol anchor
    inline constexpr Vector2 get_octagon_anchor() const noexcept {return octagon_anchor;} /// Returns the octagon anchor

    /**
     * @brief Returns an SVG representation of this symbol as a std::string.
     *
     * A SymbolStyle object may be passed as a const reference to define the style; if not provided, a default
     * SymbolStyle will be used. This defaults to using the light color scheme with 100px headquarter staffs
     * and zero extra padding around the borders of the symbol.
     */
    inline std::string get_svg_string(const SymbolStyle& style = {}) const noexcept {
        return get_svg(style).svg;
    }

    /**
     * @brief Represents a "rendered" SVG representation of a symbol along
     * with data relevant for common use cases.
     */
    struct RichOutput {
        /**
         * @brief String SVG representation of the rendered symbol
         */
        std::string svg;

        /**
         * @brief Bounding box of the SVG, relative to the SVG's viewbox
         *
         * This is relative to the SVG's viewbox - (0, 0) is always the top left
         * corner of the full SVG.
         */
        BoundingBox svg_bounding_box;

        /**
         * @brief Bounding box of the icon frame itself.
         *
         * This is relative to the SVG's viewbox - (0, 0) is always the top left
         * corner of the full SVG.
         */
        BoundingBox frame_bounding_box;

        /**
         * @brief Symbol anchor of the symbol (usually the center, or the base of the
         * headquarters staff for HQs).
         *
         * This is relative to the SVG's viewbox - (0, 0) is always the top left
         * corner of the full SVG.
         */
        Vector2 symbol_anchor;
    };

    /**
     * @brief Returns an SVG representation of this symbol as a std::string.
     *
     * A SymbolStyle object may be passed as a const reference to define the style; if not provided, a default
     * SymbolStyle will be used. This defaults to using the light color scheme with 100px headquarter staffs
     * and zero extra padding around the borders of the symbol.
     */
    RichOutput get_svg(const SymbolStyle& style = {}) const noexcept;

    /**
     * @brief Returns whether this is a joker or faker (a friendly unit acting as a suspect or hostile unit for an exercise)
     */
    inline constexpr bool is_joker_or_faker() const noexcept {
        return (context != Context::REALITY && (affiliation == Affiliation::HOSTILE || affiliation == Affiliation::SUSPECT));
    }


    inline constexpr void set_affiliation(Affiliation affiliation) noexcept {this->affiliation = affiliation;} /// Setter for affiliation
    inline constexpr Affiliation get_affiliation() const noexcept {return affiliation;} /// Getter for affiliation

    inline constexpr void set_status(Status status) noexcept {this->status = status;} /// Setter for status
    inline constexpr Status get_status() const noexcept {return status;} /// Getter for status

    inline constexpr void set_echelon(Echelon echelon) noexcept {this->echelon = echelon;} /// Setter for echelon
    inline constexpr Echelon get_echelon() const noexcept {return echelon;} /// Getter for echelon

    inline constexpr void set_mobility(Mobility mobility) noexcept {this->mobility = mobility;} /// Setter for mobility
    inline constexpr Mobility get_mobility() const noexcept {return mobility;} /// Getter for mobility

    inline constexpr void set_feint_or_dummy(bool feint_dummy) noexcept {this->feint_dummy = feint_dummy;} /// Setter for feint/dummy
    inline constexpr bool is_feint_or_dummy() const noexcept {return feint_dummy;} /// Getter for feint/dummy

    inline constexpr void set_headquarters(bool headquarters) noexcept {this->headquarters = headquarters;} /// Setter for headquarters
    inline constexpr bool is_headquarters() const noexcept {return headquarters;} /// Getter for headquarters

    inline constexpr void set_task_force(bool task_force) noexcept {this->task_force = task_force;} /// Setter for headquarters
    inline constexpr bool is_task_force() const noexcept {return task_force;} /// Getter for headquarters

    inline constexpr SymbolSet get_symbol_set() const noexcept {return symbol_set;}

    static std::vector<entity_t> get_all_entities(SymbolSet symbol_set) noexcept;
    static std::vector<entity_t> get_all_modifier_1s(SymbolSet symbol_set) noexcept;
    static std::vector<entity_t> get_all_modifier_2s(SymbolSet symbol_set) noexcept;
    static std::vector<entity_t> get_all_symbol_sets() noexcept;

    inline entity_t get_entity() const noexcept {return entity;}
    modifier_t get_modifier(int mod) const noexcept;

private:

    static constexpr entity_t ENTITY_SYMBOL_SET_OFFSET = 1000000;
    static constexpr modifier_t MODIFIER_SYMBOL_SET_OFFSET = 100;

    static_assert(std::numeric_limits<entity_t>::max() > 99999999, "Insufficient space for entity storage"); // Check for storing entities as 8-digit integers for speed
    static_assert(std::numeric_limits<modifier_t>::max() > 9999, "Insufficient space for modifier storage"); // Check for storing entities as 4-digit integers for speed

    /*
     * Metadata
     */

    Affiliation affiliation = Affiliation::UNKNOWN; /// The displayed affiliation. Defaults to unknown.
    Status status = Status::PRESENT; /// Which condition this icon is in, for equipment. Defaults to present (no particular status).
    Context context = Context::REALITY; /// The context of the symbol (reality, exercise, or simulation). Defaults to reality.
    Echelon echelon = Echelon::UNDEFINED; /// The echelon of this symbol. Defaults to undefined (no echelon).
    Mobility mobility = Mobility::UNDEFINED; /// The symbol mobility (for equipment)

    bool feint_dummy = false; /// Whether this is a fake/dummy (true) or not (false)
    bool headquarters = false; /// Whether this is a headquarters (with a staff indicator)
    bool task_force = false; /// Whether this symbol indicates a task force

    // Symbols
    SymbolSet symbol_set = SymbolSet::LAND_UNIT;
    entity_t entity = 0;    /// The entity ID for the symbol. from 0-999999 inclusive
    modifier_t modifier_1 = 0; /// Modifier 1 code, from 0-99 inclusive
    modifier_t modifier_2 = 0; /// Modifier 2 code, from 0-99 inclusive

    /*
     * Positioning data
     */

    Vector2 octagon_anchor = Vector2{100, 100}; /// The anchor point for the octagon in the current symbol
    Vector2 symbol_anchor = Vector2{100, 100}; /// The symbol anchor


}; // End of class definition

} // End of the milsymbol namespace

