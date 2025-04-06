import os
import json
import sys
import re
import glob
import argparse
import copy
import itertools

sys.path.append(os.path.join(os.path.dirname(__file__), 'python', 'src'))

import military_symbol

from schema import *
from output_style import OutputStyle

def _codify_hex(item):
	return f'0x{item}' if is_valid_hex_key(item) else -1

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
	text_path_font:str=OutputStyle.DEFAULT_FONT_FILE, ## A path to the font to use
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
		('SymbolSet', 'symbol_set', 'symbol_sets', SymbolSet),
		('FrameShape', 'frame_shape', 'frame_shapes', FrameShape)
	]

	for enum_value, singular, plural, cls in CONSTANT_ITEMS:
		const_text += f'enum class {enum_value} {{\n'
		const_text += ',\n'.join([f'\t{sanitize_constant(item.names[0])} = {_codify_hex(item.id_code)}' for item in getattr(schema, plural).values()])
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
	entities = [(ent, symset) for symset in symbol_sets for ent in symset.entities.values()]
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
			f'\t\t{{{_codify_hex(item.id_code)}, {enum_value}::{sanitize_constant(item.names[0])}}}' for item in getattr(schema, plural).values() if \
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
		dim_entries = [f'\t\t{{0x{entity_id}, Entity::{sanitize_constant(symbol_set.names[0])}_{sanitize_constant(entity.names[0])}}}' for (entity_id, entity) in symbol_set.entities.items()]
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

	# Create frame shape draw commands
	schema_text += "static constexpr const SymbolLayer get_base_symbol_geometry(FrameShape frame_shape, Affiliation affiliation) {\n"
	schema_text += "\tAffiliation base_affiliation = get_frame_base_affiliation(affiliation);\n"

	for base in schema.get_base_affiliations():
		schema_text += f'\tif (base_affiliation == Affiliation::{sanitize_constant(base.names[0])}) {{\n'
		schema_text += f'\t\tconst auto ENTITY_MAP = mapbox::eternal::map<FrameShape, SymbolLayer>({{\n'
		dim_entries = []

		for frame_shape in [f for f in schema.frame_shapes.values() if len(f.frames) > 0]:
			draw_commands = frame_shape.frames[base.id_code]
			draw_cmd = f'SymbolLayer{{{", ".join([cmd.cpp(schema=schema) for cmd in draw_commands])}}}'
			dim_entries.append(f'{{FrameShape::{sanitize_constant(frame_shape.names[0])}, {draw_cmd}}}')

		schema_text += ',\n'.join([f'\t\t\t{dim_entry}' for dim_entry in dim_entries])
		schema_text += f'\n\t\t}});\n\n'

		schema_text += '\t\tauto it = ENTITY_MAP.find(frame_shape);\n'
		schema_text += '\t\treturn (it != ENTITY_MAP.end() ? it->second : SymbolLayer{});\n'
		schema_text += f'\t}}\n\n'
	
	schema_text += '\treturn {};\n}\n\n'

	schema_text += 'static constexpr const FrameShape get_frame_shape(Dimension dimension) {\n\tswitch(dimension) {\n'
	for dimension in schema.dimensions.values():
		schema_text += f'\t\tcase Dimension::{sanitize_constant(dimension.names[0])}:\n\t\t\treturn FrameShape::{sanitize_constant(dimension.frame_shape.names[0]) if dimension.frame_shape else 'UNKNOWN'};\n'
	schema_text += f'\t\tdefault:\n\t\t\treturn FrameShape::LAND_UNIT;\n'
	schema_text += '\t}\n}\n\n'



	# Create base frame draw commands
	schema_text += "static constexpr const SymbolLayer get_base_symbol_geometry(Dimension dimension, Affiliation affiliation, bool position_only = false) {\n"
	schema_text += "\tAffiliation base_affiliation = get_frame_base_affiliation(affiliation);\n"
	schema_text += "\tFrameShape frame_shape = position_only ? FrameShape::POSITION_ONLY : get_frame_shape(dimension);\n"
	schema_text += "\tif (position_only) {dimension = Dimension::POSITION_MARKER;}\n"	
	schema_text += '\treturn get_base_symbol_geometry(frame_shape, affiliation);\n}\n\n'

	# Get amplifier offset


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

	schema_text += f'static constexpr Vector2 get_amplifier_offset(IconSide icon_side, Affiliation affiliation, FrameShape frame_shape) noexcept {{\n'
	schema_text += f'\tif (icon_side == IconSide::MIDDLE) {{\n\t\treturn {{}};\n\t}}\n\n'
	schema_text += '\taffiliation = get_frame_base_affiliation(affiliation);\n'
	schema_text += '\tswitch (frame_shape) {\n'
	for frame_shape in schema.frame_shapes.values():
		schema_text += '\t\tcase FrameShape::{}: {{\n'.format(sanitize_constant(frame_shape.names[0]))
		schema_text += f'\t\t\tconst auto MAP = mapbox::eternal::map<Affiliation, std::pair<Vector2, Vector2> >({{\n'
		for aindex, affil in enumerate(schema.get_base_affiliations()):
			def v2(item):
				return f'Vector2{{{item[0]}, {item[1]}}}'

			offsets = frame_shape.amplifier_offsets.get(affil.id_code, {'top': [0, 0], 'bottom': [0, 0], 'middle': [0, 0]})
			schema_text += '\t\t\t\t{{Affiliation::{}, std::pair<Vector2, Vector2>{{{}, {}}}}}{}\n'.format(
				sanitize_constant(affil.names[0]),
				v2(offsets['top']), v2(offsets['bottom']),
				',' if aindex < len(schema.get_base_affiliations()) else ''
			)
		schema_text += f'\t\t\t}});\n'
		schema_text += '\t\t\tauto it = MAP.find(affiliation);\n'
		schema_text += f'\t\t\treturn it == MAP.end() ? Vector2{{}} : (icon_side == IconSide::TOP ? it->second.first : it->second.second);\n'
		schema_text += '\t\t} break;\n'

	schema_text += '\n\t}\n\n\treturn Vector2{};\n'
	schema_text += '}\n\n'

	for item_name, list_attr in [('amplifier', 'amplifiers'), ('status', 'statuses')]:
		# Get symbol layer

		schema_text += f'static constexpr SymbolLayer get_{item_name}_layer({item_name.capitalize()} {item_name}, Affiliation affiliation, FrameShape frame_shape, bool use_alternate_icons = false) {{\n'
		alt_icons = [item for item in getattr(schema, list_attr).values() if hasattr(item, 'alt_icon') and item.alt_icon]		
		
		schema_text += f'\tstruct SymbolEntry {{\n\t\tSymbolLayer layer;\n\t\tIconSide icon_side = IconSide::MIDDLE;\n\t}};\n\n'

		if alt_icons:
			schema_text += f'\tconst auto ALT_MAP = mapbox::eternal::map<{item_name.capitalize()}, SymbolEntry >({{\n'			
			schema_text += ',\n'.join(
				[f'\t\t{{{item_name.capitalize()}::{sanitize_constant(item.names[0])}, {{{item.alt_icon_cpp(output_style=None, schema=schema)}, IconSide::{item.alt_icon_side.upper()}}}}}' for item in alt_icons]
			)
			schema_text += f'\n\t}});\n'
		
		schema_text += f'\tconst auto MAP = mapbox::eternal::map<{item_name.capitalize()}, SymbolEntry>({{\n'
		dim_entries = [f'\t{{{item_name.capitalize()}::{sanitize_constant(item.names[0])}, {{{item.icon_cpp(output_style=None, schema=schema)}, IconSide::{item.icon_side.upper()}}}}}' for item in getattr(schema, list_attr).values()]
		schema_text += ',\n'.join([f'\t{e}' for e in dim_entries])
		schema_text += f'\n\t}});\n\n'

		schema_text += '\tSymbolEntry ret;\n\n'
		extra_plus = ''

		if alt_icons:
			schema_text += f'\tauto it = ALT_MAP.find({item_name});\n'
			schema_text += '\tif (it != ALT_MAP.end()) {\n\t\tret = it->second;\n'
			schema_text += '\t} else {\n'
			extra_plus = '\t'
		
		schema_text += extra_plus + f'\tauto it = MAP.find({item_name});\n'
		schema_text += extra_plus + f'\tif (it != MAP.end()) {{\n{extra_plus}\t\tret = it->second;\n{extra_plus}\t}}\n'
		if alt_icons:
			schema_text += '\t}\n\n'

		schema_text += f'\tif (ret.layer.empty()) {{\n\t\treturn {{}};\n\t}}\n\n'

		schema_text += f'\tVector2 offset = get_amplifier_offset(ret.icon_side, affiliation, frame_shape);\n'
		schema_text += '\treturn SymbolLayer{DrawCommand::translate(offset, ret.layer)};\n'
		schema_text += '}\n\n'

	# Create the master list of symbol sets
	schema_text += "static constexpr SymbolLayer get_symbol_layer(SymbolSet symbol_set, int32_t code, IconType symbol_type) {\n"

	ICON_TYPES = [('ENTITY', 'Entity', ''), ('MODIFIER_1', 'Modifier1', 'M1_'), ('MODIFIER_2','Modifier2', 'M2_')]

	for index, symbol_set in enumerate(symbol_sets):
		schema_text += '\t{}if (symbol_set == SymbolSet::{}) {{\n'.format('else ' if index > 0 else '', sanitize_constant(symbol_set.names[0]))

		for symtype_index, sym_type in enumerate([symbol_set.entities, symbol_set.m1, symbol_set.m2]):
			if len(sym_type) < 1:
				continue
			schema_text += '\t\t{}if (symbol_type == IconType::{}) {{\n'.format('else ' if symtype_index > 0 and not symbol_set.common else '', ICON_TYPES[symtype_index][0])
			map_title:str = 'MAP'

			# Iterate through symbols
			schema_text += '\t\t\tconst auto {} = mapbox::eternal::map<int32_t, SymbolLayer>({{\n'.format(map_title)

			out_symbols = []
			for sym_code, symbol in sym_type.items():
				mod_code = f'M{symtype_index}_' if symtype_index > 0 else ''
				sanitized_name = sanitize_constant(f"{symbol_set.names[0]}_{mod_code}{symbol.names[0]}")
				out_symbols.append((sanitized_name, symbol.icon_cpp(output_style=output_style, schema=schema, with_bbox=True), symbol.names[0]))

			schema_text += ',\n'.join([f'\t\t\t\t{{static_cast<int32_t>({constant_name}), {draw_commands}}} /* {comment} */' for constant_name, draw_commands, comment in out_symbols]) + '\n'
			schema_text += '\t\t\t});\n'

			schema_text += "\t\t\tauto it = {}.find(code);\n".format(map_title) + \
				f"\t\t\treturn (it != {map_title}.end() ? it->second : SymbolLayer{{}});\n"

			schema_text += '\t\t}\n'

		schema_text += '\t\telse {\n\t\t\treturn {};\n\t\t}\n'
		schema_text += '\t}\n\n'

	schema_text +=  "\t// Default to nothing\n\treturn {};\n" + "}\n"

	# Create alt icons
	schema_text += "static constexpr SymbolLayer get_symbol_layer_alt_icon(SymbolSet symbol_set, int32_t code, IconType symbol_type) {\n"
	for symtype_index, sym_type in enumerate(['entities', 'm1', 'm2']):
		schema_text += '\t{}if (symbol_type == IconType::{}) {{\n'.format('else ' if symtype_index > 0 else '', ICON_TYPES[symtype_index][0])
		for index, symbol_set in enumerate(symbol_sets):
			candidates = [item for item in getattr(symbol_set, sym_type).values() if len(item.alt_icon) > 0]
			if len(candidates) < 1:
				continue

			schema_text += '\t\tif (symbol_set == SymbolSet::{}) {{\n'.format(sanitize_constant(symbol_set.names[0]))
			schema_text += '\t\t\tconst auto MAP = mapbox::eternal::map<int32_t, SymbolLayer>({\n'
			schema_text += ',\n'.join(['\t\t\t\t{{{}, {}}}'.format(
					f'static_cast<int32_t>({ICON_TYPES[symtype_index][1]}::{sanitize_constant(symbol_set.names[0])}_{ICON_TYPES[symtype_index][2]}{sanitize_constant(item.names[0])})',
					f'{item.alt_icon_cpp(output_style=output_style, schema=schema, with_bbox=True)}'
				)  for item in candidates])

			schema_text += '\n\t\t\t});\n\n'
			schema_text += "\t\t\tauto it = MAP.find(code);\n"
			schema_text += f"\t\t\tif (it != MAP.end()) {{\n\t\t\t\treturn it->second;\n\t\t\t}}\n"
			schema_text += '\t\t}\n'

			pass
		schema_text += '\t}\n'
	schema_text +=  "\n\t// Default to normal icon\n\treturn get_symbol_layer(symbol_set, code, symbol_type);\n" + "}\n\n"


	# Create the enumerator
	if include_enumerator:
		schema_text += '#define MILSYMBOL_HAS_SYMBOL_ENUMERATORS\n\n'
		schema_text += "static constexpr std::vector<int32_t> get_available_symbols(SymbolSet symbol_set, IconType symbol_type) {\n"

		for index, symbol_set in enumerate(symbol_sets):
			schema_text += '\t{}if (symbol_set == SymbolSet::{}) {{\n'.format('else ' if index > 0 else '', sanitize_constant(symbol_set.names[0]))

			SYMBOL_TYPE_HEADERS = ['ENTITY', 'MODIFIER_1', 'MODIFIER_2']

			for symtype_index, sym_type in enumerate([symbol_set.entities, symbol_set.m1, symbol_set.m2]):
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
	cwd = os.path.dirname(__file__)

	# Gather the JSON files to parse - all .json files in this directory
	schema = Schema.load_from_directory(directory=os.path.join(cwd, 'python', 'src', 'schema'))

	# Parse command line options
	parser = argparse.ArgumentParser('milymbol-build-helper', description='Milsymbol build helper')
	parser.add_argument('-p', '--text-paths', dest='use_text_paths', action='store_const', const=True, 
		default=True, help='Use paths for SVGs instead of text elements; this can be useful for rendering in some applications')
	parser.add_argument('-f', '--text-path-font', dest='text_path_font', action='store',
		default="",
		help='Font to use when creating a text path; only applicable when -p or --text-paths is passes as well')
	parser.add_argument('-g', '--godot_file_name', dest='godot_file_name', action='store', default='')
	arguments = parser.parse_args()

	print(f"Outputting C++ headers, using {'path' if arguments.use_text_paths else 'text'} elements for text...")
	create_schema(
		schema=schema,
		use_text_paths=arguments.use_text_paths,
		text_path_font=arguments.text_path_font,
		constant_filename=os.path.join(cwd, 'cpp', 'include', 'Constants.hpp'),
		schema_filename=os.path.join(cwd, 'cpp', 'include', 'Schema.hpp')
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

