#pragma once

/*
 * This file contains draw commands, most of which are definable at compile time,
 * to make up symbols.
 */

#include <iostream>
#include <variant>
#include <optional>
#include <vector>

#include "Types.hpp"
#include "Colors.hpp"
#include "BBox.hpp"

namespace milsymbol::_impl {

using SVGString = std::string;

/// Stroke options - only solid or dashed for now
enum class StrokeStyle {
    SOLID = 0,
    DASHED
};

/// Font weights - if compiled with the SVG text option, this
/// doesn't do anything.
enum FontWeight {
    NORMAL = 0,
    BOLD
};

/// The text alignment. If compiled with the SVG text option, this doesn't do anything.
enum TextAlignment {
    START = 0,
    MIDDLE,
    END
};

/// Returns the appropriate SVG string to indicate a font weight
static constexpr const char* get_font_weight_string(FontWeight font_weight) {
    switch(font_weight) {
    case FontWeight::BOLD:
        return "bold";
    default:
        return "normal";
    }
}

/// Returns the appropriate SVG string to indicate a text alignment
static constexpr const char* get_font_alignment_string(TextAlignment alignment) {
    switch(alignment) {
    case TextAlignment::START:
        return "start";
    case TextAlignment::END:
        return "end";
    default:
        return "middle";
    }
}


/**
 * @brief Represents a style with which to draw a symbol element
 */
struct Style {
    static constexpr real_t DEFAULT_STROKE_WIDTH = 4;

    Affiliation affiliation = Affiliation::UNKNOWN;
    bool civilian = false;
    ColorMode color_mode = ColorMode::LIGHT;
    bool use_color_override = false;
    Color color_override = Color{255, 255, 0};
    float stroke_width_override = -1;

    inline constexpr Color get_color(ColorType color_type) const noexcept {
        if (use_color_override) {
            if (color_type == ColorType::ICON_FILL) {
                return color_override;
            } else if (color_mode == ColorMode::UNFILLED && color_type != ColorType::NONE) {
                return color_override;
            }
        }

        return _impl::get_color(color_type, affiliation, civilian, color_mode);
    }
};

/**
 * @brief Base class for draw instruction data
 */
template<typename Derived>
struct DrawInstructionBase {

    // Fill color override
    ColorType fill_color = ColorType::ICON;
    ColorType stroke_color = ColorType::NONE;
    real_t stroke_width = Style::DEFAULT_STROKE_WIDTH;
    StrokeStyle stroke_style = StrokeStyle::SOLID;

    /*
     * Modifiers
     */
    inline constexpr Derived& with_fill(ColorType use_fill) noexcept {
        this->fill_color = use_fill;
        return *static_cast<Derived*>(this);
    }

    inline constexpr Derived& with_stroke(ColorType use_stroke) noexcept {
        this->stroke_color = use_stroke;
        return *static_cast<Derived*>(this);
    }

    inline constexpr Derived& with_stroke_width(real_t width) noexcept {
        this->stroke_width = width;
        return *static_cast<Derived*>(this);
    }

    inline constexpr Derived& with_stroke_style(StrokeStyle stroke_style) noexcept {
        this->stroke_style = stroke_style;
        return *static_cast<Derived*>(this);
    }

    /*
     * Const modifiers
     */
    inline constexpr Derived copy_with_fill(ColorType use_fill) const noexcept {
        Derived ret = *this;
        ret.fill_color = use_fill;
        return ret;
    }

    inline constexpr Derived copy_with_stroke(ColorType use_stroke) const noexcept {
        Derived ret = *this;
        ret.stroke_color = use_stroke;
        return ret;
    }

    inline constexpr Derived copy_with_stroke_width(real_t width) const noexcept {
        Derived ret = *this;
        ret.stroke_width = width;
        return ret;
    }

    inline constexpr Derived copy_with_stroke_style(StrokeStyle style) const noexcept {
        Derived ret = *this;
        ret.stroke_style = style;
        return ret;
    }

    std::optional<ColorIndex> color_override;
};

/**
 * @brief SVG command indicating a path
 */
struct DrawInstructionPath : public DrawInstructionBase<DrawInstructionPath> {

    inline constexpr DrawInstructionPath() : DrawInstructionBase(), d{""}, bbox{}, dynamic_path{} {
        fill_color = ColorType::NONE;
        stroke_color = ColorType::ICON;
    };

    inline constexpr DrawInstructionPath(const char* d, const BoundingBox& bbox) :
        DrawInstructionBase(),
        d{d}, bbox{bbox}, dynamic_path{}
    {
        fill_color = ColorType::NONE;
        stroke_color = ColorType::ICON;
    }

    inline constexpr DrawInstructionPath(const DrawInstructionPath& other) :
        DrawInstructionBase<DrawInstructionPath>{other}, bbox{other.bbox}, d{other.d}, dynamic_path{other.dynamic_path} {}

    inline constexpr DrawInstructionPath& operator=(const DrawInstructionPath& other) {
        DrawInstructionBase<DrawInstructionPath>::operator=(other);
        bbox = other.bbox;
        d = other.d;
        dynamic_path = other.dynamic_path;
        return *this;
    }

    BoundingBox bbox;
    const char* d = ""; /// Path string
    std::string dynamic_path;

    std::string get_svg_string(const Style& context) const noexcept;
};

/**
 * @brief SVG command indicating a circle
 */
struct DrawInstructionCircle : public DrawInstructionBase<DrawInstructionCircle> {

    inline constexpr DrawInstructionCircle(const Vector2& center, real_t radius) :
        DrawInstructionBase(),
        center{center}, radius{radius}
    {
        fill_color = ColorType::NONE;
        stroke_color = ColorType::ICON;
    };

    Vector2 center;
    real_t radius = 1;
    real_t stroke_width = 4;

    std::string get_svg_string(const Style& context) const noexcept;
    inline constexpr BoundingBox get_bbox() const noexcept {return BoundingBox{center.x - radius, center.y - radius, center.x + radius, center.y + radius};}
};

/**
 * @brief SVG command indicating text
 */
struct DrawInstructionText : public DrawInstructionBase<DrawInstructionText> {

    inline constexpr DrawInstructionText(const char* text, const Vector2& xy, int font_size,
                                         TextAlignment alignment = TextAlignment::START,
                                         const char* font_family = "Arial") :
        DrawInstructionBase(),
        text{text}, xy{xy}, font_size{font_size}, font_family{font_family},
        alignment{alignment}
    {
        fill_color = ColorType::ICON;
        stroke_color = ColorType::NONE;
        stroke_width = 1;
    }

    inline constexpr BoundingBox get_bbox() const noexcept {return BoundingBox{xy.x, xy.y, xy.x, xy.y};}

    Vector2 xy; /// Position of the text
    const char* text = ""; /// The actual contained text
    const char* font_family = "Arial";
    int font_size = 12; /// Font size
    FontWeight font_weight = FontWeight::BOLD;
    TextAlignment alignment = TextAlignment::MIDDLE;

    std::string get_svg_string(const Style& context) const noexcept;

    inline constexpr DrawInstructionText& with_font_weight(FontWeight weight) noexcept {
        this->font_weight = weight;
        return *this;
    }
};

// Forward declaration so transform objects can have children
struct DrawCommand;

/**
 * @brief Represents a translation of child elements
 */
struct DrawInstructionTranslate : public DrawInstructionBase<DrawInstructionTranslate> {
    inline constexpr DrawInstructionTranslate(const Vector2& delta) noexcept : DrawInstructionBase(), delta{delta} {}

    Vector2 delta;

    SVGString get_svg_string(const Style& context, const std::vector<DrawCommand>* children) const noexcept;
};

/**
 * @brief Represents a scaling of child elements
 */
struct DrawInstructionScale: public DrawInstructionBase<DrawInstructionScale> {
    inline constexpr DrawInstructionScale(real_t scale) noexcept : DrawInstructionBase(), scale{scale} {}

    real_t scale;

    SVGString get_svg_string(const Style& context, const std::vector<DrawCommand>* children) const noexcept;
};

/**
 * @brief Base draw command class, all constexpr
 */
struct DrawCommand {
    enum class Type {
        UNDEFINED = 0,
        PATH,
        CIRCLE,
        TEXT,
        TRANSLATE,
        SCALE,

        FULL_FRAME
    };

    std::vector<DrawCommand> children; /// Child commands (for transformations)


    /// Default constructor (monostate)
    inline constexpr DrawCommand() : variant{std::monostate{}} {}
    inline constexpr DrawCommand(const DrawCommand& other) noexcept : variant{other.variant}, children{other.children} {};
    inline constexpr DrawCommand& operator= (const DrawCommand& other) {
        variant = other.variant;
        children = other.children;
        return *this;
    }

    inline constexpr Type get_type() const noexcept {return static_cast<Type>(variant.index());}

    inline static constexpr DrawCommand path(const char* d) {
        DrawCommand ret;
        ret.variant = DrawInstructionPath{d, BoundingBox{}};
        return ret;
    }

    inline static constexpr DrawCommand path(const char* d, const BoundingBox& bbox) {
        DrawCommand ret;
        ret.variant = DrawInstructionPath{d, bbox};
        return ret;
    }

    template<typename... Args>
    inline static DrawCommand dynamic_path(const BoundingBox& bbox, Args&& ... args) {

        std::stringstream ss;

        // Fold expressions require C++20 or higher
        ([&]{
            ss << args;
        } (), ...);

        DrawCommand ret;
        _impl::DrawInstructionPath ret_cmd;
        ret_cmd.dynamic_path = std::move(ss.str());
        ret_cmd.bbox = bbox;
        ret.variant = std::move(ret_cmd);
        return ret;
    }

    inline static constexpr DrawCommand dynamic_path(std::string&& d, const BoundingBox& bbox) {
        DrawCommand ret;
        _impl::DrawInstructionPath ret_cmd;
        ret_cmd.dynamic_path = std::move(d);
        ret_cmd.bbox = bbox;
        ret.variant = std::move(ret_cmd);
        return ret;
    }

    /// Creates a translation command
    template<typename... Args>
    inline static constexpr DrawCommand translate(const Vector2& delta, Args... args) {
        DrawCommand ret;
        ret.variant = DrawInstructionTranslate{delta};
        ret.children = std::vector<DrawCommand>{args...};
        return ret;
    }

    /// Creates a scale command
    template<typename... Args>
    inline static constexpr DrawCommand scale(real_t scale, Args... args) {
        DrawCommand ret;
        ret.variant = DrawInstructionScale{scale};
        ret.children = std::vector<DrawCommand>{args...};
        return translate(Vector2{100 - scale*100, 100 - scale*100}, ret);
    }

    /// Creates a circle with the specified center and radius
    inline static constexpr DrawCommand circle(const Vector2& center, real_t radius) {
        DrawCommand ret;
        ret.variant = DrawInstructionCircle{center, radius};
        return ret;
    }

    /// Creates text at the specified position and with the specified font size
    inline static constexpr DrawCommand text(const char* text, const Vector2& pos,
                                             int font_size, FontWeight font_weight = FontWeight::NORMAL,
                                             TextAlignment alignment = TextAlignment::MIDDLE,
                                             const char* font_family = "Arial") {
        DrawCommand ret;
        ret.variant = DrawInstructionText{text, pos, font_size, alignment, font_family}.with_font_weight(font_weight);
        return ret;
    }

    /**
     * @brief autotext Automatically creates text with the appropriate size and spacing in the APP-6D octagon
     * @param text_contents The text to render
     * @param font_family The font family to use, defaults to "Arial"
     * @return
     */
    inline static constexpr DrawCommand autotext(const char* text_contents,
                                                 FontWeight weight = FontWeight::NORMAL,
                                                 TextAlignment alignment = TextAlignment::MIDDLE,
                                                 const char* font_family = "Arial") {
        DrawCommand ret;
        int size = 42;
        real_t y = 115;
        std::string_view strview{text_contents};
        if (strview.size() == 1) {
            size = 45;
            y = 115;
        } else if (strview.size() == 3) {
            size = 35;
            y = 110;
        } else if (strview.size() >= 4) {
            size = 32;
            y = 110;
        }

        return text(text_contents, Vector2{100, y}, size, weight, alignment, font_family);
    }

    /**
     * @brief autotext Automatically creates text with the appropriate size for modifier 1 and spacing in the APP-6D octagon
     * @param text_contents The text to render
     * @return
     */
    inline static constexpr DrawCommand textm1(const char* text_contents) {
        DrawCommand ret;
        int size = 30;
        std::string_view strview{text_contents};

        if (strview.size() == 3) {
            size = 25;
        } else if (strview.size() >= 4) {
            size = 22;
        }

        return text(text_contents, Vector2{100, 77}, size, FontWeight::NORMAL, TextAlignment::MIDDLE);
    }

    /**
     * @brief autotext Automatically creates text with the appropriate size for modifier 1 and spacing in the APP-6D octagon
     * @param text_contents The text to render
     * @return
     */
    inline static constexpr DrawCommand textm2(const char* text_contents) {
        DrawCommand ret;
        int size = 30;
        real_t y = 145;

        std::string_view strview{text_contents};

        if (strview.size() == 3) {
            size = 25;
            y = 140;
        } else if (strview.size() >= 4) {
            size = 20;
            y = 135;
        }

        return text(text_contents, Vector2{100, y}, size, FontWeight::NORMAL, TextAlignment::MIDDLE);
    }

    /**
     * Create a variant command for different affiliations
     */
    inline static constexpr DrawCommand full_frame(
        const DrawCommand& hostile,
        const DrawCommand& friendly,
        const DrawCommand& neutral,
        const DrawCommand& unknown
    ) {
        DrawCommand ret;
        ret.variant = AffiliationSet{hostile, friendly, neutral, unknown};
        return ret;
    }

    /**
     * @brief Returns an string containing an SVG representation of the draw command
     * @param style The style to use
     * @return
     */
    SVGString get_svg_string(const Style& style) const noexcept;

    /**
     * @brief Returns the bounding box of the draw command
     */
    constexpr BoundingBox get_bbox() const noexcept {
        switch(get_type()) {
        case Type::PATH:
            return std::get<DrawInstructionPath>(variant).bbox;
            break;
        case Type::CIRCLE:
            return std::get<DrawInstructionCircle>(variant).get_bbox();
            break;
        case Type::TEXT:
            return std::get<DrawInstructionText>(variant).get_bbox();
            break;
        case Type::TRANSLATE: {
            Vector2 delta = std::get<DrawInstructionTranslate>(variant).delta;
            BoundingBox box{0, 0, 0, 0};
            bool box_inited = false;
            for (const auto& item : children) {
                if (!box_inited) {
                    box = item.get_bbox();
                    box_inited = true;
                } else {
                    box.merge(item.get_bbox());
                }
            }
            return box.translated(delta);
            break;
        }
        case Type::SCALE: {
            float scale = std::get<DrawInstructionScale>(variant).scale;
            BoundingBox box{0, 0, 0, 0};
            bool box_inited = false;
            for (const auto& item : children) {
                if (!box_inited) {
                    box = item.get_bbox().scaled_to_center(scale);
                    box_inited = true;
                } else {
                    box.merge(item.get_bbox().scaled_to_center(scale));
                }
            }
            return box;
        }

        default:
        case Type::UNDEFINED:
            // Do nothing
            return {};
            break;
        }
    }

    /**
     * @brief Returns whether the command is defined and valid
     */
    inline constexpr bool is_defined() const noexcept {
        return get_type() != Type::UNDEFINED;
    }

    /*
     * Modifiers
     */
    inline constexpr DrawCommand& with_fill(ColorType use_fill) noexcept {
        switch(get_type()) {
        case Type::PATH:
            std::get<DrawInstructionPath>(variant).with_fill(use_fill);
            break;
        case Type::CIRCLE:
            std::get<DrawInstructionCircle>(variant).with_fill(use_fill);
            break;
        case Type::TEXT:
            std::get<DrawInstructionText>(variant).with_fill(use_fill);
            break;
        default:
            break;
        }
        return *this;
    }

    inline constexpr DrawCommand& with_stroke(ColorType use_stroke) noexcept {
        switch(get_type()) {
        case Type::PATH:
            std::get<DrawInstructionPath>(variant).with_stroke(use_stroke);
            break;
        case Type::CIRCLE:
            std::get<DrawInstructionCircle>(variant).with_stroke(use_stroke);
            break;
        case Type::TEXT:
            std::get<DrawInstructionText>(variant).with_stroke(use_stroke);
            break;
        default:
            break;
        }
        return *this;
    }

    inline constexpr DrawCommand& with_stroke_width(real_t width) noexcept {
        switch(get_type()) {
        case Type::PATH:
            std::get<DrawInstructionPath>(variant).with_stroke_width(width);
            break;
        case Type::CIRCLE:
            std::get<DrawInstructionCircle>(variant).with_stroke_width(width);
            break;
        case Type::TEXT:
            std::get<DrawInstructionText>(variant).with_stroke_width(width);
            break;
        default:
            break;
        }
        return *this;
    }

    inline constexpr DrawCommand& with_stroke_style(StrokeStyle style) noexcept {
        switch(get_type()) {
        case Type::PATH:
            std::get<DrawInstructionPath>(variant).with_stroke_style(style);
            break;
        case Type::CIRCLE:
            std::get<DrawInstructionCircle>(variant).with_stroke_style(style);
            break;
        case Type::TEXT:
            std::get<DrawInstructionText>(variant).with_stroke_style(style);
            break;
        default:
            break;
        }
        return *this;
    }

    /*
     * Const modifiers
     */
    inline constexpr DrawCommand copy_with_fill(ColorType use_fill) const noexcept {
        DrawCommand ret = *this;
        ret.with_fill(use_fill);
        return ret;
    }

    inline constexpr DrawCommand copy_with_stroke(ColorType stroke) const noexcept {
        DrawCommand ret = *this;
        ret.with_stroke(stroke);
        return ret;
    }

    inline constexpr DrawCommand copy_with_stroke_width(real_t stroke_width) const noexcept {
        DrawCommand ret = *this;
        ret.with_stroke_width(stroke_width);
        return ret;
    }

    inline constexpr DrawCommand copy_with_stroke_style(StrokeStyle style) const noexcept {
        DrawCommand ret = *this;
        ret.with_stroke_style(style);
        return ret;
    }

private:
    using AffiliationSet = std::vector<DrawCommand>;

    using variant_t = std::variant<std::monostate,
                                   DrawInstructionPath,
                                   DrawInstructionCircle,
                                   DrawInstructionText,
                                   DrawInstructionTranslate,
                                   DrawInstructionScale,

                                   AffiliationSet>;
    variant_t variant;
};

/**
 * @brief Layer representing a specific symbol and all its associated draw
 * commands.
 */
struct SymbolLayer {
    inline constexpr SymbolLayer() noexcept {}; /// Default constructor
    inline constexpr SymbolLayer(const SymbolLayer& other) noexcept : draw_items{other.draw_items}, civilian_override{other.civilian_override} {}
    inline constexpr SymbolLayer& operator=(const SymbolLayer& other) noexcept {
        draw_items = other.draw_items;
        civilian_override = other.civilian_override;
        return *this;
    }

    template<typename... Args>
    inline constexpr SymbolLayer(Args... args) noexcept : draw_items{args...} {}

    std::vector<DrawCommand> draw_items;
    bool civilian_override = false;

    inline constexpr SymbolLayer& with_civilian_override(bool override) noexcept {
        civilian_override = override;
        return *this;
    }

    inline constexpr BoundingBox get_bbox() const noexcept {
        if (draw_items.empty()) {
            return {};
        }

        bool first = true;
        BoundingBox ret;
        for (const DrawCommand& cmd : draw_items) {
            if (first) {
                ret = cmd.get_bbox();
                first = false;
                continue;
            }

            ret = ret.merge(cmd.get_bbox());
        }
        return ret;
    }

    inline constexpr SymbolLayer& with_fill(const ColorType color) noexcept {
        for (auto& cmd : draw_items) {
            cmd.with_fill(color);
        }
        return *this;
    }

    inline constexpr SymbolLayer& with_stroke(const ColorType color) noexcept {
        for (auto& cmd : draw_items) {
            cmd.with_stroke(color);
        }
        return *this;
    }

    inline constexpr SymbolLayer copy_with_stroke_width(real_t stroke_width) const noexcept {
        SymbolLayer ret{*this};
        for (int i = 0; i < draw_items.size(); i++) {
            ret.draw_items[i] = draw_items[i].copy_with_stroke_width(stroke_width);
        }
        return ret;
    }
};

}
