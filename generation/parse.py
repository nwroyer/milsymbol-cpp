import os
import json
import sys
import re
import glob
import argparse
import copy

from constants_parser import *
from drawing_items import *


"""
This file generates the actual C++ header files used to define constants
for entities and modifiers, as well as the actual code used to render
SVGs for symbols. 
"""


"""
Parse a specific item from JSON as a symbol element (path, text, etc.)
`item` is the item to be parsed; `full_items` is the dictionary
of all items in this set, to allow for aliases for symbols (e.g. supply units
have a similar full-frame line; aliasing allows the schema to not repeat
the entire definition for the line every time).
"""
def parse_symbol_element(item:dict, full_items:dict, constants:Constants) -> list:
	# Parse types
	new_element = None

	if 'text' in item:
		# Parse text
		new_element = SymbolElement.Text()
		new_element.text = item['text']
		new_element.text_type = 'normal'

		if 'pos' in item:
			new_element.pos = tuple(item['pos'])
			new_element.text_type = 'manual'

		if "fontsize" in item:
			new_element.font_size = float(item["fontsize"])
			new_element.text_type = 'manual'
	elif 'textm1' in item:
		# Parse text
		new_element = SymbolElement.Text()
		new_element.text = item['textm1']
		new_element.text_type = 'm1'
	elif 'textm2' in item:
		# Parse text
		new_element = SymbolElement.Text()
		new_element.text = item['textm2']
		new_element.text_type = 'm2'		
	elif 'd' in item:
		# Parse path
		new_element = SymbolElement.Path()
		new_element.d = item['d']
		if 'bbox' in item:
			new_element.bbox = tuple(item['bbox'])
	elif 'r' in item:
		# Parse circle
		new_element = SymbolElement.Circle()
		new_element.pos = tuple(item['pos'])
		new_element.radius = item['r']
	elif 'icon' in item:
		item_name:str = item['icon']
		if not isinstance(item_name, str):
			print("Bad icon: {}".format(item_name), file=sys.stderr)
		if item_name in full_items and 'icon' in full_items[item_name]:
			new_element = parse_item_icon(full_items[item_name]['icon'], full_items=full_items, constants=constants)
			if new_element is None or len(new_element.elements) < 1:
				print("Bad new element in icon {}".format(new_element), file=sys.stderr)
				return None
			return new_element.elements
		else:
			print('Unrecognized element {}'.format(item_name), file=sys.stderr)
			return None
	elif 'translate' in item:
		new_element = SymbolElement.Translate()
		new_element.delta = tuple(item['translate'])
	elif 'scale' in item:
		new_element = SymbolElement.Scale()
		new_element.scale = float(item['scale'])
	else:
		# Test for full-frame
		affiliation_dict = constants.get_base_affiliation_dict()
		for affiliation in affiliation_dict.keys():
			if affiliation not in item:
				print("Invalid full-frame element type {} - affiliation \"{}\" not found".format(item, affiliation), file=sys.stderr)
				return None

		# This is a valid full-frame icon
		new_element = SymbolElement.FullFrame(constants=constants)

		for type_name, type_entry in item.items():
			if not(type_name in affiliation_dict):
				print('Error: Unrecognized FF type "{}"'.format(type_name), file=sys.stderr)
				return None

			if type(type_entry) is not list:
				print(f'Bad entry for full-frame icon {type_name}', file=sys.stderr)
				return None

			for sub_entry in type_entry:
				new_subelements:list = parse_symbol_element(sub_entry, full_items=full_items, constants=constants)
				type_code = affiliation_dict[type_name].id_code
				new_element.elements[type_code].extend(new_subelements)

	# Parse subitems for transformation
	if isinstance(new_element, SymbolElement.Transformation):
		subitems = item['items']
		for subitem in subitems:
			new_subelements:list = parse_symbol_element(subitem, full_items=full_items, constants=constants)
			if new_subelements is None:
				print("Invalid subelements", file=sys.stderr)
				return None
			for sl in new_subelements:
				new_element.items.append(sl)

	# TODO load fill and stroke
	if new_element is not None:
		if new_element.fill_color is not None and new_element.fill_color not in COLORS:
			print("Bad fill color {}".format(new_element.fill_color), file=sys.stderr)
			return None
		if new_element.stroke_color is not None and new_element.stroke_color not in COLORS:
			print("Bad stroke color {}".format(new_element.stroke_color), file=sys.stderr)
			return None

		new_element.parse_basics(item)
	else:
		return []

	return [new_element]

"""
Parse a dict representing a symbol layer from the JSON and 
return an SymbolLayer object. This only handles the icon itself
and assumes the item is valid. See `parse_item` for the whole
parsed item.
"""
def parse_item_icon(item_icon, full_items:dict, constants:Constants) -> SymbolLayer:
	if type(item_icon) is list:
		new_sl:SymbolLayer = SymbolLayer()

		for element in item_icon:
			new_element_list:list = parse_symbol_element(element, full_items=full_items, constants=constants)
			if new_element_list is not None:
				for new_element in new_element_list:
					new_sl.elements.append(new_element)
			else:
				print('Error parsing symbol element', file=sys.stderr)
				return None

		return new_sl
	else:
		print("Icons must all be lists", file=sys.stderr)
		return None
	
	print(f'Unrecognized type: {item_icon}')
	return None

"""
Parse a dict representing a symbol layer from the JSON and 
return an SymbolLayer object
"""
def parse_item(uid:str, item:dict, full_items:dict, constants:Constants) -> SymbolLayer:

	if 'icon' not in item or 'names' not in item:
		print('No keys in {}'.format(uid))
		return None

	item_icon = item['icon']
	symbol_layer:SymbolLayer = parse_item_icon(item_icon, full_items=full_items, constants=constants)
	if symbol_layer == None:
		print(f"Bad symbol {uid}", file=sys.stderr)
		return None

	symbol_layer.civilian = bool(item['civ']) if 'civ' in item else False

	symbol_layer.uid = uid
	symbol_layer.names = item['names'] if 'names' in item else []
	return symbol_layer

"""
All acceptable item types in the JSON file defining
a symbol set
"""
ITEM_TYPES = ["IC", "M1", "M2"]

"""
Represents an entire symbol set with entities and two sets of modifiers.
"""
class SymbolSet:
	def __init__(self):
		self.id = '00'
		self.icons = {}
		self.m1 = {}
		self.m2 = {}
		self.name = ''
	def __lt__(self, other) -> bool:
		return int(self.id) < int(other.id)

"""
Parse a JSON file representing a single symbol set. This file should
be of the form:

```
{
	"set": "00",
	"name": "example_set",
	"IC": {
		...
	},
	"M1": {
		...
	},
	"M2": {
		...
	}
}
```
"""
def parse_symbol_set_file(filepath:str, constants:Constants) -> dict:
	if not os.path.exists(filepath):
		print(f'No file "{filepath}"')
		return

	json_str:str = ''
	with open(filepath, 'r') as json_file:
		json_str = json_file.read()
		json_str = re.sub('#[.]*\n', '', json_str)

	json_dict = json.loads(json_str)


	# Parse icon sets
	ret:dict = {
		it: {} for it in ITEM_TYPES
	}

	if not ('set' in json_dict):
		print("No set", file=sys.stderr)
		return None
	icon_set:str = json_dict['set']

	for item_type in ITEM_TYPES:
		if not (item_type in json_dict):
			continue

		for item_code, item in json_dict[item_type].items():
			# print(f'Loading {json_dict["set"]}:{item_type}:{item_code}')
			if not('names' in item and 'icon' in item):
				print(f'Improper indices for {json_dict["set"]}:{item_type}:{item_code}')
				return None

			new_sl = parse_item(uid=item_code, item=item, full_items=json_dict[item_type], constants=constants)
			if new_sl is not None:
				ret[item_type][item_code] = new_sl
			else:
				print(f'Unable to process item {json_dict["set"]}:{item_type}:{item_code}: {item["names"]}', file=sys.stderr)
				return 

	ret_set = SymbolSet()
	ret_set.id = icon_set
	ret_set.icons = {item: ret['IC'][item] for item in ret['IC'].keys() if item[0] != '.'} # Ignore utility symbols
	ret_set.m1 = ret['M1']
	ret_set.m2 = ret['M2']
	ret_set.name = json_dict['name']
	return ret_set

"""
Generates the C++ headers for the combined symbol sets.

`symbol_sets` is a list of all the SymbolSet objects to construct for.
`schema_filename` is the path to output the schema (actual drawing elements) to
`constant_filename` is the path to output the enumerations to
`use_text_paths` indicates whether to replace all text elements with SVG paths,
	which may be desirable for some use cases.
"""
def create_schema(constants:Constants, symbol_sets:list, schema_filename:str, constant_filename:str, use_text_paths:bool=False,
	text_path_font:str=DEFAULT_FONT_FILE, include_enumerator:bool=True, godot_filename:str = '') -> None:

	def sanitize_constant(constant:str) -> str:
		return re.sub(r'[\s,/\(\)\-\[\]]+', '_', constant).upper()

	output_style = OutputStyle()
	output_style.use_text_paths = use_text_paths

	# Create constants
	const_text = ''

	const_text += '#pragma once\n'
	const_text += '#include <cstdint>\n'
	const_text += '#include <array>\n\n'

	const_text += 'namespace milsymbol {\n\n'

	# Create color mode constants
	const_text += 'enum class ColorMode {\n'
	const_text += ',\n'.join([f'\t{sanitize_constant(color_mode)} = {color_index}' for color_index, color_mode in enumerate(constants.color_modes)])
	const_text += '\n};\n\n'

	# Create symbol set enums
	const_text += "enum class SymbolSet {\n"
	const_text += ',\n'.join(['\tUNDEFINED = -1'] + ['\t{} = {}'.format(sanitize_constant(symbol_set.name), int(symbol_set.id)) for symbol_set in symbol_sets]) + '\n'
	const_text += f'}};\n\nstatic constexpr int SYMBOL_SET_COUNT = {len(symbol_sets)};\n'
	const_text += 'static constexpr int NOMINAL_ICON_SIZE = 200; /// The default icon size\n\n'
	const_text += 'static constexpr std::array<SymbolSet, SYMBOL_SET_COUNT> SYMBOL_SETS = {\n'
	const_text += ',\n'.join(['\tSymbolSet::{}'.format(sanitize_constant(symbol_set.name)) for symbol_set in symbol_sets]) + '\n'
	const_text += '};\n\n'

	const_text += 'enum Entities : int32_t {\n'
	entities = [(ent, symset) for symset in symbol_sets for ent in symset.icons.values()]
	const_text += ',\n'.join([f'\t{sanitize_constant(f"{symset.name}_{ent.names[0]}")} = {int(symset.id)}{ent.uid}' for (ent, symset) in entities]) + '\n'
	const_text += '};\n\n'
	
	const_text += 'enum Modifier1 : int32_t {\n'
	entities = [(ent, symset) for symset in symbol_sets for ent in symset.m1.values()]
	const_text += ',\n'.join([f'\t{f"{sanitize_constant(symset.name)}_M1_{sanitize_constant(ent.names[0])}"} = {int(symset.id)}{ent.uid}' for (ent, symset) in entities]) + '\n'
	const_text += '};\n\n'
	
	const_text += 'enum Modifier2 : int32_t {\n'
	entities = [(ent, symset) for symset in symbol_sets for ent in symset.m2.values()]
	const_text += ',\n'.join([f'\t{f"{sanitize_constant(symset.name)}_M2_{sanitize_constant(ent.names[0])}"} = {int(symset.id)}{ent.uid}' for (ent, symset) in entities]) + '\n'
	const_text += '};\n\n'

	const_text += '}\n'
	with open(constant_filename, 'w') as constant_file:
		constant_file.write(const_text)

	"""
	Create schema proper
	"""
	schema = ''
	schema += '#pragma once\n'
	schema += '#include "DrawCommands.hpp"\n'
	schema += '#include "Constants.hpp"\n'
	schema += '#include "eternal.hpp"\n\n'
	schema += 'namespace milsymbol::_impl {\n'

	# Create symbol type enum
	schema += "enum class IconType {\n" + "\tENTITY = 0,\n\tMODIFIER_1,\n\tMODIFIER_2\n\n};\n"

	# Create the master list of symbol sets
	schema += "static constexpr SymbolLayer get_symbol_layer(SymbolSet symbol_set, int32_t code, IconType symbol_type) {\n"

	for index, symbol_set in enumerate(symbol_sets):
		schema += '\t{}if (symbol_set == SymbolSet::{}) {{\n'.format('else ' if index > 0 else '', sanitize_constant(symbol_set.name))

		SYMBOL_TYPE_HEADERS = ['ENTITY', 'MODIFIER_1', 'MODIFIER_2']

		for symtype_index, sym_type in enumerate([symbol_set.icons, symbol_set.m1, symbol_set.m2]):
			schema += '\t\t{}if (symbol_type == IconType::{}) {{\n'.format('else ' if symtype_index > 0 else '', SYMBOL_TYPE_HEADERS[symtype_index])

			map_title:str = f'{SYMBOL_TYPE_HEADERS[symtype_index]}_MAP'

			# Iterate through symbols
			schema += '\t\t\tconst auto {} = mapbox::eternal::map<int32_t, SymbolLayer>({{\n'.format(map_title)

			out_symbols = []
			for sym_code, symbol in sym_type.items():
				mod_code = f'M{symtype_index}_' if symtype_index > 0 else ''
				sanitized_name = sanitize_constant(f"{symbol_set.name}_{mod_code}{symbol.names[0]}")
				out_symbols.append((sanitized_name, symbol.cpp(output_style=output_style, constants=constants), symbol.names[0]))

			schema += ',\n'.join([f'\t\t\t\t{{static_cast<int32_t>({constant_name}), {draw_commands}}} /* {comment} */' for constant_name, draw_commands, comment in out_symbols]) + '\n'
			schema += '\t\t\t});\n'

			schema += "\t\t\tauto it = {}.find(code);\n".format(map_title) + \
				f"\t\t\treturn (it != {map_title}.end() ? it->second : SymbolLayer{{}});\n"

			schema += '\t\t}\n'

		schema += '\t}\n\n'

	schema +=  "\n\t// Default to nothing\n\treturn {};\n" + "}\n"

	# Create the enumerator
	if include_enumerator:
		schema += "static constexpr std::vector<int32_t> get_available_symbols(SymbolSet symbol_set, IconType symbol_type) {\n"

		for index, symbol_set in enumerate(symbol_sets):
			schema += '\t{}if (symbol_set == SymbolSet::{}) {{\n'.format('else ' if index > 0 else '', sanitize_constant(symbol_set.name))

			SYMBOL_TYPE_HEADERS = ['ENTITY', 'MODIFIER_1', 'MODIFIER_2']

			for symtype_index, sym_type in enumerate([symbol_set.icons, symbol_set.m1, symbol_set.m2]):
				schema += '\t\t{}if (symbol_type == IconType::{}) {{\n'.format('else ' if symtype_index > 0 else '', SYMBOL_TYPE_HEADERS[symtype_index])

				# Iterate through symbols
				schema += 'return {{{}}};'.format(', '.join(
					[f'{int(sym.uid)} /*{sym.names[0]}*/' for sym in sym_type.values()]
				))

				# schema += '\t\t\tconst auto {} = mapbox::eternal::map<int32_t, SymbolLayer>({{\n'.format(map_title)
				# schema += ',\n'.join(['\t\t\t\t{{{}{:02}, {}}} /* {} */'.format(int(symbol_set.id), int(sym.uid), sym.cpp(output_style=output_style), sym.names[0]) for sym_code, sym in sym_type.items()]) + '\n'
				# schema += '\t\t\t});\n'

				# schema += "\t\t\tauto it = {}.find(code);\n".format(map_title) + \
				# 	f"\t\t\treturn (it != {map_title}.end() ? it->second : SymbolLayer{{}});\n"

				schema += '\t\t}\n' # Close if block for symbol type

			schema += '\t}\n\n' # Close if block for symbol set

		schema +=  "\n\t// Default to nothing\n\treturn {};\n" + "}\n" # Close function

	# Close the namespace
	schema += '}'

	with open(schema_filename, 'w') as schema_file:
		schema_file.write(schema)

	if len(godot_filename) > 0:
		symbol_set_name_key:str = "SYMBOL_SET_NAME"
		entity_key:str = "ENTITIES"
		modifier_1_key:str = "MODIFIER_1"
		modifier_2_key:str = "MODIFIER_2"
		# Create Godot constants
		godot_file = 'class_name SIDCConstants\n'

		godot_file += f'const {symbol_set_name_key}:StringName = &"{symbol_set_name_key}"\n'
		godot_file += f'const {entity_key}:StringName = &"{entity_key}"\n'
		godot_file += f'const {modifier_1_key}:StringName = &"{modifier_1_key}"\n'
		godot_file += f'const {modifier_2_key}:StringName = &"{modifier_2_key}"\n'

		# Create the constants
		godot_file += 'const SYMBOL_SETS:Dictionary = {\n'
		for index, symbol_set in enumerate(symbol_sets):
			godot_file += f'\t{int(symbol_set.id)}: {{\n'

			godot_file += f'\t\t{symbol_set_name_key}: "{symbol_set.name}",\n'

			# Add in entities and modifiers
			SYMBOL_TYPE_HEADERS = [entity_key, modifier_1_key, modifier_2_key]

			for symtype_index, sym_type in enumerate([symbol_set.icons, symbol_set.m1, symbol_set.m2]):
				godot_file += '\t\t{}: {{\n'.format(SYMBOL_TYPE_HEADERS[symtype_index])

				godot_file += ',\n'.join(['\t\t\t{}: [{}]'.format(
					int(sym.uid),
					', '.join([f'\"{name}\"' for name in sym.names])
				) for sym in sym_type.values()]) + '\n'

				godot_file += '\t\t}}{}\n'.format(',' if symtype_index != (len(SYMBOL_TYPE_HEADERS) - 1) else '')

			godot_file += '\t}}{}\n'.format(',' if index != (len(symbol_sets) - 1) else '')
			pass

		godot_file += '}' # Close the dict

		# Write the file
		with open(godot_filename, 'w') as godot_file_object:
			godot_file_object.write(godot_file)

def main() -> None:
	# Gather the JSON files to parse - all .json files in this directory
	cwd = os.path.dirname(__file__)
	files = glob.glob(os.path.join(cwd, '*.json'))

	# Parse the constant file
	constant_files = [f for f in files if os.path.basename(f) == 'constants.json']
	if len(constant_files) < 1:
		print("No constant file \"constants.json\" found", file=sys.stderr)
		return

	constants = parse_constant_file(filepath=constant_files[0])

	# Parse all the JSON files
	symbol_sets = []
	for filename in [f for f in files if os.path.basename(f) != 'constants.json']:
		print(f'Parsing "{filename}"...')
		items = parse_symbol_set_file(filename, constants=constants)
		if items is None:
			print(f"Bad symbol set file \"{filename}\"", file=sys.stderr)

		symbol_sets.append(items)
	symbol_sets = sorted(symbol_sets)


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
		constants=constants,
		symbol_sets=symbol_sets, 
		use_text_paths=arguments.use_text_paths,
		text_path_font=arguments.text_path_font,
		constant_filename=os.path.join(cwd, '..', 'include', 'Constants.hpp'),
		schema_filename=os.path.join(cwd, '..', 'include', 'Schema.hpp'),
		godot_filename = os.path.join(cwd, '..', 'include', 'SIDCConstants.gd'))

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

