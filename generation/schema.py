import os
import re
import json
import drawing_items
import sys

def is_valid_hex_key(key:str, required_length:int=-1) -> bool:
	"""
	Returns whether the given key is a valid hex key (string of only hex digits). If required_length is specified,
	determines whether the key is of the required length.
	"""

	if len(key) < 1:
		return False
	if required_length > 0 and len(key) != required_length:
		return False
	for k in key.upper():
		if k not in '0123456789ABCDEF':
			return False
	return True


class Context:
	"""
	Represents a standard identity context
	"""
	def __init__(self):
		self.id_code:str = ""      # The ID code of the context, a 1-digit hexadecimal 
		self.names:list  = []      # The names of the context
		self.base_context:str = "" # The base context this belongs to (reality, exercise, or simulation)

	def __repr__(self):
		return f"Context {self.id_code} [{self.base_context}]: (" + ', '.join([f'\"{f}\"' for f in self.names]) + ")"

	@staticmethod
	def from_dict(id_code:str, json:dict):
		if not is_valid_hex_key(id_code, 1):
			print(f'Bad context ID {id_code}')
			return None

		context:Context = Context()
		context.id_code = id_code
		context.names = json['names']
		context.base_context = json.get('base context', id_code)
		return context


class Affiliation:
	"""
	Represents a standard identity affiliation
	"""

	def __init__(self):
		self.id_code:str = ""                 # A 1-digit hexadecimal
		self.names:list = []                  # The names of the affiliation
		self.colors:dict = {}                 # Should contain the keys ['light', 'medium', 'dark', 'unfilled']
		self.dashed:bool = False              # Whether this renders the frame dashed
		self.has_civilian_variant:bool = True # Whether this affiliation allows civilian coloring
		self.frame_id:str = ""                # The affiliation code to use the frames from. If not set this is assumed to be its own base
		self.color_id:str = ""                # The affiliation code to use the colors from. If not set this is assumed to be its own base.

	def __repr__(self):
		ret = f"Affiliation {self.id_code}: (" + ', '.join([f'\"{f}\"' for f in self.names]) + ")"
		if len(self.frame_id) > 0:
			ret += f' [Uses frame {self.frame_id}]'
		if len(self.color_id) > 0:
			ret += f' [Uses color {self.color_id}]'
		if self.has_civilian_variant:
			ret += ' +C'
		return ret

	@staticmethod
	def from_dict(id_code:str, json:dict, schema):
		if not is_valid_hex_key(id_code, 1):
			print(f'Bad affiliation ID {id_code}', file=sys.stderr)
			return None

		affiliation:Affiliation = Affiliation()
		affiliation.id_code = id_code
		affiliation.names = json["names"]
		affiliation.has_civilian_variant = bool(json.get("has civilian variant", True))
		affiliation.dashed = bool(json.get('dashed', False))
		affiliation.frame_id = json.get("frame base", "")
		affiliation.color_id = json.get("color base", "")

		if 'colors' in json:
			if len([c for c in schema.color_modes if c in json['colors']]) != len(schema.color_modes):
				print(f'Not all colors [{",".join(schema.color_modes)}] found for {affiliation.id_code}', file=sys.stderr)
				return None

			affiliation.colors = {color_id: json['colors'][color_id] for color_id in schema.color_modes}

		return affiliation

	def get_base_frame_affiliation(self, schema):
		if not self.frame_id or schema is None:
			return self

		if self.frame_id not in schema.affiliations:
			print(f"No base frame ID {self.frame_id} found for {self}", file=sys.stderr)
			return None

		return schema.affiliations[self.frame_id]


class Dimension:
	""" 
	Represents a dimension, which sets the frame type
	"""

	def __init__(self):
		self.id_code:str = ""  # Human readable name for the dimension
		self.frames:dict = {}  # Dictionary of frames for IDs

	def __repr__(self):
		return f'Dimension \"{self.id_code}\" {len(self.frames[list(self.frames.keys())[0]])}'

	@staticmethod
	def from_dict(id_code:str, json:dict, over_dict:dict):
		dimension:Dimension = Dimension()
		dimension.id_code = id_code

		def create_base_frames(json:dict, over_dict:dict, ret:dict = {}) -> dict:
			if 'frame base' in json:
				base_dim:str = json['frame base']
				if base_dim not in over_dict:
					raise Exception(f"No dimension \"{base_dim}\" defined")
					return None

				ret = create_base_frames(json=over_dict[base_dim], over_dict=over_dict)

			# Apply base frame
			for frame_key, frame_list in json.get("frames", {}).items():
				ret[frame_key] = [f for f in frame_list]

			# Apply frame decorators
			for frame_key, frame_list in json.get("decorators", {}).items():
				if frame_key in ret:
					ret[frame_key] = ret[frame_key] + frame_list
				else:
					ret[frame_key] = [f for f in frame_list]

			return ret


		frames = create_base_frames(json=json, over_dict=over_dict['dimensions'])
		if frames is None:
			raise Exception('Ex')
			return None

		dimension.frames = {}
		for affil in frames:
			frame = []
			for item in frames[affil]:
				frame += drawing_items.SymbolElement.parse_from_dict(item=item, full_items={}, affiliations={})
			dimension.frames[affil] = frame

		return dimension


class Status:
	"""
	Represents a status
	"""

	def __init__(self):
		self.id_code:str = ""
		self.names:list = []
		self.dashed:bool = False

	def __repr__(self):
		return f"Status {self.id_code} ({' / '.join(self.names)})"

		if not is_valid_hex_key(status_id, 1):
			print(f"Bad status {status_id}", file=sys.stderr)
			return None

	@staticmethod
	def from_dict(id_code:str, json:dict):
		if not is_valid_hex_key(id_code, 1):
			print(f"Bad status {id_code}", file=sys.stderr)
			return None			

		status:Status = Status()
		status.id_code = id_code
		status.names = json.get('names', [])
		status.dashed = json.get("dashed", False)
		return status


class HQTFD:
	"""
	Represents a HQTFD code
	"""

	def __init__(self):
		self.id_code:str = "" # 1-digit hexadecimal
		self.names:list = []
		self.dashed:bool = False

	def __repr__(self) -> str:
		return f"HQTFD {self.id_code} ({self.names[0]})"

	@staticmethod
	def from_dict(id_code:str, json:dict):
		if not is_valid_hex_key(id_code, 1):
			print(f"Bad HQTFD {id_code}", file=sys.stderr)
			return None

		hqtfd:HQTFD = HQTFD()
		hqtfd.id_code = id_code
		hqtfd.names = json.get('names', [])
		if len(hqtfd.names) < 1:
			print(f"No names for amplifier {self.id_code}")
			return None

		hqtfd.dashed = json.get("dashed", False)
		return hqtfd


class Amplifier:
	"""
	Represents an amplifier
	"""

	def __init__(self):
		self.id_code:str = "" # 1-digit hexadecimal
		self.names:list = [] # Amplifier names
		self.category:str = "" # Category this applies to
		self.applies_to:list = [] # List of dimensions this applies to

	@staticmethod
	def from_dict(id_code:str, json:dict, schema):
		if not is_valid_hex_key(id_code, 2):
			print(f'Bad ID code \"{id_code}\" for amplifier', file=sys.stderr)
			return None

		amplifier:Amplifier = Amplifier()
		amplifier.id_code = id_code
		amplifier.names = json.get("names", [])
		if len(amplifier.names) < 1:
			print(f"No names for amplifier {self.id_code}")
			return None

		amplifier.category = json.get("category", "")
		amplifier.applies_to = json.get("applies to", [])
		for apt in amplifier.applies_to:
			if apt not in schema.dimensions:
				print(f"Bad applies to dimension \"{apt}\" for amplifier {amplifier.id_code}", file=sys.stderr)
				return None

		return amplifier


"""
A full symbol component (e.g. an entity or modifier)
"""
class SymbolLayer:
	def __init__(self):
		self.id_code:str = '' # A six (for entities) or two-digit hex code
		self.names:str = [] # Human-readable names
		self.elements:list = [] # the symbol elements
		self.civilian:bool = False # Whether this entity renders something a civilian item
		pass

	def __repr__(self):
		return '{{{}}} -> {}'.format(self.uid, self.elements)

	def cpp(self, schema, output_style, with_bbox=False):
		return 'SymbolLayer{{{}}}{}'.format(
			', '.join([cmd.cpp(output_style=output_style, schema=schema, with_bbox=with_bbox) for cmd in self.elements]),
			'.with_civilian_override(true)' if self.civilian else ''
		)

	@staticmethod
	def parse_from_dict(id_code:str, json:dict, full_items:dict, schema):
		if 'icon' not in json or 'names' not in json:
			print('No keys in {}'.format(uid))
			return None

		# if not is_valid_hex_key(id_code):
		# 	print(f'Invalid hex ID code for symbol layer \"{id_code}\"')
		# 	return None

		symbol_layer = SymbolLayer()
		symbol_layer.id_code = id_code
		symbol_layer.names = json['names'] if 'names' in json else []
		symbol_layer.civilian = json.get('civ', False)

		item_icon = json['icon']
		if type(item_icon) is not list:
			print("Icons must all be lists", file=sys.stderr)
			return None

		for element in item_icon:
			new_element_list:list = drawing_items.SymbolElement.parse_from_dict(element, full_items=full_items, affiliations=schema.get_base_affiliation_dict())
			if new_element_list is not None:
				symbol_layer.elements.extend(new_element_list)
			else:
				print('Error parsing symbol element', file=sys.stderr)
				return None

		if symbol_layer == None:
			print(f"Bad symbol {uid}", file=sys.stderr)
			return None
		
		return symbol_layer


class Entity(SymbolLayer):
	"""
	Represents an entity
	"""
	def __init__(self):
		super().__init__()

class Modifier(SymbolLayer):
	"""
	Represents a modifier
	"""
	def __init__(self):
		super().__init__()


class SymbolSet:
	"""
	Represents a symbol set with entities and modifiers
	"""
	def __init__(self):
		self.id_code:str = '00' # The identifier of the symbol set
		self.names:list = []
		self.dimension = None
		self.common = False

		self.entities:dict = {} # A map of the entities in this symbol set
		self.m1:dict = {}
		self.m2:dict = {}
		
	def __lt__(self, other) -> bool:
		if self.common != other.common:
			return not self.common

		return int(self.id_code) < int(other.id_code)

	@classmethod
	def parse_from_file(cls, filepath:str, schema):
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

		ITEM_TYPES = ["IC", "M1", "M2"]

		if not os.path.exists(filepath):
			print(f'No file "{filepath}"')
			return None

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

		is_common = json_dict.get('common', False)

		if 'dimension' not in json_dict and not is_common:
			raise Exception(f"No dimension defined in \"{filepath}\"")
			return None

		if not is_common and json_dict['dimension'] not in schema.dimensions:
			raise Exception(f"Dimension \"{json_dict['dimension']}\" not found from \"{filepath}\"")
			return None

		icon_set:str = json_dict['set']

		for item_type in ITEM_TYPES:
			if not (item_type in json_dict):
				continue

			for item_code, item in json_dict[item_type].items():
				# print(f'Loading {json_dict["set"]}:{item_type}:{item_code}')
				if not(('names' in item or 'name' in item) and 'icon' in item):
					print(f'Improper indices for {json_dict["set"]}:{item_type}:{item_code}')
					return None

				new_symbol_layer = SymbolLayer.parse_from_dict(id_code=item_code, json=item, full_items=json_dict[item_type], schema=schema)
				if new_symbol_layer is not None:
					ret[item_type][item_code] = new_symbol_layer
				else:
					print(f'Unable to process item {json_dict["set"]}:{item_type}:{item_code}: {item["names"]}', file=sys.stderr)
					return 

		ret_set = cls()
		ret_set.id_code = icon_set
		ret_set.icons = {item: ret['IC'][item] for item in ret['IC'].keys() if item[0] != '.'} # Ignore utility symbols
		ret_set.m1 = ret['M1']
		ret_set.m2 = ret['M2']
		ret_set.names = json_dict['names'] if 'names' in json_dict else [json_dict['name']]
		ret_set.dimension = schema.dimensions[json_dict['dimension']] if not is_common else False
		ret_set.common = is_common
		return ret_set

class Schema:
	"""
	Represents a full symbol schema
	"""

	def __init__(self):
		## The list of color modes (icon, icon fill, etc.) this has
		self.color_modes:list = []
		## The order in which full frame symbols are expected (for C++)
		self.full_frame_ordering:list = []
		## The dimensions this schema has
		self.dimensions:dict = {}
		## The contexts this schema has
		self.contexts:dict = {}
		## The affiliations this schema has
		self.affiliations:dict = {}
		## The statuses this schema has
		self.statuses:dict = {}
		## The headquarters/task force/dummy codes
		self.hqtfds:dict = {}
		## A mapping of [symbol set ID : symbol set object]
		self.symbol_sets:dict = {}

	def print_constants(self):
		print("Constants set")

		for item_set in ['contexts', 'dimensions', 'color_modes', 'contexts', 'affiliations', 'statuses', 'hqtfds']:
			print(f'\t{item_set.capitalize()}:')
			if isinstance(vars(self)[item_set], list):
				for item in vars(self)[item_set]:
					print(f'\t\t{item}')
			else:
				for item in vars(self)[item_set].values():
					print(f'\t\t{item}')

	def get_base_affiliations(self) -> list:
		ret = []
		for aff in self.affiliations.values():
			if aff.get_base_frame_affiliation(schema=self) == aff:
				ret.append(aff)

		ret = sorted(ret, key=lambda x: x.id_code)
		return ret

	def get_base_affiliation_dict(self) -> list:
		return {ret: ret.get_base_frame_affiliation(schema=self) for ret in self.affiliations.values()}

	def parse_from_file(self, filepath:str):
		"""
		Parses a set of constants from a given filepath
		"""

		if not os.path.exists(filepath):
			print(f'No constant file "{filepath}"')
			return None

		json_str:str = ''
		with open(filepath, 'r') as json_file:
			json_str = json_file.read()
			json_str = re.sub('#[.]*\n', '', json_str)

		json_dict = json.loads(json_str)

		print(f'Parsing constant file \"{filepath}\"')
			
		# Validate required keys
		REQUIRED_KEYS:list = ['contexts', 'affiliations', 'color modes', 'dimensions', 'full frame ordering']
		for required_key in REQUIRED_KEYS:
			if required_key not in json_dict:
				print(f"Required key \"{required_key}\" not found in constants.json", file=sys.stderr)
				return None

		# Load contexts
		self.contexts = {}
		for context_id, context_dict in json_dict["contexts"].items():
			context = Context.from_dict(context_id, context_dict)
			self.contexts[context_id] = context

		# Load color modes
		self.color_modes = []
		for color_mode in json_dict['color modes']:
			self.color_modes.append(color_mode.lower())

		# Load affiliations
		for aff_id, aff_dict in json_dict["affiliations"].items():
			affiliation = Affiliation.from_dict(aff_id, aff_dict, schema=self)
			self.affiliations[aff_id] = affiliation

		# Load full frame ordering
		self.full_frame_ordering = []
		for item in json_dict["full frame ordering"]:
			base_affiliations = self.get_base_affiliations()
			index = [aff.names[0] for aff in base_affiliations].index(item)
			self.full_frame_ordering.append(base_affiliations[index])

		# Load dimension
		for dim_id, dim_dict in json_dict["dimensions"].items():
			dimension = Dimension.from_dict(dim_id, dim_dict, json_dict)
			if dimension is not None:
				self.dimensions[dimension.id_code] = dimension

		# Load status
		for status_id, status_dict in json_dict.get("statuses", {}).items():
			status = Status.from_dict(status_id, status_dict)
			if status is not None:
				self.statuses[status.id_code] = status

		for hqtfd_id, hqtfd_dict in json_dict.get("hqtfds", {}).items():
			hqtfd = HQTFD.from_dict(hqtfd_id, hqtfd_dict)
			if hqtfd is not None:
				self.hqtfds[hqtfd.id_code] = hqtfd

		self.print_constants()
		return True