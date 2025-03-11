#include "Symbol.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

/**
 * @brief Examples of constructing and saving symbols as SVG.
 */
int main(int argc, const char** argv) {

    /*
     * Example 1
     *
     * Print to the command line and save a symbol for a neutral amphibious
     * PSYOP platoon, modifying a friendly SIDC and using the default
     * style.
     */

    milsymbol::SymbolStyle alt_style;
    alt_style.use_modifiers = true;
    alt_style.color_mode = milsymbol::ColorMode::LIGHT;

    milsymbol::Symbol symbol = milsymbol::Symbol::from_sidc("130315003300000000000000000000")
                                   .with_affiliation(milsymbol::Affiliation::FRIEND)
                                   .with_amplifier(milsymbol::Amplifier::LONG_TOWED_ARRAY);

    milsymbol::Symbol::RichOutput results = symbol.get_svg(alt_style);
    std::cout << "Viewbox: " << results.svg_bounding_box.x1 << ", " << results.svg_bounding_box.y1 <<
        " to " << results.svg_bounding_box.x2 << ", " << results.svg_bounding_box.y2 << std::endl;
    std::cout << "Symbol anchor: " << results.symbol_anchor.x << ", " << results.symbol_anchor.y <<
        " (" <<
            (results.symbol_anchor.x + results.svg_bounding_box.x1) << ", " <<
            (results.symbol_anchor.y + results.svg_bounding_box.y1) << " in SVG space)" <<
        std::endl;

    std::ofstream example_1_file;
    example_1_file.open("example_1.svg", std::ios_base::out);
    example_1_file << results.svg;
    example_1_file.close();
    std::cout << "Symbol set " << std::hex << static_cast<int>(symbol.get_symbol_set()) << std::endl;

    std::array array_item = {milsymbol::Affiliation::UNKNOWN, milsymbol::Affiliation::NEUTRAL, milsymbol::Affiliation::SUSPECT, milsymbol::Affiliation::HOSTILE};
    for (int i = 0; i < array_item.size(); i++) {

        milsymbol::Affiliation aff = array_item[i];
        symbol = symbol.with_affiliation(aff);
        std::cout << symbol.to_sidc() << std::endl;
        milsymbol::Symbol::RichOutput results = symbol.get_svg(alt_style);


        std::ofstream example_1_file;
        std::stringstream name_stream;
        name_stream << "example_" << (i + 2) << ".svg";
        example_1_file.open(name_stream.str(), std::ios_base::out);
        example_1_file << results.svg;
        example_1_file.close();
    }

    // /*
    //  * Example 2
    //  *
    //  * Write a symbol to an SVG file with an unusually long headquarters
    //  * staff and using the dark color scheme.
    //  */

    // // Create the appropriate style
    // alt_style.hq_staff_length = 200;
    // alt_style.color_mode = milsymbol::ColorMode::UNFILLED;
    // alt_style.with_color_override(milsymbol::Color{12, 100, 96});

    // std::ofstream example_2_file;
    // example_2_file.open("example_2.svg", std::ios_base::out);
    // example_2_file << milsymbol::Symbol::from_sidc("130620000011050017101100000000")
    //                     .as_headquarters(true)
    //                     .with_affiliation(milsymbol::Affiliation::UNKNOWN)
    //                     .get_svg_string(alt_style);
    // example_2_file.close();

    // /*
    //  * Example 3
    //  *
    //  * Write a friendly infantry battalion symbol to an SVG file with the unfilled style.
    //  */

    // milsymbol::SymbolStyle example_3_style;
    // example_3_style.color_mode = milsymbol::ColorMode::UNFILLED;

    // milsymbol::Symbol example_3_symbol = milsymbol::Symbol{}
    //                                          .with_affiliation(milsymbol::Affiliation::FRIEND)
    //                                          .with_entity(milsymbol::Entity::LAND_UNIT_INFANTRY)
    //                                          .with_amplifier(milsymbol::Amplifier::BATTALION);

    // std::ofstream example_3_file;
    // example_3_file.open("example_3.svg", std::ios_base::out);
    // example_3_file << example_3_symbol.get_svg_string(example_3_style);
    // example_3_file.close();

    // /*
    //  * Example 4
    //  *
    //  * Write position-only representation of a symbol
    //  */

    // milsymbol::Symbol example_4_symbol = milsymbol::Symbol::from_sidc("130310000011020000890000000000");
    // std::ofstream example_4_file;

    // milsymbol::SymbolStyle example_4_style;
    // // example_4_style.use_frame = false;
    // // example_4_style.use_entity_icon = false;

    // example_4_file.open("example_4.svg", std::ios_base::out);
    // example_4_file << example_4_symbol.get_svg_string(example_4_style);
    // example_4_file.close();

    // // Example 5
    // example_3_symbol = milsymbol::Symbol{}
    //                                          .with_affiliation(milsymbol::Affiliation::HOSTILE)
    //                                          .with_entity(milsymbol::Entity::LAND_UNIT_INFANTRY)
    //                                          .with_amplifier(milsymbol::Amplifier::BATTALION);

    // std::ofstream example_5_file;
    // example_3_file.open("example_5.svg", std::ios_base::out);
    // example_3_file << example_3_symbol.get_svg_string(example_3_style);
    // example_3_file.close();

    return 0;
}
