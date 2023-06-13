# Features

This is a small C++20 library for converting MILSTD-2525D military symbols into minimal SVGs, with a focus on ease of use and intended for applications where composing many symbols is required. 

The prototypical use case is the generation of many SVGs for unit symbols, which are then cached or rendered to one large "sprite sheet," for a real-time visualization or GIS application.

This library provides:

- Support for the following symbol sets as defined by MILSTD-2525D:
	- Air
	- Air missile
	- Space
	- Space missile
	- Land units
	- Land civilian units/organizations
	- Land installations
	- Land equipment
	- Sea surface
	- Sea subsurface
	- Activities
- Generation of tiny SVG files for symbols with arbitrary combinations of modifiers, entities, and identities
- Functions that shift much definition and processing to compile-time. Generating SVGs must be done at runtime; however, defining symbols and defining symbol elements is done with `constexpr` functions. 
- Generation of symbols using light, medium, dark, and unfilled styles
- Optional generation of path-only SVGs, for uses cases involving an SVG rasterizer that does not support text elements
- Useful metadata for symbol integration into larger projects, including the bounding boxes of symbol frames, appropriate symbol position origins (centers of symbols vs. bottoms of the "flagstaff" for headquarters), etc.

This library does **not** provide:

- Support for oceanographic, meteorological, and mine warfare symbols, although these may be included in future versions
- Support for tactical point, line, or area graphics, although these may be included in future versions
- Text modifiers outside the frame of a symbol
- Rasterization for SVGs. This library is intended to create SVGs for use in real-time visualization and GIS projects where rendering SVGs is already supported, or provide a starting point for integrating 2525D symbol generation with the rendering backend of your choice. 

Elements of the library are inspired by, and much of the SVG paths for symbology come from, the excellent [Milsymbol library](https://www.spatialillusions.com/milsymbol/) from Måns Beckman and contributors, [available on GitHub](https://github.com/spatialillusions/milsymbol) under an MIT license. I highly recommend using that library instead of this one for any web-based needs or for use cases where a more complete feature set is desired. 

The default font used for the optional path-only mode is "Simply Sans" from Wojciech Kalinowski, with the SIL Open Font License.

# Building

This library uses the Meson build system by default, and has no dependencies beyond the C++ standard template library. If you'd like to generate a new compile-time schema for symbol drawing, you'll need a recent Python version and (optionally) the freetype-py library (`pip install freetype-py`). 

Building is simple:

```Bash
milsymbol-cpp$ mkdir build
milsymbol-cpp$ meson setup build
milsymbol-cpp$ cd build && meson compile
```

Installation is one more command from the root directory of the repository:

```Bash
milsymbol-cpp$ cd build && meson install
```

# Example usage

*(Note that more example usage is available in `example.cpp`.)*

```cpp
/**
 * @brief Examples of constructing and saving symbols as SVG.
 */

#include <milsymbol/Symbol.hpp>

int main(int argc, const char** argv) {

    /*
     * Example 1
     *
     * Print to the command line a symbol for a neutral amphibious
     * PSYOP platoon, modifying a hostile SIDC and using the default
     * style
     */

    milsymbol::Symbol symbol = milsymbol::Symbol::from_sidc("30061000141106000060").with_affiliation(milsymbol::Affiliation::NEUTRAL);
    std::cout << symbol.get_svg();

    /*
     * Example 2
     *
     * Write a symbol to an SVG file with an unusually long headquarters
     * staff and using the dark color scheme
     */

    // Create the appropriate style
    milsymbol::SymbolStyle alt_style;
    alt_style.hq_staff_length = 200;
    alt_style.color_mode = milsymbol::ColorMode::DARK;

    std::ofstream example_2_file;
    example_2_file.open("example_2.svg", std::ios_base::out);
    example_2_file << milsymbol::Symbol{}.with_affiliation(milsymbol::Affiliation::FRIEND).as_headquarters(true)
                     .with_affiliation(milsymbol::Affiliation::HOSTILE).get_svg(alt_style);
    example_2_file.close();

    /*
     * Example 3
     *
     * Write a friendly infantry battalion symbol to an SVG file with the unfilled style
     */

    milsymbol::SymbolStyle example_3_style;
    example_3_style.color_mode = milsymbol::ColorMode::UNFILLED;

    milsymbol::Symbol example_3_symbol = milsymbol::Symbol{}
                                             .with_affiliation(milsymbol::Affiliation::FRIEND)
                                             .with_entity(milsymbol::Entities::LAND_UNIT_INFANTRY)
                                             .with_echelon(milsymbol::Echelon::BATTALION);

    std::ofstream example_3_file;
    example_3_file.open("example_3.svg", std::ios_base::out);
    example_3_file << example_3_symbol.get_svg(example_3_style);
    example_3_file.close();

    return 0;
}
```

# Code generation for symbol schemas

This generates the C++ header files used to define the symbols at compile time. This workflow was chosen because it's easier to update and add new symbols like this instead of manually hard-coding them, and provides for more consistency and correctness in the generated files. 

To use the default functionality, simply run the following:

```bash
milsymbol-cpp$ python generation/parse.py
```

If your use case involves an SVG rendering library that doesn't have text support, you can compile the library to include all text-based symbols as paths. To do this, run the generation script as follows:

```bash
milsymbol-cpp$ python generation/parse.py --text-paths --text-path-font 'path/to/font.ttf'
```

If no `--text-path-font` parameter is provided, the default font will be used.

# License

This library is licensed under the MIT license:

Copyright 2023 Nicholas Royer

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

