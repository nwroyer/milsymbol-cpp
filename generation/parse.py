import os
import json
import sys
import re
import glob
import argparse
import copy
import itertools

from constants_parser import *
from drawing_items import *
from schema import *

"""
Generates the C++ headers for the combined symbol sets.

`symbol_sets` is a list of all the SymbolSet objects to construct for.
`schema_filename` is the path to output the schema (actual drawing elements) to
`constant_filename` is the path to output the enumerations to
`use_text_paths` indicates whether to replace all text elements with SVG paths,
	which may be desirable for some use cases.
"""
def create_schema(schema:Schema, 
	schema_filename:str,                  ## The filename of the C++ file to generate the symbol schema itself in
	constant_filename:str, 				  ## The filename of the C++ file to generate constants in
	use_text_paths:bool=False, 			  ## Whether to convert all text objects to SVG paths
	text_path_font:str=DEFAULT_FONT_FILE, ## A path to the font to use
	include_enumerator:bool=True         ## Whether to include a list of available symbols retrievable with a function in the generated files
	) -> None:
	
	if schema is None:
		print('No schema provided', file=sys.stderr)
		return

	symbol_sets = sorted(schema.symbol_sets.values())

	def sanitize_constant(constant:str) -> str:
		return re.sub(r'[\s,/\(\)\-\[\]]+', '_', constant).upper()

	output_style = OutputStyle()
	output_style.use_text_paths = use_text_paths

	# Create constant file
	const_text = ''

	const_text += '#pragma once\n'
	const_text += '#include <cstdint>\n'
	const_text += '#include <array>\n\n'

	const_text += 'namespace milsymbol {\n\n'

	# Create color mode schema
	const_text += 'enum class ColorMode {\n'
	const_text += ',\n'.join([f'\t{sanitize_constant(color_mode)} = {color_index}' for color_index, color_mode in enumerate(schema.color_modes)])
	const_text += '\n};\n\n'

	CONSTANT_ITEMS = [
		('Context', 'context', 'contexts', Context), 
		('Affiliation', 'affiliation', 'affiliations', Affiliation),
		('Amplifier', 'amplifier', 'amplifiers', Amplifier),
		('Status', 'status', 'statuses', Status), 
		('HQTFD', 'hqtfd', 'hqtfds', HQTFD), 
		('SymbolSet', 'symbol_set', 'symbol_sets', SymbolSet)
	]

	for enum_value, singular, plural, cls in CONSTANT_ITEMS:
		const_text += f'enum class {enum_value} {{\n'
		const_text += ',\n'.join([f'\t{sanitize_constant(item.names[0])} = 0x{item.id_code}' for item in getattr(schema, plural).values()])
		const_text += '\n};\n\n'

		if hasattr(cls, 'is_dashed') and len([i for i in getattr(schema, plural).values() if i.is_dashed()]) > 0:
			const_text += f"static constexpr bool is_{singular}_dashed({enum_value} {singular}) noexcept {{\n"
			const_text += "\tif(" + ' || '.join([f'{singular} == {enum_value}::{sanitize_constant(item.names[0])}' for item in getattr(schema, plural).values() if item.is_dashed()]) + ') {\n'
			const_text += '\t\treturn true;\n\t}\n\treturn false;\n}\n\n'	
			pass

	const_text += "static constexpr Affiliation get_frame_base_affiliation(Affiliation affiliation) noexcept {\n\tswitch(affiliation) {\n"
	# Create base frame affiliations
	for base in schema.get_base_affiliations():
		aliases = [affil for affil in schema.affiliations.values() if affil.get_base_frame_affiliation(schema=schema) == base]
		const_text += ''.join([f'\t\tcase Affiliation::{sanitize_constant(alias.names[0])}:\n' for alias in aliases])
		if base.names[0] == 'unknown':
			const_text += '\t\tdefault:\n'
		const_text += f'\t\t\treturn Affiliation::{sanitize_constant(base.names[0])};\n'
	const_text += '\t}\n}\n\n'

	const_text += "enum class Dimension {\n"
	const_text += ',\n'.join(['\tUNDEFINED = -1'] + [f'\t{sanitize_constant(dimension.id_code)} = {index}' for index, dimension in enumerate(schema.dimensions.values())])
	const_text += '\n};\n\n'

	# Create symbol set enums
	const_text += 'static constexpr int NOMINAL_ICON_SIZE = 200; /// The default icon size\n\n'
	const_text += f'static constexpr std::array<SymbolSet, {len(symbol_sets)}> SYMBOL_SETS = {{\n'
	const_text += ',\n'.join(['\tSymbolSet::{}'.format(sanitize_constant(symbol_set.names[0])) for symbol_set in symbol_sets]) + '\n'
	const_text += '};\n\n'

	const_text += 'enum Entity : int32_t {\n'
	const_text += '\tENTITY_UNKNOWN = 0,\n'
	entities = [(ent, symset) for symset in symbol_sets for ent in symset.icons.values()]
	const_text += ',\n'.join([f'\t{sanitize_constant(f"{symset.names[0]}_{ent.names[0]}")} = 0x{int(symset.id_code)}{ent.id_code}' for (ent, symset) in entities]) + '\n'
	const_text += '};\n\n'
	
	const_text += 'enum Modifier1 : int32_t {\n'
	const_text += f'\tM1_UNKNOWN = 0,\n'
	entities = [(ent, symset) for symset in symbol_sets for ent in symset.m1.values()]
	const_text += ',\n'.join([f'\t{f"{sanitize_constant(symset.names[0])}_M1_{sanitize_constant(ent.names[0])}"} = 0x{symset.id_code}{ent.id_code}' for (ent, symset) in entities]) + '\n'
	const_text += '};\n\n'
	
	const_text += 'enum Modifier2 : int32_t {\n'
	const_text += f'\tM2_UNKNOWN = 0,\n'
	entities = [(ent, symset) for symset in symbol_sets for ent in symset.m2.values()]
	const_text += ',\n'.join([f'\t{f"{sanitize_constant(symset.names[0])}_M2_{sanitize_constant(ent.names[0])}"} = 0x{symset.id_code}{ent.id_code}' for (ent, symset) in entities]) + '\n'
	const_text += '};\n\n'

	# Close namespace
	const_text += '}\n'
	with open(constant_filename, 'w') as constant_file:
		constant_file.write(const_text)

	"""
	Create schema proper
	"""
	schema_text = ''
	schema_text += '#pragma once\n'
	schema_text += '#include "DrawCommands.hpp"\n'
	schema_text += '#include "Types.hpp"\n'
	schema_text += '#include "Constants.hpp"\n'
	schema_text += '#include "eternal.hpp"\n\n'

	schema_text += 'namespace milsymbol::_impl {\n\n'

	# Create symbol type enum
	schema_text += "enum class IconType {\n" + "\tENTITY = 0,\n\tMODIFIER_1,\n\tMODIFIER_2\n};\n\n"

	for enum_value, singular, plural, cls in CONSTANT_ITEMS:
		schema_text += f'static constexpr {enum_value} sidc_to_{singular}(int hex_code) noexcept {{\n'
		schema_text += f'\tconst auto MAP = mapbox::eternal::map<int, {enum_value}>({{\n'
		schema_text += ',\n'.join([
			f'\t\t{{0x{item.id_code}, {enum_value}::{sanitize_constant(item.names[0])}}}' for item in getattr(schema, plural).values() if \
			not ('common' in dir(item) and item.common)
		])
		schema_text += '\n\t});\n'
		schema_text += '\tauto it = MAP.find(hex_code);\n'
		schema_text += f'\treturn (it != MAP.end() ? it->second : {enum_value}{{}});\n'
		schema_text += '}\n\n'

		schema_text += f'inline static constexpr {enum_value} sidc_to_{singular}(std::string_view strview) noexcept {{\n'
		schema_text += f'\treturn sidc_to_{singular}(_impl::hex_from_substring(strview));\n}}\n\n'

	HQTFD_COMPONENTS:list = ['headquarters', 'task_force', 'dummy']
	for item in HQTFD_COMPONENTS:
		schema_text += f'static constexpr bool sidc_to_{item}(int hex_code) noexcept {{\n'
		schema_text += f'\tHQTFD hqtfd = sidc_to_hqtfd(hex_code);\n'
		valid = [hqtfd for hqtfd in schema.hqtfds.values() if getattr(hqtfd, item)]
		schema_text += f'\tif ({" || ".join([f"hqtfd == HQTFD::{sanitize_constant(v.names[0])}" for v in valid])}) {{\n' 
		schema_text += f'\t\treturn true;\n\t}}\n\treturn false;\n}}\n\n'
		schema_text += f'inline static constexpr bool sidc_to_{item}(std::string_view strview) noexcept {{\n'
		schema_text += f'\treturn sidc_to_{item}(_impl::hex_from_substring(strview));\n}}\n\n'

	schema_text += f'static constexpr HQTFD get_hqtfd(bool headquarters, bool task_force, bool dummy) noexcept {{\n'
	hqtfd_options = list(reversed(sorted(schema.hqtfds.values(), key=lambda x: len(x.get_hqtfds()))))
	for index, hqtfd in enumerate(hqtfd_options):
		options = hqtfd.get_hqtfds()
		schema_text += f'\t{"else " if index > 0 else ""}{'if (' if len(options) > 0 else ''}' + ' && '.join(options) + \
			f'{')' if len(options) > 0 else ''} {{\n\t\treturn HQTFD::{sanitize_constant(hqtfd.names[0])};\n\t}}\n'
	schema_text += f'}};\n\n'

	# Get entity set
	schema_text += "static constexpr const Entity sidc_to_entity(SymbolSet symbol_set, int hex_code) {\n"
	for symbol_set in symbol_sets:
		if symbol_set.common:
			continue

		schema_text += f'\tif (symbol_set == SymbolSet::{sanitize_constant(symbol_set.names[0])}) {{\n'
		schema_text += f'\t\tconst auto ENTITY_MAP = mapbox::eternal::map<int, Entity>({{\n'
		dim_entries = [f'\t\t{{0x{entity_id}, Entity::{sanitize_constant(symbol_set.names[0])}_{sanitize_constant(entity.names[0])}}}' for (entity_id, entity) in symbol_set.icons.items()]
		schema_text += ',\n'.join([f'\t{e}' for e in dim_entries])
		schema_text += f'\n\t\t}});\n\n'

		schema_text += '\t\tauto it = ENTITY_MAP.find(hex_code);\n'
		schema_text += '\t\treturn (it != ENTITY_MAP.end() ? it->second : Entity::ENTITY_UNKNOWN);\n'
		schema_text += f'\t}}\n\n'
	schema_text += '\treturn {};\n}\n\n'

	for m in range(0, 2):
		schema_text += f"static constexpr const Modifier{m+1} sidc_to_modifier_{m+1}(SymbolSet symbol_set, int hex_code) {{\n"
		for symbol_set in symbol_sets:
			modifier_set = symbol_set.m1 if m == 0 else symbol_set.m2
			if len(modifier_set) < 1:
				continue

			schema_text += f'\tif (symbol_set == SymbolSet::{sanitize_constant(symbol_set.names[0])}) {{\n'
			schema_text += f'\t\tconst auto MODIFIER_MAP = mapbox::eternal::map<int, Modifier{m+1}>({{\n'
			dim_entries = [f'\t\t{{0x{mod_id}, Modifier{m+1}::{sanitize_constant(symbol_set.names[0])}_M{m+1}_{sanitize_constant(mod.names[0])}}}' for (mod_id, mod) in modifier_set.items()]
			schema_text += ',\n'.join([f'\t{e}' for e in dim_entries])
			schema_text += f'\n\t\t}});\n\n'

			schema_text += '\t\tauto it = MODIFIER_MAP.find(hex_code);\n'
			schema_text += f'\t\treturn (it != MODIFIER_MAP.end() ? it->second : Modifier{m+1}::M{m+1}_UNKNOWN);\n'
			schema_text += f'\t}}\n\n'

		schema_text += '\treturn {};\n}\n\n'

		schema_text += f"static constexpr bool is_modifier_{m+1}_common(Modifier{m+1} modifier) {{\n"
		schema_text += '\treturn ((static_cast<int>(modifier) & 0xF000) == 0xC000);\n'
		schema_text += '}\n\n'

	# Create base frame draw commands
	schema_text += "static constexpr const SymbolLayer get_base_symbol_geometry(Dimension dimension, Affiliation affiliation, Context context, bool position_only = false) {\n"
	schema_text += "\tAffiliation base_affiliation = get_frame_base_affiliation(affiliation);\n"
	schema_text += "\tif (position_only) {dimension = Dimension::POSITION_MARKER;}\n\n"

	for base in schema.get_base_affiliations():
		schema_text += f'\tif (base_affiliation == Affiliation::{sanitize_constant(base.names[0])}) {{\n'
		schema_text += f'\t\tconst auto ENTITY_MAP = mapbox::eternal::map<Dimension, SymbolLayer>({{\n'
		dim_entries = []

		for dimension in schema.dimensions.values():
			draw_commands = dimension.frames[base.names[0]]
			draw_cmd = f'SymbolLayer{{{", ".join([cmd.cpp(schema=schema) for cmd in draw_commands])}}}'
			dim_entries.append(f'{{Dimension::{sanitize_constant(dimension.id_code)}, {draw_cmd}}}')

		schema_text += ',\n'.join([f'\t\t\t{dim_entry}' for dim_entry in dim_entries])
		schema_text += f'\n\t\t}});\n\n'

		schema_text += '\t\tauto it = ENTITY_MAP.find(dimension);\n'
		schema_text += '\t\treturn (it != ENTITY_MAP.end() ? it->second : SymbolLayer{});\n'
		schema_text += f'\t}}\n\n'
	
	schema_text += '\treturn {};\n}\n\n'

	schema_text += 'static constexpr Vector2 get_amplifier_offset(Amplifier amplifier, Affiliation affiliation) noexcept {\n'
	schema_text += '\taffiliation = get_frame_base_affiliation(affiliation);\n'
	# Default amplifier to top
	schema_text += '\tbool amplifier_on_top = !({});\n'.format(' || '.join([f'amplifier == Amplifier::{sanitize_constant(amp.names[0])}' for amp in schema.amplifiers.values() if amp.side == 'bottom']))
	schema_text += '\tswitch (affiliation) {\n'
	for affil in schema.get_base_affiliations():
		
		for ta in [a for a in schema.get_base_affiliation_dict() if schema.get_base_affiliation_dict()[a] == affil]:
			schema_text += '\t\tcase Affiliation::{}:\n'.format(sanitize_constant(ta.names[0]))

		schema_text += '\t\t\treturn Vector2{{amplifier_on_top ? static_cast<real_t>({}) : static_cast<real_t>({}), amplifier_on_top ? static_cast<real_t>({}) : static_cast<real_t>({})}};\n\t\t\tbreak;\n'.format(
			affil.amplifier_offsets['top'][0], 
			affil.amplifier_offsets['bottom'][0], 
			affil.amplifier_offsets['top'][1], 
			affil.amplifier_offsets['bottom'][1]
		)
	schema_text += '\t}\n}\n\n'

	# Create the symbol set to dimension mapping
	schema_text += 'static constexpr Dimension dimension_from_symbol_set(SymbolSet set) noexcept {\n'
	schema_text += '\tswitch(set) {\n'
	for dimension in schema.dimensions.values():
		dim_sets = [symset for symset in symbol_sets if symset.dimension == dimension]
		for dim_set in dim_sets:
			schema_text += f'\t\tcase SymbolSet::{sanitize_constant(dim_set.names[0])}:\n'
		if dimension.id_code == 'land unit':
			schema_text += '\t\tdefault:\n'
		schema_text += f'\t\t\treturn Dimension::{sanitize_constant(dimension.id_code)};\n'

	schema_text += '\t}\n}\n\n'

	# Create full frame ordering
	schema_text += 'static constexpr int get_full_frame_ordering(Affiliation affiliation) noexcept {\n'
	schema_text += f'\tswitch(get_frame_affiliation(affiliation)) {{\n'
	for index, affiliation in enumerate(schema.full_frame_ordering):
		schema_text += f'\t\tcase Affiliation::{sanitize_constant(affiliation.names[0])}:\n\t\t\treturn {index};\n'
	unknown_index = [a.names[0] for a in schema.full_frame_ordering].index('unknown')
	schema_text += f'\t\tdefault:\n\t\t\treturn {unknown_index};\n'

	schema_text += '\t}\n}\n\n'

	schema_text += 'static constexpr SymbolLayer get_amplifier_layer(Amplifier amplifier, Affiliation affiliation) {\n'
	schema_text += f'\t\tconst auto MAP = mapbox::eternal::map<Amplifier, SymbolLayer>({{\n'
	dim_entries = [f'\t\t{{Amplifier::{sanitize_constant(amplifier.names[0])}, {amplifier.cpp(output_style=None, schema=schema)}}}' for amplifier in schema.amplifiers.values()]
	schema_text += ',\n'.join([f'\t{e}' for e in dim_entries])
	schema_text += f'\n\t\t}});\n\n'
	schema_text += '\t\tauto it = MAP.find(amplifier);\n'
	schema_text += '\t\tif (it == MAP.end()) {\n\t\t\treturn SymbolLayer{};\n\t\t}\n\n'
	schema_text += '\t\tVector2 offset = get_amplifier_offset(amplifier, affiliation);\n'
	schema_text += '\t\treturn SymbolLayer{DrawCommand::translate(offset, it->second)};\n'
	schema_text += '\t}\n\n'

	# Create the master list of symbol sets
	schema_text += "static constexpr SymbolLayer get_symbol_layer(SymbolSet symbol_set, int32_t code, IconType symbol_type) {\n"

	for index, symbol_set in enumerate(symbol_sets):
		schema_text += '\t{}if (symbol_set == SymbolSet::{}) {{\n'.format('else ' if index > 0 else '', sanitize_constant(symbol_set.names[0]))

		SYMBOL_TYPE_HEADERS = ['ENTITY', 'MODIFIER_1', 'MODIFIER_2']

		for symtype_index, sym_type in enumerate([symbol_set.icons, symbol_set.m1, symbol_set.m2]):
			if len(sym_type) < 1:
				continue
			schema_text += '\t\t{}if (symbol_type == IconType::{}) {{\n'.format('else ' if symtype_index > 0 and not symbol_set.common else '', SYMBOL_TYPE_HEADERS[symtype_index])

			map_title:str = f'{SYMBOL_TYPE_HEADERS[symtype_index]}_MAP'

			# Iterate through symbols
			schema_text += '\t\t\tconst auto {} = mapbox::eternal::map<int32_t, SymbolLayer>({{\n'.format(map_title)

			out_symbols = []
			for sym_code, symbol in sym_type.items():
				mod_code = f'M{symtype_index}_' if symtype_index > 0 else ''
				sanitized_name = sanitize_constant(f"{symbol_set.names[0]}_{mod_code}{symbol.names[0]}")
				out_symbols.append((sanitized_name, symbol.cpp(output_style=output_style, schema=schema, with_bbox=True), symbol.names[0]))

			schema_text += ',\n'.join([f'\t\t\t\t{{static_cast<int32_t>({constant_name}), {draw_commands}}} /* {comment} */' for constant_name, draw_commands, comment in out_symbols]) + '\n'
			schema_text += '\t\t\t});\n'

			schema_text += "\t\t\tauto it = {}.find(code);\n".format(map_title) + \
				f"\t\t\treturn (it != {map_title}.end() ? it->second : SymbolLayer{{}});\n"

			schema_text += '\t\t}\n'

		schema_text += '\t\telse {\n\t\t\treturn {};\n\t\t}\n'
		schema_text += '\t}\n\n'

	schema_text +=  "\n\t// Default to nothing\n\treturn {};\n" + "}\n"

	# Create the enumerator
	if include_enumerator:
		schema_text += '#define MILSYMBOL_HAS_SYMBOL_ENUMERATORS\n\n'
		schema_text += "static constexpr std::vector<int32_t> get_available_symbols(SymbolSet symbol_set, IconType symbol_type) {\n"

		for index, symbol_set in enumerate(symbol_sets):
			schema_text += '\t{}if (symbol_set == SymbolSet::{}) {{\n'.format('else ' if index > 0 else '', sanitize_constant(symbol_set.names[0]))

			SYMBOL_TYPE_HEADERS = ['ENTITY', 'MODIFIER_1', 'MODIFIER_2']

			for symtype_index, sym_type in enumerate([symbol_set.icons, symbol_set.m1, symbol_set.m2]):
				schema_text += '\t\t{}if (symbol_type == IconType::{}) {{\n'.format('else ' if symtype_index > 0 else '', SYMBOL_TYPE_HEADERS[symtype_index])

				# Iterate through symbols
				schema_text += '\t\t\treturn {{{}}};\n'.format(', '.join(
					[f'{sanitize_constant(symbol_set.names[0])}_{f"M{symtype_index}_" if symtype_index > 0 else ""}{sanitize_constant(sym.names[0])}' for sym_id, sym in sym_type.items()]
				))

				schema_text += '\t\t}\n' # Close if block for symbol type

			schema_text += '\t}\n\n' # Close if block for symbol set

		schema_text +=  "\n\t// Default to nothing\n\treturn {};\n" + "}\n" # Close function

	# Close the namespace
	schema_text += '}'

	with open(schema_filename, 'w') as schema_file:
		schema_file.write(schema_text)


def main() -> None:
	directory=os.path.dirname(__file__)

	# Gather the JSON files to parse - all .json files in this directory
	schema = Schema.parse_from_directory(directory=directory)

	# Parse command line options
	parser = argparse.ArgumentParser('milymbol-build-helper', description='Milsymbol build helper')
	parser.add_argument('-p', '--text-paths', dest='use_text_paths', action='store_const', const=True, 
		default=True, help='Use paths for SVGs instead of text elements; this can be useful for rendering in some applications')
	parser.add_argument('-f', '--text-path-font', dest='text_path_font', action='store',
		default=DEFAULT_FONT_FILE,
		help='Font to use when creating a text path; only applicable when -p or --text-paths is passes as well')
	parser.add_argument('-g', '--godot_file_name', dest='godot_file_name', action='store', default='')
	arguments = parser.parse_args()


	print(f"Outputting C++ headers, using {'path' if arguments.use_text_paths else 'text'} elements for text...")
	create_schema(
		schema=schema,
		use_text_paths=arguments.use_text_paths,
		text_path_font=arguments.text_path_font,
		constant_filename=os.path.join(directory, '..', 'include', 'Constants.hpp'),
		schema_filename=os.path.join(directory, '..', 'include', 'Schema.hpp')
	)

	if False:
		# Generate examples
		identity:int = 3 # Friend
		sidcs:list = []
		for sym_set in symbol_sets:
			for entity_id, entity in sym_set.m2.items():
				sidc:str = '300{}{:02}0000{:06}00{:02}'.format(identity, int(sym_set.id), 0, int(entity.uid)) # For M1
				# sidc:str = '300{}{:02}0000{:06}{:02}00'.format(identity, int(sym_set.id), 0, int(entity.uid)) # For M1
				# sidc:str = '300{}{:02}0000{:06}0000'.format(identity, int(sym_set.id), int(entity.uid)) # For entities
				sidcs.append(sidc)

		print('static constexpr std::array<const char*, {}> SIDCS = {{\n{}\n}};'.format(
			len(sidcs),
			',\n'.join([f'\t"{sidc}"' for sidc in sidcs])
		))

"""
Main command line interface. The only option is -p or --text-paths for using paths for text.
This is useful for rendering in some instances (e.g., the rendering library used for the end 
application doesn't support text elements). Otherwise, you'll generally get better quality
from leaving the arguments as default and thus using SVG text elements.
"""
if __name__ == '__main__':
	main()

