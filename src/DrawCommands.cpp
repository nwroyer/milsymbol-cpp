#include "DrawCommands.hpp"
#include "eternal.hpp"
#include <iostream>

namespace milsymbol::_impl {

static constexpr const char* SOLID_STYLE = "0";
static constexpr const char* DASH_STYLE = "8 8";
static constexpr const char* DOT_STYLE = "1 1";

inline static constexpr const char* dash_style_to_SVG(StrokeStyle style) {
    switch(style) {
    case StrokeStyle::DASHED:
        return DASH_STYLE;
    default:
        return SOLID_STYLE;
    }
}

/// Returns a string representation rgb(r, g, b) of this color, or "none"
static std::string color_to_string(const Color& color) noexcept {
    std::stringstream ss;
    if (color.r < 0) {
        return "none";
    }

    ss << "rgb(" << color.r << "," << color.g << "," << color.b << ")";
    return ss.str();
}

SVGString DrawInstructionPath::get_svg_string(const Style& context) const noexcept {
    std::stringstream ss;
    ss << "<path ";
    ss << "fill=\"" << color_to_string(context.get_color(fill_color)) << "\" ";
    ss << "stroke=\"" << color_to_string(context.get_color(stroke_color)) << "\" " <<
        "d = \"" << (dynamic_path.empty() ? d : dynamic_path) << "\" ";

    ss << "stroke-width=\"" << (context.stroke_width_override >= 0 ? context.stroke_width_override : stroke_width) << "\" ";

    if (stroke_color != ColorType::NONE && stroke_style != StrokeStyle::SOLID) {
        ss << "stroke-dasharray=\"" << dash_style_to_SVG(stroke_style) << "\" ";
    }

    ss << "/>";
    return ss.str();
}

SVGString DrawInstructionCircle::get_svg_string(const Style& context) const noexcept {
    std::stringstream ss;
    ss << "<circle cx=\"" << center.x << "\" cy=\"" << center.y << "\" r=\"" << radius << "\" fill=\"" <<
        color_to_string(context.get_color(fill_color)) << "\" stroke=\"" <<
        color_to_string(context.get_color(stroke_color)) << "\" ";
    ss << "stroke-width=\"" << (context.stroke_width_override >= 0 ? context.stroke_width_override : stroke_width) << "\"";

    if (stroke_color != ColorType::NONE && stroke_style != StrokeStyle::SOLID) {
        ss << "stroke-dasharray=\"" << dash_style_to_SVG(stroke_style) << "\" ";
    }

    ss << "/>";
    return ss.str();
}

SVGString DrawInstructionText::get_svg_string(const Style& context) const noexcept {
    std::stringstream ss;
    ss << "<text x=\"" << xy.x << "\" y=\"" << xy.y << "\" ";

    // Fill and stroke
    ss << "fill=\"" << color_to_string(context.get_color(fill_color)) << "\" ";
    ss << "stroke=\"" << color_to_string(context.get_color(stroke_color)) << "\" ";

    if (stroke_color != ColorType::NONE) {
        ss << "stroke-width=\"" << (context.stroke_width_override >= 0 ? context.stroke_width_override : stroke_width) << "\" ";

        if (stroke_style != StrokeStyle::SOLID) {
            ss << "stroke-dasharray=\"" << dash_style_to_SVG(stroke_style) << "\" ";
        }
    }

    ss << "font-size=\"" << font_size << "\" ";

    // Font weight
    ss << "font-family=\"Arial\" ";
    ss << "font-weight=\"" << get_font_weight_string(font_weight) << "\" ";
    ss << "text-anchor=\"" << get_font_alignment_string(alignment) << "\" ";

    ss << ">" << text << "</text>";
    return ss.str();
}

SVGString DrawInstructionTranslate::get_svg_string(const Style& context, const std::vector<DrawCommand>* children) const noexcept {
    std::stringstream ss;
    ss << "<g transform=\"translate(" << delta.x << " " << delta.y << ")\">";
    for (const auto& child : *children) {
        ss << child.get_svg_string(context);
    }
    ss << "</g>";
    return ss.str();
}

SVGString DrawInstructionScale::get_svg_string(const Style& context, const std::vector<DrawCommand>* children) const noexcept {
    std::stringstream ss;
    ss << "<g transform=\"scale(" << scale << ")\">";
    for (const auto& child : *children) {
        ss << child.get_svg_string(context);
    }
    ss << "</g>";
    return ss.str();
}

SVGString DrawCommand::get_svg_string(const Style& context) const noexcept {

    switch(get_type()) {
    case Type::PATH:
        return std::get<DrawInstructionPath>(variant).get_svg_string(context);
        break;
    case Type::CIRCLE:
        return std::get<DrawInstructionCircle>(variant).get_svg_string(context);
        break;
    case Type::TEXT:
        return std::get<DrawInstructionText>(variant).get_svg_string(context);
        break;
    case Type::TRANSLATE:
        return std::get<DrawInstructionTranslate>(variant).get_svg_string(context, &children);
        break;
    case Type::SCALE:
        return std::get<DrawInstructionScale>(variant).get_svg_string(context, &children);
        break;
    case Type::FULL_FRAME: {
        std::stringstream ss;
        return std::get<AffiliationSet>(variant)[static_cast<int>(context.affiliation)].get_svg_string(context) + SVGString{""};
        break;
    }
    default:
    case Type::UNDEFINED:
        // Do nothing
        return {};
        break;
    }
}

}
