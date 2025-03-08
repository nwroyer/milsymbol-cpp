from font_rendering import Font
from constants_parser import Constants

"""
The default stroke to use for symbols
"""
DEFAULT_STROKE_WIDTH:float = 4.0

"""
The default font file to use
"""
DEFAULT_FONT_FILE:str = 'SimplySans-Bold.ttf'

"""
Converts a color to the appropriate C++ constant
"""
def color_type_to_cpp(color_type) -> str:
	if color_type is None:
		return 'ColorType::NONE'
	else:
		return f'ColorType::{color_type.upper()}'

"""
Acceptable values for colors in the JSON schema. Right now
yellow is only used for missile icons and chemical spills.
"""
COLORS:list = {
	'icon', 'icon_fill', 'white', 'yellow'	
}

"""
Convert color in the JSON schema to one of our
defined items from the COLORS constant
"""
def convert_color(item):
	if type(item) is bool:
		return 'icon' if item else 'none'
	elif type(item) is str:
		item = item.lower()
		if not(item in COLORS):
			print(f'Unrecognized color "{item}"', file=sys.stderr)
			return None
		return item
	else:
		print(f"Bad color: {item}", file=sys.stderr)
		return None

"""
Class for defining an output style for the generated C++ code
"""
class OutputStyle:
	def __init__(self, use_text_paths:bool = False):
		self.use_text_paths = use_text_paths
		self.text_path_font = DEFAULT_FONT_FILE

"""
A basic symbol element
"""
class SymbolElement:

	"""
	Base class that contains styling elements
	"""
	class Base:
		def __init__(self):
			self.fill_color:str = None
			self.stroke_color:str = "icon"
			self.stroke_width:float = DEFAULT_STROKE_WIDTH

		def base_params(self) -> str:
			return 'fill="{}" stroke="{}"{}'.format(
				self.fill_color if self.fill_color is not None and self.fill_color != '' else 'none',
				self.stroke_color if self.stroke_color is not None and self.fill_color != '' else 'none',
				f' stroke_width="{self.stroke_width}"' if self.stroke_color is not None and self.stroke_color != '' else ''
			)

		def parse_basics(self, element) -> None:
			if 'fill' in element:
				self.fill_color = element['fill']
				if type(self.fill_color) == str and self.fill_color.lower() == 'none':
					self.fill_color = None
					
				if type(self.fill_color) == bool:
					self.fill_color = 'icon' if self.fill_color else None

			if 'stroke' in element:
				self.stroke_color = element['stroke']
				if type(self.stroke_color) == bool:
					self.stroke_color = 'icon' if self.stroke_color else None

			if 'strokewidth' in element:
				self.stroke_width = float(element['strokewidth'])

	"""
	Full frame command
	"""
	class FullFrame(Base):
		def __init__(self, constants:Constants):
			super().__init__()
			self.elements:dict = {
				affil.id_code: [] for affil in constants.affiliations.values()			
			}

		def cpp(self, constants:Constants, output_style=OutputStyle(), with_bbox=False):
			ordering = [affiliation.id_code for affiliation in constants.full_frame_ordering]
			elements_ordered = [(item, self.elements[item]) for item in ordering]

			ret = 'DrawCommand::full_frame('

			items = []
			for affiliation in constants.full_frame_ordering:
				items.append('{' + ', '.join([e.cpp(constants=constants, output_style=output_style) for e in self.elements[affiliation.id_code]]) + '}')

			#print(constants.full_frame_ordering)

			ret += ', '.join(items)
			ret += ')'
			return ret

	"""
	Represents a path command
	"""
	class Path(Base):
		def __init__(self):
			super().__init__()
			self.d:str = '' # The SVG path
			self.bbox:tuple = (100, 100, 100, 100)
			self.fill_color = None # Default to an unfilled path
			self.stroke_color = "icon" # Default to a filled stroke

		def __repr__(self):
			return f'<path d="{self.d}" {self.base_params()} />'

		def cpp(self, constants:Constants, output_style=OutputStyle(), with_bbox=False) -> str:
			ret:str = 'DrawCommand::path(\"{}\", BoundingBox({}, {}, {}, {}))'.format(self.d, *self.bbox)
			if self.fill_color is not None:
				ret += '.with_fill({})'.format(color_type_to_cpp(self.fill_color))
			if self.stroke_color is None or self.stroke_color != 'icon':
				ret += '.with_stroke({})'.format(color_type_to_cpp(self.stroke_color))
			if self.stroke_width != DEFAULT_STROKE_WIDTH and self.stroke_color is not None:
				ret += '.with_stroke_width({})'.format(self.stroke_width)

			return ret



	"""
	Represents a circle command
	"""
	class Circle(Base):
		def __init__(self):
			super().__init__()
			self.pos:tuple = (100, 100)
			self.radius:float = 1
			self.fill_color = None
			self.stroke_color = "icon"

		def __repr__(self):
			return f'<circle cx="{self.pos[0]}" cy="{self.pos[1]}" radius="{self.radius}" {self.base_params()} />'

		def cpp(self, constants:Constants, output_style=OutputStyle(), with_bbox=False) -> str:
			ret:str = 'DrawCommand::circle(Vector2{{{}, {}}}, {})'.format(self.pos[0], self.pos[1], self.radius)
			if self.fill_color is not None:
				ret += '.with_fill({})'.format(color_type_to_cpp(self.fill_color))
			if self.stroke_color is None or self.stroke_color != 'icon':
				ret += '.with_stroke({})'.format(color_type_to_cpp(self.stroke_color))
			if self.stroke_width != DEFAULT_STROKE_WIDTH and self.stroke_color is not None:
				ret += '.with_stroke_width({})'.format(self.stroke_width)			
			return ret

	"""
	Represents a text command
	"""
	class Text(Base):
		def __init__(self):
			super().__init__()
			self.text:str = '' # The actual rendered text
			self.pos:tuple = (100, 100) # Text origin
			self.font_size:int = 12
			self.font_family:str = 'Arial'
			self.align:str = 'middle' # Can be ['left', 'middle', 'right']
			self.text_type:str = 'auto' # ['auto', 'manual', 'm1', 'm2']
			self.fill_color = 'icon' # Default to filled text
			self.stroke_color = None # Default to no stroke
			self.text_type = 'manual' # ['normal', 'm1', 'm2', 'manual']

		def __repr__(self):
			return f'<text x="{self.pos[0]}" y="{self.pos[1]}" font-size="{self.font_size}" font-anchor="{self.align}" {self.base_params()}>{self.text}</text>'

		def cpp(self, constants:Constants, output_style=OutputStyle(), with_bbox=False) -> str:

			"""
			If we're supposed to convert text to paths, do so here and
			then return
			"""
			if output_style.use_text_paths:
				font_face = Font(output_style.text_path_font, size = int(self.font_size))

				pos = self.pos
				size = self.font_size

				if self.text_type == 'normal':
					size = 42
					y = 115
					if len(self.text) == 1:
						size = 45
						y = 115
					elif len(self.text) == 3:
						size = 35
						y = 110
					elif len(self.text) >= 4:
						size = 32
						y = 110
					pos = (100, y)
				elif self.text_type == 'm1':
					pos = (100, 77)
					size = 30
					if len(self.text) == 3:
						size = 25
					elif len(self.text) >= 4:
						size = 22
				elif self.text_type == 'm2':
					y = 145
					size = 30
					if len(self.text) == 3:
						size = 25
						y = 140
					elif len(self.text) >= 4:
						size = 22
						y = 135
					pos = (100, y)
				else:
					pos = tuple(self.pos)
					size = self.font_size

				paths = font_face.render_text(
					text = self.text, 
					pos = pos,
					fontsize = int(size),
					align = self.align)
				
				ret_path = ' '.join(paths)
				path_el = SymbolElement.Path()
				path_el.fill_color = self.fill_color
				path_el.stroke_color = self.stroke_color
				path_el.d = ret_path
				return path_el.cpp(constants=Constants)

			# Default text-as-text rendition
			ret:str = ''
			if self.text_type == 'normal':
				ret = 'DrawCommand::autotext("{}")'.format(self.text)
			elif self.text_type == 'm1':
				ret = 'DrawCommand::textm1("{}")'.format(self.text)
			elif self.text_type == 'm2':
				ret = 'DrawCommand::textm2("{}")'.format(self.text)
			else:
				ret = 'DrawCommand::text("{}", Vector2{{{}, {}}}, {})'.format(self.text, self.pos[0], self.pos[1], self.font_size)

			if self.fill_color is None or self.fill_color != 'icon':
				ret += '.with_fill({})'.format(color_type_to_cpp(self.fill_color))
			if self.stroke_color is not None:
				ret += '.with_stroke({})'.format(color_type_to_cpp(self.stroke_color))
			if self.stroke_width != DEFAULT_STROKE_WIDTH and self.stroke_color is not None:
				ret += '.with_stroke_width({})'.format(self.stroke_width)

			return ret

	"""
	Base class for transformation
	"""
	class Transformation(Base):
		def __init__(self):
			super().__init__()
			self.items:list = []

	"""
	Represents a translation
	"""
	class Translate(Transformation):
		def __init__(self):
			super().__init__()
			self.delta:tuple = (0, 0)

		def __repr__(self):
			return '<g transform=\"translate({} {})\">{}</g>'.format(
				self.delta[0],
				self.delta[1],
				' '.join([str(item) for item in self.items])
			)

		def cpp(self, constants:Constants, output_style=OutputStyle(), with_bbox=False) -> str:
			return 'DrawCommand::translate(Vector2{{{}, {}}}, {})'.format(
				self.delta[0], self.delta[1],
				', '.join([x.cpp(constants=constants, output_style=output_style, with_bbox=with_bbox) for x in self.items])
			)

	"""
	Represents a scaling
	"""
	class Scale(Transformation):
		def __init__(self):
			super().__init__()
			self.scale:float = 1.0

		def __repr__(self):
			return '<g transform=\"scale({})\">{}</g>'.format(
				self.scale,
				' '.join([str(item) for item in self.items])
			)

		def cpp(self, constants:Constants, output_style=OutputStyle(), with_bbox=False):
			return 'DrawCommand::scale({}, {})'.format(
				self.scale,
				', '.join([x.cpp(constants=constants, output_style=output_style, with_bbox=with_bbox) for x in self.items])
			)

"""
A full symbol component (e.g. an entity or modifier)
"""
class SymbolLayer:
	def __init__(self):
		self.uid:str = '' # Canonical unique name
		self.names:str = [] # Human-readable names
		self.elements:list = []
		self.civilian:bool = False
		pass

	def __repr__(self):
		return '{{{}}} -> {}'.format(self.uid, self.elements)

	def cpp(self, constants:Constants, output_style=OutputStyle(), with_bbox=False):
		return 'SymbolLayer{{{}}}{}'.format(
			', '.join([cmd.cpp(output_style=output_style, constants=constants, with_bbox=with_bbox) for cmd in self.elements]),
			'.with_civilian_override(true)' if self.civilian else ''
		)