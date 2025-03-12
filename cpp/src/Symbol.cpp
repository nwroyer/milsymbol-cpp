#include "Symbol.hpp"
#include "Schema.hpp"
#include <iomanip>

namespace milsymbol {

class DrawInstruction {
    enum class Type {
        PATH = 0,
        CIRCLE,
        TEXT,
        TRANSLATE,
        ROTATE,
        SCALE
    };
};

static constexpr int SIDC_VERSION = 0x13;

static constexpr real_t get_task_force_width(Amplifier amplifier) {
    return 90;

    // switch(echelon) {
    // case Echelon::CORPS:
    //     return 110;
    // case Echelon::ARMY:
    //     return 145;
    // case Echelon::ARMY_GROUP:
    //     return 180;
    // case Echelon::REGION:
    //     return 215;
    // default:
    //     return 90;
    // }
}

_impl::DrawCommand get_symbol_headquarters(Affiliation affiliation, FrameShape frame_shape,
    real_t hq_staff_length, const BoundingBox& base_bbox, real_t frame_stroke_width,
    Vector2& staff_base)

{
    real_t y = 100;
    // dimension = _impl::get_base_dimension(dimension);

    // For air and ground friendly/neutral, and sea/subsurface neutral icons, we start the HQ staff
    // at the bottom left corner, so we adjust the starting point accordingly.
    if (
        (frame_shape == FrameShape::AIR && (affiliation == Affiliation::FRIEND || affiliation == Affiliation::NEUTRAL)) ||
        (frame_shape == FrameShape::LAND_UNIT && (affiliation == Affiliation::FRIEND || affiliation == Affiliation::NEUTRAL)) ||
        (frame_shape == FrameShape::LAND_EQUIPMENT_AND_SEA_SURFACE && affiliation == Affiliation::NEUTRAL)
        ) {
        y = base_bbox.y2;
    }

    // For friendly subsurface units, we start at the upper-left corner
    if (frame_shape == FrameShape::SEA_SUBSURFACE && affiliation == Affiliation::FRIEND) {
        y = base_bbox.y1;
    }

    // Construct the geometry
    BoundingBox hq_box = BoundingBox{base_bbox.x1, y, base_bbox.x1, base_bbox.y2 + hq_staff_length};

    staff_base = Vector2{base_bbox.x1, (base_bbox.y2 + hq_staff_length)};
    std::stringstream ss;
    ss << "M" << base_bbox.x1 << "," << y << " L" << staff_base.x << "," << staff_base.y;

    return _impl::DrawCommand::dynamic_path(std::move(ss.str()), hq_box).with_stroke_width(frame_stroke_width);
}

static void get_status_modifiers(const Symbol& symbol, const BoundingBox& bbox, std::vector<_impl::DrawCommand>& out) {
    static constexpr std::array<Color, 4> CONDITION_COLORS = {
        Color{0, 255, 0}, // Fully capable
        Color{255, 255, 0}, // Damaged
        Color{255, 0, 0}, // Destroyed
        Color{0, 180, 240} // Full to capacity
    };

    if (symbol.get_status() != Status::PRESENT) {
        // @TODO handle status modifiers
    }
}

static BoundingBox apply_amplifiers(const SymbolStyle& style,
                                    const Symbol& symbol,
                                    const BoundingBox& base_bbox_raw,
                                    std::vector<_impl::DrawCommand>& out,
                                    Vector2& staff_base) {
    BoundingBox base_bbox = base_bbox_raw;
    BoundingBox modifier_bbox = base_bbox;
    FrameShape frame_shape = symbol.get_used_frame_shape(style);

    /*
     * Apply headquarters staff
     */

    if (symbol.is_headquarters()) {
        _impl::DrawCommand cmd = get_symbol_headquarters(symbol.get_affiliation(),
                                                         frame_shape,
                                                         style.hq_staff_length,
                                                         base_bbox,
                                                         style.frame_stroke_width,
                                                         staff_base);
        modifier_bbox.merge(cmd.get_bbox(symbol.get_affiliation()));
        out.push_back(cmd);
    }

    /*
     * Handle task force indicators
     */

    if (symbol.is_task_force()) {
        // real_t width = get_task_force_width(symbol.get_echelon());
        // BoundingBox tf_bbox{100 - width / 2, base_bbox.y1 - 40, 100 + width/2, base_bbox.y1};

        // // Construct the path
        // std::stringstream ss;
        // ss << "M" << (100 - width/2) << "," << base_bbox.y1 << " L" << (100 - width/2) <<
        //     "," << (base_bbox.y1 - 40) << " " << (100 + width/2) << "," << (base_bbox.y1 - 40) <<
        //     " " << (100 + width/2) << "," << base_bbox.y1;
        // out.push_back(_impl::DrawCommand::dynamic_path(std::move(ss.str()),
        //                                                modifier_bbox).with_stroke_width(style.frame_stroke_width));
        // modifier_bbox.merge(tf_bbox);
    }

    /*
     * Handle feint/dummy
     */

    if (symbol.is_feint_or_dummy()) {
        real_t top_point = base_bbox.y1 - 0 - base_bbox.width() / 2;

        std::stringstream ss;
        ss << "M100," <<
            top_point <<
            " L" <<
            base_bbox.x1 <<
            "," <<
            (base_bbox.y1 - 0) <<
            " M100," <<
            top_point <<
            " L" <<
            base_bbox.x2 <<
            "," <<
            (base_bbox.y1 - 0);

        BoundingBox cmd_bbox{base_bbox.x1, top_point, base_bbox.x2, base_bbox.y1};
        out.push_back(_impl::DrawCommand::dynamic_path(std::move(ss.str()), cmd_bbox)
                          .with_stroke_style(_impl::StrokeStyle::DASHED));
        modifier_bbox.merge(cmd_bbox);
    }

    _impl::SymbolLayer ret = _impl::get_amplifier_layer(symbol.get_amplifier(), symbol.get_affiliation(), frame_shape);
    for (const auto& item : ret.draw_items) {
        out.emplace_back(item);
        base_bbox.merge(item.get_bbox(symbol.get_affiliation()));
    }

    base_bbox.merge(modifier_bbox);
    return base_bbox;
}

static void apply_context(Context context, Affiliation affil, Dimension dim, const BoundingBox& bbox, std::vector<_impl::DrawCommand>& out) {
    using namespace _impl;

    real_t spacing = 10;
    if (affil == Affiliation::UNKNOWN || (affil == Affiliation::HOSTILE && dim == Dimension::SEA_SUBSURFACE)) {
        spacing = -10;
    }

    if (context != Context::REALITY) {
        bool joker = (affil == Affiliation::SUSPECT);
        bool faker = (affil == Affiliation::ASSUMED_FRIEND);

        out.push_back(DrawCommand::text(
            context == Context::EXERCISE ? (joker ? "J" : (faker ? "K" : "X")) : "S",
            Vector2{bbox.x2 + spacing, 60}, 35, FontWeight::BOLD, TextAlignment::START));
    }

}

static int int_substring(const std::string_view& view, int start, int len) {
    int ret = 0;
    std::from_chars(&view[start], &view[start + len], ret);
    return ret;
}

Symbol Symbol::from_sidc(const std::string& sidc_raw) noexcept {

    /*
     * The SIDC is a 30-position string, with each character being a hexadecimal character (0-9, A-F).
     *
     * The first 20 digits are the same as the MIL-STD-2525D codes.
     *
     * - [0-1]:   SIDC version
     * - [2]:     Context
     * - [4]:     Affiliation
     * - [4-5]:   Symbol set
     * - [6]:     Status
     * - [7]:     Task force / headquarters / dummy
     * - [8-9]:   Amplifier / descriptor
     * - [10-11]: Entity
     * - [12-13]: Entity type
     * - [14-15]: Entity subtype
     * - [16-17]: Sector 1 modifier
     * - [18-19]: Sector 2 modifier
     *
     *  The following 10 digits are new for MIL-STD-2525E.
     *
     * - [20]:    Sector 1 common identity modifier
     * - [21]:    Sector 2 common identity modifier
     * - [22]:    Frame shape
     * - [23-26]: Reserved for future use
     * - [27-29]: Nationality / country / geopolitical identifier
     */

    std::string ret = sidc_raw + '\0';
    std::string_view sidc = ret;

    if (sidc_raw.length() < 2) {
        std::cerr << "SIDC \"" << sidc << "\" must be at least 20 characters; can't determine version" << std::endl;
        return {};
    }

    // Determine version
    int version = int_substring(sidc, 0, 2);
    if (version != 13) {
        std::cerr << "Warning: SIDC versions not equal to 13 (MIL-STD-2525E) may not be dealt with appropriately (version is " <<
            version << " -> " << version << ")" << std::endl;
    }

    Symbol symbol;

    /*
     * Parse standard identity
     */


    // Part 1: Parse standard identity
    symbol.context = _impl::sidc_to_context(sidc.substr(2, 1));
    symbol.affiliation = _impl::sidc_to_affiliation(sidc.substr(3, 1));
    symbol.symbol_set = _impl::sidc_to_symbol_set(sidc.substr(4, 2));
    symbol.status = _impl::sidc_to_status(sidc.substr(4, 2));
    symbol.headquarters = _impl::sidc_to_headquarters(sidc.substr(7, 1));
    symbol.task_force = _impl::sidc_to_task_force(sidc.substr(7, 1));
    symbol.task_force = _impl::sidc_to_task_force(sidc.substr(7, 1));
    symbol.amplifier = _impl::sidc_to_amplifier(sidc.substr(8, 2));
    symbol.entity = _impl::sidc_to_entity(symbol.symbol_set, _impl::hex_from_substring(sidc.substr(10, 6)));

    // Determine whether we're using common modifiers
    bool common_mod_1 = sidc.length() >= 30 ? (_impl::hex_from_substring(sidc.substr(20, 1)) != 0) : false;
    bool common_mod_2 = sidc.length() >= 30 ? (_impl::hex_from_substring(sidc.substr(21, 1)) != 0) : false;

    if (sidc.length() >= 30) {
        symbol.set_frame_shape_override(_impl::sidc_to_frame_shape(sidc.substr(22, 1)));
    }

    // Execute on the common modifier
    symbol.modifier_1 = _impl::sidc_to_modifier_1(common_mod_1 ? SymbolSet::COMMON_MODIFIERS : symbol.symbol_set,
                                                  (common_mod_1 ? 0x100 : 0) + _impl::hex_from_substring(sidc.substr(16, 2)));
    symbol.modifier_2 = _impl::sidc_to_modifier_2(common_mod_2 ? SymbolSet::COMMON_MODIFIERS : symbol.symbol_set,
                                                  (common_mod_2 ? 0x100 : 0) + _impl::hex_from_substring(sidc.substr(18, 2)));

    return symbol;
}

template<typename T>
static void append_to_ss(std::stringstream& ss, const T& item, int width) {
    ss << std::hex << std::setw(width) << std::setfill('0') << static_cast<int>(item);
}

std::string Symbol::to_sidc() const noexcept {
    std::stringstream ss;
    append_to_ss(ss, SIDC_VERSION, 2);
    append_to_ss(ss, context, 1);
    append_to_ss(ss, affiliation, 1);
    append_to_ss(ss, symbol_set, 2);
    append_to_ss(ss, status, 1);
    append_to_ss(ss, 0, 1);
    append_to_ss(ss, amplifier, 2);
    append_to_ss(ss, entity & 0xFFFFFF, 6);
    append_to_ss(ss, modifier_1 & 0xFF, 2);
    append_to_ss(ss, modifier_2 & 0xFF, 2);
    append_to_ss(ss, _impl::is_modifier_1_common(modifier_1) ? 1 : 0, 1);
    append_to_ss(ss, _impl::is_modifier_2_common(modifier_2) ? 1 : 0, 1);
    append_to_ss(ss, frame_shape_override, 1);
    append_to_ss(ss, 0, 4); // Reserved for future use
    append_to_ss(ss, 0, 3); // Country code
    return ss.str();
}

Symbol::modifier_t Symbol::get_modifier(int mod) const noexcept {
    if (mod < 1 || mod > 2) {
        std::cerr << "No modifier set " << mod << std::endl;
        return 0;
    }

    return mod == 1 ? static_cast<modifier_t>(modifier_1) : static_cast<modifier_t>(modifier_2);
}

inline Vector2 scaled_to_center(const Vector2& vec, float scale) noexcept {
    return Vector2{100 +(vec.x - 100) * scale, 100 + (vec.y - 100) * scale};
}

Symbol::RichOutput Symbol::get_svg(const SymbolStyle& style) const noexcept {
    using namespace _impl;
    static constexpr const char* SVG_NS = "http://w3.org/2000/svg";

    SymbolSet symbol_set = get_symbol_set();

    _impl::SymbolLayer symbol_layer = _impl::get_symbol_layer(symbol_set, entity, IconType::ENTITY);
    _impl::SymbolLayer m1_layer = _impl::get_symbol_layer(_impl::is_modifier_1_common(modifier_1) ? SymbolSet::COMMON_MODIFIERS : symbol_set, modifier_1, IconType::MODIFIER_1);
    _impl::SymbolLayer m2_layer = _impl::get_symbol_layer(_impl::is_modifier_2_common(modifier_2) ? SymbolSet::COMMON_MODIFIERS : symbol_set, modifier_2, IconType::MODIFIER_2);

    // Add the base geometry
    std::vector<_impl::DrawCommand> components;
    bool use_civilian_color = false;
    if (symbol_layer.civilian_override || m1_layer.civilian_override || m2_layer.civilian_override) {
        use_civilian_color = true;
    }

    // Get base symbol_geometry
    BoundingBox base_bbox{100, 100, 100, 100};
    FrameShape used_frame_shape = get_used_frame_shape(style);
    SymbolLayer base = get_base_symbol_geometry(used_frame_shape, get_frame_affiliation(affiliation, context));

    base_bbox = base.get_bbox(affiliation);

    if (style.use_frame || style.is_position_only()) {

        // Get base symbol
        // Set the width of the frame
        base.draw_items[0].with_stroke_width(style.frame_stroke_width);

        // Handle unfilled icons
        if (style.color_mode == ColorMode::UNFILLED) {
            base.draw_items[0].with_fill(ColorType::NONE);
            for (int i = 1; i < base.draw_items.size(); i++) {
                base.draw_items[i].with_fill(ColorType::ICON);
            }
        }

        bool dashed_frame = (is_affiliation_dashed(affiliation) || is_status_dashed(status));

        if (dashed_frame) {
            // Apply dashed frame base
            base.draw_items[0].with_stroke(ColorType::WHITE);
        }

        for (const auto& cmd : base.draw_items) {
            components.push_back(cmd);
        }

        if (dashed_frame) {
            // Apply dashed frame
            DrawCommand copy = base.draw_items[0].copy_with_stroke(ColorType::ICON).with_stroke_style(StrokeStyle::DASHED).with_fill(ColorType::NONE);
            components.push_back(copy);
        }

        base_bbox = base.get_bbox(affiliation);
    }

    // Handle various graphical modifiers
    if (!style.is_position_only() && style.use_amplifiers) {
        apply_context(context, affiliation, dimension_from_symbol_set(symbol_set), base_bbox, components);
    }

    /*
     * Convert the commands to SVG
     */

    // Initialize the bounding box
    bool bbox_initialized = false;
    BoundingBox bbox = style.use_frame ? BoundingBox{} : base_bbox;
    for (const auto& comp : components) {
        // Expand the bounding box
        if (!bbox_initialized) {
            bbox = comp.get_bbox(affiliation);
            bbox_initialized = true;
        } else {
            bbox.merge(comp.get_bbox(affiliation));
        }
    }

    /*
     * Apply modifiers
     */

    Vector2 hq_staff_base;
    if (!style.is_position_only() && style.use_amplifiers) {
        bbox.merge(apply_amplifiers(style, *this, bbox, components, hq_staff_base));
    }

    //    bbox_initialized = false;
    for (const auto& comp : components) {
        bbox.merge(comp.get_bbox(affiliation));
    }

    // Add entity
    if (style.use_entity_icon) {
        for (const auto& cmd : symbol_layer.draw_items) {
            components.push_back(cmd);
        }
    }

    // Add modifiers
    if (!style.is_position_only() && style.use_entity_icon && style.use_modifiers) {
        for (const auto& cmd : m1_layer.draw_items) {
            components.push_back(cmd);
        }
        for (const auto& cmd : m2_layer.draw_items) {
            components.push_back(cmd);
        }
    }

    // Handle non-default sizes
    if (style.has_non_default_size()) {
        std::vector<DrawCommand> interior_components = std::move(components);
        components = std::vector<DrawCommand>();
        components.push_back(DrawCommand::scale(style.get_icon_internal_scale_factor(), std::move(interior_components)));
    }

    // Execute the context
    _impl::Style context;
    context.affiliation = affiliation;
    context.civilian = use_civilian_color && style.use_civilian_color;
    context.color_mode = style.color_mode;
    context.use_color_override = style.use_color_override;
    context.color_override = style.color_override;
    context.stroke_width_override = (style.uses_stroke_width_override() ? style.get_stroke_width_override() : -1);

    std::stringstream ss;
    for (const auto& comp : components) {
        ss << comp.get_svg_string(context) << std::endl;
    }

    // ss << "<!-- BBox: " << bbox.x1 << ", " << bbox.y1 << " to " << bbox.x2 << ", " << bbox.y2 << " -->" << std::endl;

    // Create the svg

    RichOutput result;
    result.svg_bounding_box = BoundingBox{
        bbox.x1 - style.frame_stroke_width - style.padding,
        bbox.y1 - style.frame_stroke_width - style.padding,
        bbox.x2 + style.frame_stroke_width + style.padding,
        bbox.y2 + style.frame_stroke_width + style.padding
    };

    if (style.has_non_default_size()) {
        result.svg_bounding_box = result.svg_bounding_box.scaled_to_center(style.get_icon_internal_scale_factor());
    }

    std::stringstream ret_stream;
    ret_stream << "<svg width=\"" << result.svg_bounding_box.width() << "\" " <<
        "height=\"" << result.svg_bounding_box.height() << "\" " <<
        "viewBox=\"" << result.svg_bounding_box.x1 << " " <<
        result.svg_bounding_box.y1 << " " <<
        result.svg_bounding_box.width() << " " <<
        result.svg_bounding_box.height() << "\" " <<
        ">" << std::endl;
    ret_stream << ss.str();
    ret_stream << "</svg>";

    result.svg = ret_stream.str();

    // Offset the frame bounding box
    result.frame_bounding_box = base_bbox;
    result.frame_bounding_box.x1 -= result.svg_bounding_box.x1;
    result.frame_bounding_box.x2 -= result.svg_bounding_box.x1;
    result.frame_bounding_box.y1 -= result.svg_bounding_box.y1;
    result.frame_bounding_box.y2 -= result.svg_bounding_box.y1;

    if (headquarters) {
        // Tip of the staff
        result.symbol_anchor = scaled_to_center(hq_staff_base, style.get_icon_internal_scale_factor()) - result.svg_bounding_box.point_1();
    } else {
        // Center of the frame
        result.symbol_anchor = scaled_to_center(Vector2{100, 100}, style.get_icon_internal_scale_factor()) - result.svg_bounding_box.point_1();
    }

    return result;
}

#ifdef MILSYMBOL_HAS_SYMBOL_ENUMERATORS
std::vector<Symbol::entity_t> Symbol::get_all_entities(SymbolSet symbol_set) noexcept {
    return _impl::get_available_symbols(symbol_set, _impl::IconType::ENTITY);
}

std::vector<Symbol::entity_t> Symbol::get_all_modifier_1s(SymbolSet symbol_set) noexcept {
    return _impl::get_available_symbols(symbol_set, _impl::IconType::MODIFIER_1);
}

std::vector<Symbol::entity_t> Symbol::get_all_modifier_2s(SymbolSet symbol_set) noexcept {
    return _impl::get_available_symbols(symbol_set, _impl::IconType::MODIFIER_2);
}
#endif

std::vector<Symbol::entity_t> Symbol::get_all_symbol_sets() noexcept {
    std::vector<Symbol::entity_t> ret;
    ret.reserve(SYMBOL_SETS.size());
    for (SymbolSet sym_set : SYMBOL_SETS) {
        ret.push_back(static_cast<entity_t>(sym_set));
    }
    return ret;
}

}
