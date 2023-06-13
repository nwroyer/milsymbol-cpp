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

inline static constexpr const _impl::DrawCommand get_space_modifier(Affiliation affiliation) {
    const std::array<DrawCommand, SYMBOL_GEOMETRY_SUBINDEX_COUNT> SPACE_MODIFIERS = {
        // Hostile
        DrawCommand::path("M67,50 L100,20 133,50 z").with_fill(ColorType::ICON).with_stroke(ColorType::NONE),
        // Friend
        DrawCommand::path("M 100,30 C 90,30 80,35 68.65625,50 l 62.6875,0 C 120,35 110,30 100,30").with_fill(ColorType::ICON).with_stroke(ColorType::NONE),
        // Neutral
        DrawCommand::path("M45,50 l0,-20 110,0 0,20 z").with_fill(ColorType::ICON).with_stroke(ColorType::NONE),
        // Unknown
        DrawCommand::path("M 100 22.5 C 85 22.5 70 31.669211 66 50 L 134 50 C 130 31.669204 115 22.5 100 22.5 z").with_fill(ColorType::ICON).with_stroke(ColorType::NONE),
    };
    return SPACE_MODIFIERS[static_cast<int>(affiliation)];
}

inline static constexpr const _impl::DrawCommand get_activity_modifier(Affiliation affiliation) {
    const std::array<DrawCommand, SYMBOL_GEOMETRY_SUBINDEX_COUNT> ACTIVITY_MODIFIERS = {
        // Hostile
        DrawCommand::path("M 100 28 L 89.40625 38.59375 L 100 49.21875 L 110.59375 38.59375 L 100 28 z M 38.6875 89.3125 L 28.0625 99.9375 L 38.6875 110.53125 L 49.28125 99.9375 L 38.6875 89.3125 z M 161.40625 89.40625 L 150.78125 100 L 161.40625 110.59375 L 172 100 L 161.40625 89.40625 z M 99.9375 150.71875 L 89.3125 161.3125 L 99.9375 171.9375 L 110.53125 161.3125 L 99.9375 150.71875").with_fill(ColorType::ICON).with_stroke(ColorType::NONE),
        // Friend
        DrawCommand::path("m 160,135 0,15 15,0 0,-15 z m -135,0 15,0 0,15 -15,0 z m 135,-85 0,15 15,0 0,-15 z m -135,0 15,0 0,15 -15,0 z").with_fill(ColorType::ICON).with_stroke(ColorType::NONE),
        // Neutral
        DrawCommand::path("m 140,140 15,0 0,15 -15,0 z m -80,0 0,15 -15,0 0,-15 z m 80,-80 0,-15 15,0 0,15 z m -80,0 -15,0 0,-15 15,0 z").with_fill(ColorType::ICON).with_stroke(ColorType::NONE),
        // Unknown
        DrawCommand::path("M 107.96875 31.46875 L 92.03125 31.71875 L 92.03125 46.4375 L 107.71875 46.4375 L 107.96875 31.46875 z M 47.03125 92.5 L 31.09375 92.75 L 31.09375 107.5 L 46.78125 107.5 L 47.03125 92.5 z M 168.4375 92.5 L 152.5 92.75 L 152.5 107.5 L 168.1875 107.5 L 168.4375 92.5 z M 107.96875 153.5625 L 92.03125 153.8125 L 92.03125 168.53125 L 107.71875 168.53125 L 107.96875 153.5625 z").with_fill(ColorType::ICON).with_stroke(ColorType::NONE)
    };

    return ACTIVITY_MODIFIERS[static_cast<int>(affiliation)];
}

inline static constexpr Dimension get_base_dimension(Dimension dim) noexcept {
    if (dim == Dimension::UNDEFINED) {
        return Dimension::LAND;
    } else if (dim == Dimension::SPACE) {
        return Dimension::AIR;
    } else {
        return dim;
    }

}

inline static constexpr const _impl::DrawCommand get_base_symbol_geometry(Dimension index, Affiliation subindex,
    bool position_only = false)
{

    if (index == Dimension::UNDEFINED) {
        index = Dimension::LAND;
    }

    constexpr int SYMBOL_GEOMETRY_INDEX_COUNT = 6;

    const std::array<std::array<DrawCommand, SYMBOL_GEOMETRY_SUBINDEX_COUNT>, SYMBOL_GEOMETRY_INDEX_COUNT> SYMBOL_GEOMETRIES = {
        /*
         * Air units
         */

        std::array<DrawCommand, SYMBOL_GEOMETRY_SUBINDEX_COUNT>{
            // Air hostile
            DrawCommand::path(
                "M 45,150 L45,70 100,20 155,70 155,150",
                BoundingBox{45, 20, 45 + 110, 20 + 130}).with_fill(ColorType::ICON_FILL),

            // Air friend
            DrawCommand::path(
                "M 155,150 C 155,50 115,30 100,30 85,30 45,50 45,150",
                BoundingBox{45, 30, 45 + 110, 30 + 120}).with_fill(ColorType::ICON_FILL),

            // Air neutral
            DrawCommand::path(
                "M 45,150 L 45,30,155,30,155,150",
                BoundingBox{45, 30, 45 + 110, 30 + 120}).with_fill(ColorType::ICON_FILL),

            // Air unknown
            DrawCommand::path(
                "M 65,150 c -55,0 -50,-90 0,-90 0,-50 70,-50 70,0 50,0 55,90 0,90",
                BoundingBox{25, 20, 25 + 150, 20 + 130}).with_fill(ColorType::ICON_FILL)
        },

        /*
         * Ground units
         */

        std::array<DrawCommand, SYMBOL_GEOMETRY_SUBINDEX_COUNT>{
            // Ground hostile
            DrawCommand::path(
                "M 100,28 L172,100 100,172 28,100 100,28 Z",
                BoundingBox{28, 28, 28 + 144, 28 + 144}).with_fill(ColorType::ICON_FILL),

            // Ground friend
            DrawCommand::path(
                "M25,50 l150,0 0,100 -150,0 z",
                BoundingBox{25, 50, 25 + 150, 50 + 100}).with_fill(ColorType::ICON_FILL),

            // Ground neutral
            DrawCommand::path(
                "M45,45 l110,0 0,110 -110,0 z",
                BoundingBox{45, 45, 45 + 110, 45 + 110}).with_fill(ColorType::ICON_FILL),

            // Ground unknown
            DrawCommand::path(
                "M63,63 C63,20 137,20 137,63 C180,63 180,137 137,137 C137,180 63,180 63,137 C20,137 20,63 63,63 Z",
                BoundingBox{30.75, 30.75, 30.75 + 138.5, 30.75 + 138.5}).with_fill(ColorType::ICON_FILL)
        },

        /*
         * Land dismounted units
         */
        std::array<DrawCommand, SYMBOL_GEOMETRY_SUBINDEX_COUNT>{
            // Land dismounted hostile
            DrawCommand::path(
                "M 100,28 L172,100 100,172 28,100 100,28 Z",
                BoundingBox{28, 28, 28 + 144, 28 + 144}).with_fill(ColorType::ICON_FILL),

            // Land dismounted friend
            DrawCommand::path(
                "m 100,45 55,25 0,60 -55,25 -55,-25 0,-60 z",
                BoundingBox{45, 45, 45 + 1-0, 45 + 110}).with_fill(ColorType::ICON_FILL),

            // Land dismounted neutral
            DrawCommand::path(
                "M45,45 l110,0 0,110 -110,0 z",
                BoundingBox{45, 45, 45 + 110, 45 + 110}).with_fill(ColorType::ICON_FILL),

            // Land dismounted unknown
            DrawCommand::path(
                "M63,63 C63,20 137,20 137,63 C180,63 180,137 137,137 C137,180 63,180 63,137 C20,137 20,63 63,63 Z",
                BoundingBox{30.75, 30.75, 30.75 + 138.5, 30.75 + 138.5}).with_fill(ColorType::ICON_FILL)
        },

        /*
         * Sea units
         */
        std::array<DrawCommand, SYMBOL_GEOMETRY_SUBINDEX_COUNT>{
            // Sea hostile
            DrawCommand::path(
                "M100,28 L172,100 100,172 28,100 100,28 Z",
                BoundingBox{28, 28, 28 + 144, 28 + 144}).with_fill(ColorType::ICON_FILL),

            // Sea friend
            DrawCommand::circle(Vector2{100, 100}, 60).with_fill(ColorType::ICON_FILL),

            // Sea neutral
            DrawCommand::path(
                "M45,45 l110,0 0,110 -110,0 z",
                BoundingBox{45, 45, 45 + 110, 45 + 110}).with_fill(ColorType::ICON_FILL),

            // Sea unknown
            DrawCommand::path(
                "M63,63 C63,20 137,20 137,63 C180,63 180,137 137,137 C137,180 63,180 63,137 C20,137 20,63 63,63 Z",
                BoundingBox{30.75, 30.75, 30.75 + 138.5, 30.75 + 138.5}).with_fill(ColorType::ICON_FILL),
        },

            /*
         * Subsurface units
         */

        std::array<DrawCommand, SYMBOL_GEOMETRY_SUBINDEX_COUNT>{
            // Subsurface hostile
            DrawCommand::path(
                "M45,50 L45,130 100,180 155,130 155,50",
                BoundingBox{45, 50, 45 + 110, 50 + 120}).with_fill(ColorType::ICON_FILL),

            // Subsurface friend
            DrawCommand::path(
                "m 45,50 c 0,100 40,120 55,120 15,0 55,-20 55,-120",
                BoundingBox{45, 50, 45 + 110, 50 + 120}).with_fill(ColorType::ICON_FILL),

            // Subsurface neutral
            DrawCommand::path(
                "M45,50 L45,170 155,170 155,50",
                BoundingBox{45, 50, 45 + 110, 50 + 120}).with_fill(ColorType::ICON_FILL),

            // Subsurface unknown
            DrawCommand::path(
                "m 65,50 c -55,0 -50,90 0,90 0,50 70,50 70,0 50,0 55,-90 0,-90",
                BoundingBox{25, 50, 25 + 150, 50 + 130}).with_fill(ColorType::ICON_FILL),
        },

        /*
         * Position marker
         */
        std::array<DrawCommand, SYMBOL_GEOMETRY_SUBINDEX_COUNT>{
            // Hostile position marker
            DrawCommand::circle(Vector2{100, 100}, 15).with_fill(ColorType::ICON_FILL),

            // Friendly position marker
            DrawCommand::circle(Vector2{100, 100}, 15).with_fill(ColorType::ICON_FILL),

            // Neutral position marker
            DrawCommand::circle(Vector2{100, 100}, 15).with_fill(ColorType::ICON_FILL),

            // Unknown position marker
            DrawCommand::circle(Vector2{100, 100}, 15).with_fill(ColorType::ICON_FILL)
        }
    };

    return SYMBOL_GEOMETRIES[position_only ? SYMBOL_GEOMETRIES.size() - 1 : static_cast<int>(get_base_dimension(index))][static_cast<int>(get_base_affiliation(subindex))];
}

} // Impl namespace
} // milsymbol namespace
