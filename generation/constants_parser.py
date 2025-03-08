import json
import os
import re

def is_valid_hex_key(key:str, required_length:int=-1) -> bool:
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
	def from_dict(id_code:str, json:dict, constants):
		if not is_valid_hex_key(id_code, 1):
			print(f'Bad context ID {id_code}')
			return None

		context:Context = Context()
		context.id_code = id_code
		context.names = json['names']
		context.base_context = json.get('base context', id_code)
		constants.contexts[context.id_code] = context
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
	def from_dict(id_code:str, json:dict, constants):
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
			if len([c for c in constants.color_modes if c in json['colors']]) != len(constants.color_modes):
				print(f'Not all colors [{",".join(constants.color_modes)}] found for {affiliation.id_code}', file=sys.stderr)
				return None

			affiliation.colors = {color_id: json['colors'][color_id] for color_id in constants.color_modes}

		constants.affiliations[affiliation.id_code] = affiliation
		return affiliation

	def get_base_frame_affiliation(self, constants):
		if not self.frame_id:
			return self

		if self.frame_id not in constants.affiliations:
			print(f"No base frame ID {self.frame_id} found for {self}", file=sys.stderr)
			return None

		return constants.affiliations[self.frame_id]

class Dimension:
	""" 
	Represents a dimension, which sets the frame type
	"""

	def __init__(self):
		self.id_code:str = ""  # Human readable name for the dimension
		self.frames:dict = {}  # Dictionary of frames for IDs

	def __repr__(self):
		return f'Dimension \"{self.id_code}\"'

	@staticmethod
	def from_dict(id_code:str, json:dict, over_dict:dict, constants):
		dimension:Dimension = Dimension()
		dimension.id_code = id_code

		def create_base_frames(json:dict, over_dict:dict) -> dict:
			ret = {}
			if 'frame base' in json:
				base_dim:str = json['frame base']
				if json['frame base'] not in over_dict:
					raise Exception(f"No dimension \"{json['frame_base']}\" defined")
					return None

				ret = create_base_frames(json=over_dict[base_dim], over_dict=over_dict)

			# Apply base frame
			for frame_key, frame_list in json.get("frames", {}).items():
				ret[frame_key] = [f for f in frame_list]

			# Apply frame decorators
			for frame_key, frame_list in json.get("decorators", {}).items():
				if frame_key in dimension.frames:
					dimension.frames[frame_key].extend(frame_list)
				else:
					dimension.frames[frame_key] = [f for f in frame_list]

			return ret


		dimension.frames = create_base_frames(json=json, over_dict=over_dict['dimensions'])
		if dimension.frames is None:
			raise Exception('Ex')
			return None
		constants.dimensions[id_code] = dimension
		#print(f'Dim {dimension.id_code}: {dimension.frames}')
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
	def from_dict(id_code:str, json:dict, constants):
		if not is_valid_hex_key(id_code, 1):
			print(f"Bad status {id_code}", file=sys.stderr)
			return None			

		status:Status = Status()
		status.id_code = id_code
		status.names = json.get('names', [])
		status.dashed = json.get("dashed", False)
		constants.statuses[status.id_code] = status
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
	def from_dict(id_code:str, json:dict, constants):
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
		constants.hqtfds[hqtfd.id_code] = hqtfd
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
	def from_dict(id_code:str, json:dict, constants):
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
			if apt not in constants.dimensions:
				print(f"Bad applies to dimension \"{apt}\" for amplifier {amplifier.id_code}", file=sys.stderr)
				return None

		return amplifier

class Constants:
	"""
	Represents the constants
	"""

	def __init__(self):
		self.dimensions:dict = {}
		self.color_modes:list = []
		self.contexts:dict = {}
		self.affiliations:dict = {}
		self.statuses:dict = {}
		self.hqtfds:dict = {}
		self.full_frame_ordering:list = []

	def print_self(self):
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
			if aff.get_base_frame_affiliation(constants=self) == aff:
				ret.append(aff)

		ret = sorted(ret, key=lambda x: x.id_code)
		return ret

	def get_base_affiliation_dict(self) -> list:
		return {ret.names[0]: ret for ret in self.get_base_affiliations()}

def parse_constant_file(filepath:str) -> Constants:
	constants = Constants()

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
	for context_id, context_dict in json_dict["contexts"].items():
		context = Context.from_dict(context_id, context_dict, constants)

	# Load color modes
	for color_mode in json_dict['color modes']:
		constants.color_modes.append(color_mode.lower())

	# Load affiliations
	for aff_id, aff_dict in json_dict["affiliations"].items():
		affiliation = Affiliation.from_dict(aff_id, aff_dict, constants)

	# Load full frame ordering
	constants.full_frame_ordering = []
	for item in json_dict["full frame ordering"]:
		base_affiliations = constants.get_base_affiliations()
		index = [aff.names[0] for aff in base_affiliations].index(item)
		constants.full_frame_ordering.append(base_affiliations[index])

	# Load dimension
	for dim_id, dim_dict in json_dict["dimensions"].items():
		dimension = Dimension.from_dict(dim_id, dim_dict, json_dict, constants)

	# Load status
	for status_id, status_dict in json_dict.get("statuses", {}).items():
		status = Status.from_dict(status_id, status_dict, constants)

	for hqtfd_id, hqtfd_dict in json_dict.get("hqtfds", {}).items():
		hqtfd = HQTFD.from_dict(hqtfd_id, hqtfd_dict, constants)

	constants.print_self()
	return constants
