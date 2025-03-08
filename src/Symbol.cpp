#include "Symbol.hpp"
#include <charconv>

#include "Schema.hpp"
#include "SymbolGeometries.hpp"

namespace milsymbol {

static constexpr Dimension dimension_from_symbol_set(SymbolSet set) noexcept {
    switch(set) {
    case SymbolSet::AIR:
    case SymbolSet::AIR_MISSILE:
        return Dimension::AIR;
    case SymbolSet::SPACE:
    case SymbolSet::SPACE_MISSILE:
        return Dimension::SPACE;
    case SymbolSet::LAND_UNIT:
    case SymbolSet::LAND_CIVILIAN_UNIT_ORGANIZATION:
    case SymbolSet::LAND_INSTALLATION:
    case SymbolSet::ACTIVITIES:
        return Dimension::LAND;
    case SymbolSet::SEA_SURFACE:
    case SymbolSet::LAND_EQUIPMENT:
        return Dimension::SEA;
    case SymbolSet::SEA_SUBSURFACE:
        return Dimension::SUBSURFACE;
    default:
        return Dimension::LAND;
    }
}

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

static constexpr real_t get_task_force_width(Echelon echelon) {
    switch(echelon) {
    case Echelon::CORPS:
        return 110;
    case Echelon::ARMY:
        return 145;
    case Echelon::ARMY_GROUP:
        return 180;
    case Echelon::REGION:
        return 215;
    default:
        return 90;
    }
}

/*
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
*/
static void get_echelon_layer(bool installation, const BoundingBox& bbox, Echelon echelon, std::vector<_impl::DrawCommand>& out_items) {
    if (echelon == Echelon::UNDEFINED) {
        return;
    }

    real_t padding = installation ? 15 : 0;
    using namespace _impl;

    std::vector<_impl::DrawCommand> out;

    if (echelon == Echelon::TEAM) {
        out.push_back(DrawCommand::circle(Vector2{100, bbox.y1 - 20}, 15));
        std::stringstream ss;
        ss << "M80," << (bbox.y1 - 10) << "L120," << (bbox.y1 - 30);
        BoundingBox cmd_bbox;
        cmd_bbox.y1 = bbox.y1 - 40 - installation;
        out.push_back(DrawCommand::dynamic_path(std::move(ss.str()), cmd_bbox));
    } else if (echelon == Echelon::SQUAD) {
        out.push_back(DrawCommand::circle(Vector2(100, bbox.y1 - 20), 7.5).with_fill(ColorType::ICON));
    } else if (echelon == Echelon::SECTION) {
        out.push_back(DrawCommand::circle(Vector2{115, bbox.y1 - 20}, 7.5).with_fill(ColorType::ICON));
        out.push_back(DrawCommand::circle(Vector2{85, bbox.y1 - 20}, 7.5).with_fill(ColorType::ICON));
    } else if (echelon == Echelon::PLATOON) {
        out.push_back(DrawCommand::circle(Vector2{100, bbox.y1 - 20}, 7.5).with_fill(ColorType::ICON));
        out.push_back(DrawCommand::circle(Vector2{70, bbox.y1 - 20}, 7.5).with_fill(ColorType::ICON));
        out.push_back(DrawCommand::circle(Vector2{130, bbox.y1 - 20}, 7.5).with_fill(ColorType::ICON));
    } else if (echelon == Echelon::COMPANY) {
        std::stringstream ss;
        ss << "M100," << (bbox.y1 - 10) << "L100," << (bbox.y1 - 35);
        out.push_back(DrawCommand::dynamic_path(std::move(ss.str()), bbox.with_y1(bbox.y1 - 40 - padding)));
    } else if (echelon == Echelon::BATTALION) {
        out.push_back(DrawCommand::dynamic_path(bbox.with_y1(bbox.y1 - 40 - padding),
                                                "M90,", (bbox.y1 - 10), "L90,", (bbox.y1 - 35)));
        out.push_back(DrawCommand::dynamic_path(bbox.with_y1(bbox.y1 - 40 - padding),
                                                "M110,", (bbox.y1 - 10), "L110,", (bbox.y1 - 35)));
    } else if (echelon == Echelon::REGIMENT) {
        out.push_back(DrawCommand::dynamic_path(bbox.with_y1(bbox.y1 - 40 - padding),
                                                "M80,", (bbox.y1 - 10), "L80,", (bbox.y1 - 35)));
        out.push_back(DrawCommand::dynamic_path(bbox.with_y1(bbox.y1 - 40 - padding),
                                                "M100,", (bbox.y1 - 10), "L100,", (bbox.y1 - 35)));
        out.push_back(DrawCommand::dynamic_path(bbox.with_y1(bbox.y1 - 40 - padding),
                                                "M120,", (bbox.y1 - 10), "L120,", (bbox.y1 - 35)));
    } else if (echelon == Echelon::BRIGADE) {
        out.push_back(DrawCommand::dynamic_path(bbox.with_y1(bbox.y1 - 40 - padding),
                                                "M87.5,", (bbox.y1 - 10), " l25,-25 m0,25 l-25,-25"));
    } else if (echelon == Echelon::DIVISION) {
        out.push_back(DrawCommand::dynamic_path(BoundingBox{70, bbox.y1 - 40 - padding, 130, bbox.y1},
                                                "M70,",
                                                (bbox.y1 - 10),
                                                " l25,-25 m0,25 l-25,-25   M105,",
                                                (bbox.y1 - 10),
                                                " l25,-25 m0,25 l-25,-25"));
    } else if (echelon == Echelon::CORPS) {
        out.push_back(DrawCommand::dynamic_path(BoundingBox{52.5, bbox.y1 - 40 - padding, 147.5, bbox.y1},
                                                "M52.5,",
                                                (bbox.y1 - 10),
                                                " l25,-25 m0,25 l-25,-25 M87.5,",
                                                (bbox.y1 - 10),
                                                " l25,-25 m0,25 l-25,-25 M122.5,",
                                                (bbox.y1 - 10),
                                                " l25,-25 m0,25 l-25,-25"
        ));
    } else if (echelon == Echelon::ARMY) {
        out.push_back(DrawCommand::dynamic_path(BoundingBox{35, bbox.y1 - 40 - padding, 165, bbox.y1},
              "M35,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25   M70,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25   M105,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25    M140,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25"));
    } else if (echelon == Echelon::ARMY_GROUP) {
        out.push_back(DrawCommand::dynamic_path(BoundingBox{17.5, bbox.y1 - 40 - padding, 182.5, bbox.y1},
              "M17.5,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25    M52.5,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25    M87.5,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25    M122.5,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25       M157.5,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25"));
    } else if (echelon == Echelon::REGION) {
        out.push_back(DrawCommand::dynamic_path(BoundingBox{0, bbox.y1 - 40 - padding, 200, bbox.y1},
              "M0,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25   M35,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25   M70,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25   M105,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25    M140,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25     M175,",
              (bbox.y1 - 10),
              " l25,-25 m0,25 l-25,-25"));
    } else if (echelon == Echelon::COMMAND) {
        out.push_back(DrawCommand::dynamic_path(BoundingBox{70, bbox.y1 - 40 - padding, 130, bbox.y1},
            "M70,",
            (bbox.y1 - 22.5),
            " l25,0 m-12.5,12.5 l0,-25   M105,",
            (bbox.y1 - 22.5),
            " l25,0 m-12.5,12.5 l0,-25"));
    }

    out_items.push_back(DrawCommand::translate(Vector2{0, - padding}, out));
}

static BoundingBox get_mobility_layer(bool installation, const BoundingBox& base_bbox, Mobility mob, Affiliation affiliation, std::vector<_impl::DrawCommand>& out_items) {
    using namespace _impl;

    if (mob == Mobility::UNDEFINED) {
        return base_bbox;
    }

    /*
     * Calculate padding
     */

    BoundingBox bbox = base_bbox;

    if (affiliation == Affiliation::NEUTRAL) {
        if (mob == Mobility::TOWED || mob == Mobility::SHORT_TOWED_ARRAY || mob == Mobility::LONG_TOWED_ARRAY) {
            bbox = bbox.with_y2(bbox.y2 + 8);
        }
        else if (mob == Mobility::OVER_SNOW || mob == Mobility::SLED) {
            bbox = bbox.with_y2(bbox.y2 + 13);
        }
    }

    /*
     * Calculate geometry
     */
    std::vector<_impl::DrawCommand> out;
    if (mob == Mobility::WHEELED) {
        out.push_back(DrawCommand::path("M 53,1 l 94, 0", BoundingBox{53, 1, 94, 1}));
        out.push_back(DrawCommand::circle(Vector2{58, 8}, 8));
        out.push_back(DrawCommand::circle(Vector2{142, 8}, 8));
    } else if (mob == Mobility::WHEELED_CROSS_COUNTRY) {
        out.push_back(DrawCommand::path("M 53,1 l 94, 0", BoundingBox{53, 1, 94, 1}));
        out.push_back(DrawCommand::circle(Vector2{58, 8}, 8));
        out.push_back(DrawCommand::circle(Vector2{142, 8}, 8));
        out.push_back(DrawCommand::circle(Vector2{100, 8}, 8));
    } else if (mob == Mobility::TRACKED) {
        out.push_back(DrawCommand::path("M 53,1 l 100,0 c15,0 15,15 0,15 l -100,0 c-15,0 -15,-15 0,-15",
                                        BoundingBox{42, 0, 168, 18}));
    } else if (mob == Mobility::WHEELED_AND_TRACKED) {
        out.push_back(DrawCommand::circle(Vector2{58, 8}, 8));
        out.push_back(DrawCommand::path("M 83,1 l 70,0 c15,0 15,15 0,15 l -70,0 c-15,0 -15,-15 0,-15",
                                        BoundingBox{42, 0, 168, 18}));
    } else if (mob == Mobility::TOWED) {
        out.push_back(DrawCommand::path("M 63,1 l 74,0", BoundingBox{55, 0, 145, 10}));
        out.push_back(DrawCommand::circle(Vector2{58, 3}, 8));
        out.push_back(DrawCommand::circle(Vector2{142, 3}, 8));
    } else if (mob == Mobility::RAIL) {
        out.push_back(DrawCommand::path("M 53,1 l 96,0", BoundingBox{53, 1, 53 + 96, 1}));
        out.push_back(DrawCommand::circle(Vector2{58, 8}, 8));
        out.push_back(DrawCommand::circle(Vector2{73, 8}, 8));
        out.push_back(DrawCommand::circle(Vector2{127, 8}, 8));
        out.push_back(DrawCommand::circle(Vector2{142, 8}, 8));
    } else if (mob == Mobility::OVER_SNOW) {
        out.push_back(DrawCommand::path("M 50,-9 l10,10 90,0", BoundingBox{50, -9, 50 + 10 + 90, -9 + 10}));
    } else if (mob == Mobility::SLED) {
        out.push_back(DrawCommand::path("M 145,-12  c15,0 15,15 0,15 l -90,0 c-15,0 -15,-15 0,-15",
                                        BoundingBox{42, -12, 168, 3}));
    } else if (mob == Mobility::PACK_ANIMALS) {
        out.push_back(DrawCommand::path("M 80,20 l 10,-20 10,20 10,-20 10,20", BoundingBox{80, 0, 120, 20}));
    } else if (mob == Mobility::BARGE) {
        out.push_back(DrawCommand::path("M 50,1 l 100,0 c0,10 -100,10 -100,0", BoundingBox{50, 0, 150, 10}));
    } else if (mob == Mobility::AMPHIBIOUS) {
        out.push_back(DrawCommand::path("M 65,10 c 0,-10 10,-10 10,0 0,10 10,10 10,0	0,-10 10,-10 10,0 0,10 10,10 10,0	0,-10 10,-10 10,0 0,10 10,10 10,0	0,-10 10,-10 10,0",
                                        BoundingBox{65, 0, 100 - 65, 20}));
    } else if (mob == Mobility::SHORT_TOWED_ARRAY) {
        out.push_back(DrawCommand::path("M 50,5 l 100,0 M50,0 l10,0 0,10 -10,0 z M150,0 l-10,0 0,10 10,0 z M100,0 l5,5 -5,5 -5,-5 z",
                                        BoundingBox{50, 0, 150, 10}).with_fill(ColorType::ICON));
    } else if (mob == Mobility::LONG_TOWED_ARRAY) {
        out.push_back(DrawCommand::path("M 50,5 l 100,0 M50,0 l10,0 0,10 -10,0 z M150,0 l-10,0 0,10 10,0 z M105,0 l-10,0 0,10 10,0 z M75,0 l5,5 -5,5 -5,-5 z  M125,0 l5,5 -5,5 -5,-5 z",
                                        BoundingBox{50, 0, 150, 10}).with_fill(ColorType::ICON));
    }

    if (out.empty()) {
        return base_bbox;
    }

    out_items.push_back(DrawCommand::translate(Vector2{0, bbox.y2}, out));
    BoundingBox ret = base_bbox;
    ret.merge(out_items.back().get_bbox());
    return ret;
}

static void get_dismounted_leadership(bool leadership, Affiliation affiliation, const BoundingBox& bbox, std::vector<_impl::DrawCommand>& out_items) {
    if (!leadership)
        return;

    if (affiliation == Affiliation::FRIEND) {
        out_items.push_back(_impl::DrawCommand::path("m 45,60 55,-25 55,25", bbox.with_y1(bbox.y1 - 20)));
    } else if (affiliation == Affiliation::NEUTRAL) {
        out_items.push_back(_impl::DrawCommand::path("m 45,60 55,-25 55,25", bbox.with_y1(bbox.y1 - 20)));
    } else if (affiliation == Affiliation::HOSTILE) {
        out_items.push_back(_impl::DrawCommand::path("m 42,71 57.8,-43.3 58.2,42.8", bbox.with_y1(bbox.y1 - 20)));
    } else {
        out_items.push_back(_impl::DrawCommand::path("m 50,60 10,-20 80,0 10,20", bbox.with_y1(bbox.y1 - 20)));
    }
}


_impl::DrawCommand get_symbol_headquarters(Affiliation affiliation, Dimension dimension,
    real_t hq_staff_length, const BoundingBox& base_bbox, real_t frame_stroke_width,
    Vector2& staff_base)

{
    real_t y = 100;

    // For air and ground friendly/neutral, and sea/subsurface neutral icons, we start the HQ staff
    // at the bottom left corner, so we adjust the starting point accordingly.
    if (
        (dimension == Dimension::AIR && (affiliation == Affiliation::FRIEND || affiliation == Affiliation::NEUTRAL)) ||
        (dimension == Dimension::LAND && (affiliation == Affiliation::FRIEND || affiliation == Affiliation::NEUTRAL)) ||
        ((dimension == Dimension::SEA || dimension == Dimension::SUBSURFACE) && affiliation == Affiliation::NEUTRAL)
        ) {
        y = base_bbox.y2;
    }

    // For friendly subsurface units, we start at the upper-left corner
    if (dimension == Dimension::SUBSURFACE && affiliation == Affiliation::FRIEND) {
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

    real_t y1 = bbox.y1;
    real_t y2 = bbox.y2;

    if (symbol.get_status() != Status::UNDEFINED) {
        // @TODO handle status modifiers
    }
}

static BoundingBox apply_amplifiers(const SymbolStyle& style, const Symbol& symbol,
                                    const BoundingBox& base_bbox_raw,
                                    std::vector<_impl::DrawCommand>& out,
                                    Vector2& staff_base) {
    BoundingBox base_bbox = base_bbox_raw;
    BoundingBox modifier_bbox = base_bbox;
    Dimension dimension = dimension_from_symbol_set(symbol.get_symbol_set()); //.get_dimension();

    /*
     * Apply headquarters staff
     */

    if (symbol.is_headquarters()) {
        _impl::DrawCommand cmd = get_symbol_headquarters(symbol.get_affiliation(),
                                                         dimension,
                                                         style.hq_staff_length,
                                                         base_bbox,
                                                         style.frame_stroke_width,
                                                         staff_base);
        modifier_bbox.merge(cmd.get_bbox());
        out.push_back(cmd);
    }

    /*
     * Handle task force indicators
     */

    if (symbol.is_task_force()) {
        real_t width = get_task_force_width(symbol.get_echelon());
        BoundingBox tf_bbox{100 - width / 2, base_bbox.y1 - 40, 100 + width/2, base_bbox.y1};

        // Construct the path
        std::stringstream ss;
        ss << "M" << (100 - width/2) << "," << base_bbox.y1 << " L" << (100 - width/2) <<
            "," << (base_bbox.y1 - 40) << " " << (100 + width/2) << "," << (base_bbox.y1 - 40) <<
            " " << (100 + width/2) << "," << base_bbox.y1;
        out.push_back(_impl::DrawCommand::dynamic_path(std::move(ss.str()),
                                                       modifier_bbox).with_stroke_width(style.frame_stroke_width));
        modifier_bbox.merge(tf_bbox);
    }

    /*
     * Handle installation
     */
    bool is_installation =  (symbol.get_symbol_set() == SymbolSet::LAND_INSTALLATION);
    if (is_installation) {
        real_t gap_filler = 0;

        // Enemey air, ground, and sea symbols
        if (symbol.get_affiliation() == Affiliation::HOSTILE && (dimension == Dimension::AIR ||
                                                           dimension == Dimension::LAND ||
                                                           dimension == Dimension::SEA))
        {
            gap_filler = 14;
        }

        // Unknown air/sea/ground symbols
        if (symbol.get_affiliation() == Affiliation::UNKNOWN && (dimension == Dimension::AIR ||
                                                           dimension == Dimension::SEA ||
                                                           dimension == Dimension::LAND))
        {
            gap_filler = 2;
        }

        // Friendly air/sea symbols
        if (symbol.get_affiliation() == Affiliation::FRIEND && (dimension == Dimension::AIR ||
                                                          dimension == Dimension::SEA))
        {
            gap_filler = 2;
        }

        std::stringstream ss;
        ss << "M85," <<
            (base_bbox.y1 + gap_filler - style.frame_stroke_width / 2) <<
            " 85," <<
            (base_bbox.y1 - 10) <<
            " 115," <<
            (base_bbox.y1 - 10) <<
            " 115," <<
            (base_bbox.y1 + gap_filler - style.frame_stroke_width / 2) <<
            " 100," <<
            (base_bbox.y1 - style.frame_stroke_width) <<
            " Z";

        BoundingBox cmd_bbox = base_bbox;
        cmd_bbox.y1 = base_bbox.y1 - 10;
        auto cmd = _impl::DrawCommand::dynamic_path(std::move(ss.str()), cmd_bbox).with_fill(_impl::ColorType::ICON);
        out.push_back(cmd);
        modifier_bbox.merge(cmd_bbox);
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

    get_echelon_layer(is_installation, base_bbox, symbol.get_echelon(), out);
    modifier_bbox.merge(get_mobility_layer(is_installation, base_bbox, symbol.get_mobility(), symbol.get_affiliation(), out));
    get_dismounted_leadership(false, symbol.get_affiliation(), base_bbox, out);

    base_bbox.merge(modifier_bbox);
    return base_bbox;
}

static void apply_context(Context context, Affiliation affil, Dimension dim, const BoundingBox& bbox, std::vector<_impl::DrawCommand>& out) {
    using namespace _impl;

    real_t spacing = 10;
    if (affil == Affiliation::UNKNOWN || (affil == Affiliation::HOSTILE && dim == Dimension::SUBSURFACE)) {
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

    std::string ret = sidc_raw + '\0';
    std::string_view sidc = ret;

    if (sidc_raw.length() < 20) {
        std::cerr << "SIDC \"" << sidc << "\" must be at least 20 characters" << std::endl;
        return {};
    }

    // Determine version
    int version = int_substring(sidc, 0, 2);
    if (version > 10) {
//        std::cerr << "Warning: SIDC versions higher than 10 may not be dealt with appropriately (version is " <<
//            version_str << " -> " << version << ")" << std::endl;
    }

    Symbol symbol;

    /*
     * Parse standard identity
     */
    char context_int = sidc[2];
    switch(context_int) {
    case '2':
        symbol.context = Context::SIMULATION;
        break;
    case '1':
        symbol.context = Context::EXERCISE;
        break;
    default:
        symbol.context = Context::REALITY;
        break;
    }

    char affil = sidc[3];
    switch(affil) {
    case '1':
        symbol.affiliation = Affiliation::UNKNOWN;
        break;
    case '2':
        symbol.affiliation = Affiliation::ASSUMED_FRIEND;
        break;
    case '3':
        symbol.affiliation = Affiliation::FRIEND;
        break;
    case '4':
        symbol.affiliation = Affiliation::NEUTRAL;
        break;
    case '5':
        symbol.affiliation = Affiliation::SUSPECT;
        break;
    case '6':
        symbol.affiliation = Affiliation::HOSTILE;
        break;
    case '0':
        symbol.affiliation = Affiliation::PENDING;
        break;
    }

    // Parse the symbol sets
    int symbol_set_raw = int_substring(sidc, 4, 2);

    SymbolSet symbol_set;
    switch (symbol_set_raw) {
    case 1:
        symbol_set = SymbolSet::AIR;
        break;
    case 2:
        symbol_set = SymbolSet::AIR_MISSILE;
        break;
    case 5:
        symbol_set = SymbolSet::SPACE;
        break;
    case 6:
        symbol_set = SymbolSet::SPACE_MISSILE;
        break;
    case 10:
        symbol_set = SymbolSet::LAND_UNIT;
        break;
    case 11:
        symbol_set = SymbolSet::LAND_CIVILIAN_UNIT_ORGANIZATION;
        break;
    case 15:
        symbol_set = SymbolSet::LAND_EQUIPMENT;
        break;
    case 20:
        symbol_set = SymbolSet::LAND_INSTALLATION;
        break;
    case 30:
        symbol_set = SymbolSet::SEA_SURFACE;
        break;
    case 35:
        symbol_set = SymbolSet::SEA_SUBSURFACE;
        break;
    case 40:
        symbol_set = SymbolSet::ACTIVITIES;
        break;
    default:
        symbol_set = SymbolSet::UNDEFINED;
        break;
    }

    /*
     * Parse status
     */

    char status = sidc[6];
    switch(status) {
    case '1':
        symbol.presence = Presence::PLANNED;
        symbol.status = Status::UNDEFINED;
        break;
    case '2':
        symbol.presence = Presence::PLANNED;
        symbol.status = Status::FULLY_CAPABLE;
        break;
    case '3':
        symbol.presence = Presence::PLANNED;
        symbol.status = Status::DAMAGED;
        break;
    case '4':
        symbol.presence = Presence::PLANNED;
        symbol.status = Status::DESTROYED;
        break;
    case '5':
        symbol.presence = Presence::PLANNED;
        symbol.status = Status::FULL_TO_CAPACITY;
        break;
    default:
        symbol.presence = Presence::PRESENT;
        symbol.status = Status::UNDEFINED;
        break;
    }

    /*
     * Parse headquarters/task force/dummy elements
     */

    char hq = sidc[7];
    switch(hq) {
    case '1':
        symbol.headquarters = false;
        symbol.task_force = false;
        symbol.feint_dummy = true;
        break;
    case '2':
        symbol.headquarters = true;
        symbol.task_force = false;
        symbol.feint_dummy = false;
        break;
    case '3':
        symbol.headquarters = true;
        symbol.task_force = false;
        symbol.feint_dummy = true;
        break;
    case '4':
        symbol.headquarters = false;
        symbol.task_force = true;
        symbol.feint_dummy = false;
        break;
    case '5':
        symbol.headquarters = false;
        symbol.task_force = true;
        symbol.feint_dummy = true;
        break;
    case '6':
        symbol.headquarters = true;
        symbol.task_force = true;
        symbol.feint_dummy = false;
        break;
    case '7':
        symbol.headquarters = true;
        symbol.task_force = true;
        symbol.feint_dummy = true;
        break;
    default:
        symbol.headquarters = false;
        symbol.task_force = false;
        symbol.feint_dummy = false;
        break;
    }

    /*
     * Parse mobility/echelon
     */

    char ech1 = sidc[8];
    char ech2 = sidc[9];

    symbol.echelon = Echelon::UNDEFINED;
    symbol.mobility = Mobility::UNDEFINED;

    if (ech1 == '1') {
        // Echelon at brigade and below
        if (ech2 == '1')
            symbol.echelon = Echelon::TEAM;
        else if (ech2 == '2')
            symbol.echelon = Echelon::SQUAD;
        else if (ech2 == '3')
            symbol.echelon = Echelon::SECTION;
        else if (ech2 == '4')
            symbol.echelon = Echelon::PLATOON;
        else if (ech2 == '5')
            symbol.echelon = Echelon::COMPANY;
        else if (ech2 == '6')
            symbol.echelon = Echelon::BATTALION;
        else if (ech2 == '7')
            symbol.echelon = Echelon::REGIMENT;
        else if (ech2 == '8')
            symbol.echelon = Echelon::BRIGADE;
    } else if (ech1 == '2') {
        // Echelon at division and above
        if (ech2 == '1')
            symbol.echelon = Echelon::DIVISION;
        else if (ech2 == '2')
            symbol.echelon = Echelon::CORPS;
        else if (ech2 == '3')
            symbol.echelon = Echelon::ARMY;
        else if (ech2 == '4')
            symbol.echelon = Echelon::ARMY_GROUP;
        else if (ech2 == '5')
            symbol.echelon = Echelon::REGION;
        else if (ech2 == '6')
            symbol.echelon = Echelon::COMMAND;
    } else if (ech1 == '3') {
        // Equipment mobility on land
        if (ech2 == '1')
            symbol.mobility = Mobility::WHEELED;
        else if (ech2 == '2')
            symbol.mobility = Mobility::WHEELED_CROSS_COUNTRY;
        else if (ech2 == '3')
            symbol.mobility = Mobility::TRACKED;
        else if (ech2 == '4')
            symbol.mobility = Mobility::WHEELED_AND_TRACKED;
        else if (ech2 == '5')
            symbol.mobility = Mobility::TOWED;
        else if (ech2 == '6')
            symbol.mobility = Mobility::RAIL;
        else if (ech2 == '7')
            symbol.mobility = Mobility::PACK_ANIMALS;
    } else if (ech1 == '4') {
        // Equipment mobility on snow
        if (ech2 == '1')
            symbol.mobility = Mobility::OVER_SNOW;
        else if (ech2 == '2')
            symbol.mobility = Mobility::SLED;

    } else if (ech1 == '5') {
        // Equipment mobility on water
        if (ech2 == '1')
            symbol.mobility = Mobility::BARGE;
        else if (ech2 == '2')
            symbol.mobility = Mobility::AMPHIBIOUS;

    } else if (ech1 == '6') {
        // Naval towed array
        if (ech2 == '1')
            symbol.mobility = Mobility::SHORT_TOWED_ARRAY;
        else if (ech2 == '2')
            symbol.mobility = Mobility::LONG_TOWED_ARRAY;
    }

    /*
     * Parse entity
     * - Characters 10-15 inclusive are the entity type
     * - Characters 16-17 inclusive are modifier 1
     * - Characters 18-19 inclusive are modifier 2
     */
    entity_t entity_raw = 0;
    modifier_t modifier_1_raw = 0;
    modifier_t modifier_2_raw = 0;

    entity_raw = int_substring(sidc, 10, 6);
    symbol.entity = static_cast<int>(symbol_set) * ENTITY_SYMBOL_SET_OFFSET + entity_raw;

    modifier_1_raw = int_substring(sidc, 16, 2);
    symbol.modifier_1 = modifier_1_raw == 0 ? 0 : static_cast<int>(symbol_set) * MODIFIER_SYMBOL_SET_OFFSET + modifier_1_raw;

    modifier_2_raw = int_substring(sidc, 18, 2);
    symbol.modifier_2 = modifier_2_raw == 0 ? 0 : static_cast<int>(symbol_set) * MODIFIER_SYMBOL_SET_OFFSET + modifier_2_raw;

    return symbol;
}

Symbol::modifier_t Symbol::get_modifier(int mod) const noexcept {
    if (mod < 1 || mod > 2) {
        std::cerr << "No modifier set " << mod << std::endl;
        return 0;
    }

    return mod == 1 ? modifier_1 : modifier_2;
}

inline Vector2 scaled_to_center(const Vector2& vec, float scale) noexcept {
    return Vector2{100 +(vec.x - 100) * scale, 100 + (vec.y - 100) * scale};
}

Symbol::RichOutput Symbol::get_svg(const SymbolStyle& style) const noexcept {
    using namespace _impl;
    static constexpr const char* SVG_NS = "http://w3.org/2000/svg";

    bool position_only = (!style.use_entity_icon && !style.use_frame);

    SymbolSet symbol_set = get_symbol_set();

    _impl::SymbolLayer symbol_layer = _impl::get_symbol_layer(symbol_set, entity, IconType::ENTITY);
    _impl::SymbolLayer m1_layer = _impl::get_symbol_layer(symbol_set, modifier_1, IconType::MODIFIER_1);
    _impl::SymbolLayer m2_layer = _impl::get_symbol_layer(symbol_set, modifier_2, IconType::MODIFIER_2);

    // Add the base geometry
    std::vector<_impl::DrawCommand> components;
    bool use_civilian_color = false;
    if (symbol_layer.civilian_override || m1_layer.civilian_override || m2_layer.civilian_override) {
        use_civilian_color = true;
    }

    // Get base symbol_geometry
    BoundingBox base_bbox{100, 100, 100, 100};

    DrawCommand base = get_base_symbol_geometry(dimension_from_symbol_set(symbol_set),
                                                get_frame_affiliation(affiliation, context),
                                                context,
                                                position_only);
    if (!base.is_defined()) {
        std::cerr << "Undefined base" << std::endl;
        return {};
    }

    base_bbox = base.get_bbox();

    if (style.use_frame || position_only) {

        // Get base symbol
        _impl::DrawCommand sdc = base.copy_with_stroke_width(style.frame_stroke_width);

        // Handle unfilled icons
        if (style.color_mode == ColorMode::UNFILLED) {
            sdc.with_fill(ColorType::NONE);
        }

        bool dashed_frame = (affiliation == Affiliation::ASSUMED_FRIEND ||
                             affiliation == Affiliation::PENDING ||
                             affiliation == Affiliation::SUSPECT ||
                             presence != Presence::PRESENT);

        if (dashed_frame) {
            // Apply dashed frame base
            sdc.with_stroke(ColorType::WHITE);
        }

        components.push_back(sdc);

        if (dashed_frame) {
            // Apply dashed frame
            DrawCommand copy = sdc;
            copy.with_stroke(ColorType::ICON).with_stroke_style(StrokeStyle::DASHED).with_fill(ColorType::NONE);
            components.push_back(copy);
        }

        base_bbox = base.get_bbox();
    }

    // Handle various graphical modifiers
    if (!position_only) {
        if (style.use_amplifiers) {
            apply_context(context, affiliation, dimension_from_symbol_set(symbol_set), base_bbox, components);
        }

        if (symbol_set == SymbolSet::SPACE || symbol_set == SymbolSet::SPACE_MISSILE) {
            components.push_back(get_space_modifier(affiliation));
        } else if (symbol_set == SymbolSet::ACTIVITIES) {
            components.push_back(get_activity_modifier(affiliation));
        }
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
            bbox = comp.get_bbox();
            bbox_initialized = true;
        } else {
            bbox.merge(comp.get_bbox());
        }
    }

    /*
     * Apply modifiers
     */

    Vector2 hq_staff_base;
    if (!position_only && style.use_amplifiers) {
        bbox.merge(apply_amplifiers(style, *this, bbox, components, hq_staff_base));
    }

    //    bbox_initialized = false;
    for (const auto& comp : components) {
        bbox.merge(comp.get_bbox());
    }

    // Add entity
    if (style.use_entity_icon) {
        for (const auto& cmd : symbol_layer.draw_items) {
            components.push_back(cmd);
        }
    }

    // Add modifiers
    if (!position_only && style.use_entity_icon && style.use_modifiers) {
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

std::vector<Symbol::entity_t> Symbol::get_all_entities(SymbolSet symbol_set) noexcept {
    return _impl::get_available_symbols(symbol_set, _impl::IconType::ENTITY);
}

std::vector<Symbol::entity_t> Symbol::get_all_modifier_1s(SymbolSet symbol_set) noexcept {
    return _impl::get_available_symbols(symbol_set, _impl::IconType::MODIFIER_1);
}

std::vector<Symbol::entity_t> Symbol::get_all_modifier_2s(SymbolSet symbol_set) noexcept {
    return _impl::get_available_symbols(symbol_set, _impl::IconType::MODIFIER_2);
}

std::vector<Symbol::entity_t> Symbol::get_all_symbol_sets() noexcept {
    std::vector<Symbol::entity_t> ret;
    ret.reserve(SYMBOL_SETS.size());
    for (SymbolSet sym_set : SYMBOL_SETS) {
        ret.push_back(static_cast<entity_t>(sym_set));
    }
    return ret;
}

}
