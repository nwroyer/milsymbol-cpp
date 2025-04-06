from schema import *

class Symbol():
	def __init__(self):
		self.context:Context = None
		self.affiliation:Affiliation = None
		self.status:Status = None
		self.hqtfd:HQTFD = None
		self.amplifier:Amplifier = None
		self.symbol_set:SymbolSet = None
		self.entity:Entity = None
		self.modifier_1:Modifier = None
		self.modifier_2:Modifier = None

		self.frame_shape_override = None

	def __repr__(self):
		ret = ', '.join([
			'Symbol',
			f'context = {self.context.names[0]}',
			f'affiliation = {self.affiliation.names[0]} [{self.affiliation.id_code}]',
			f'symbol set = {self.symbol_set.names[0]} [{self.symbol_set.id_code}]',
			f'status = {self.status.names[0]} [{self.status.id_code}]',
			f'HQTFD = {self.hqtfd.names[0]} [{self.hqtfd.id_code}]',
			f'amplifier = {self.amplifier.names[0]} [{self.amplifier.id_code}]',
			f'entity = {self.entity.names[0]} [{self.entity.id_code}]' if self.entity is not None else 'entity = none',
		] 
			+ ([] if self.frame_shape_override is None else [f'frame shape override = {self.frame_shape_override.names[0]}'])
			+ ([] if self.modifier_1 is None else [f'modifier 1 = {self.modifier_1.names[0]}'])
			+ ([] if self.modifier_2 is None else [f'modifier 2 = {self.modifier_2.names[0]}'])
		)
		return ret

	@classmethod
	def from_sidc(cls, sidc:str, schema:Schema):
		if len(sidc) < 20:
			raise Exception(f'SIDC "{sidc}" must be at least 20 characters')
		if schema is None:
			raise Exception('No schema supplied')

		ret = cls()
		
		# Digits 0,1 are version
		
		# Digit 2 is the context
		ret.context = schema.contexts.get(sidc[2], schema.contexts['0'])

		# Digit 3 is the affiliation
		ret.affiliation = schema.affiliations.get(sidc[3], schema.affiliations['0'])

		# Digit 6 is the status
		ret.status = schema.statuses.get(sidc[6], schema.statuses['0'])

		# Digit 7 is the HQTFD code
		ret.hqtfd = schema.hqtfds.get(sidc[7], schema.hqtfds['0'])

		# Digits 8-9 are the amplifier
		ret.amplifier = schema.amplifiers.get(sidc[8:10], schema.amplifiers['00'])

		# Digits 4-5 are the symbol set
		ret.symbol_set = schema.symbol_sets.get(sidc[4:6], None)
		if ret.symbol_set is None:
			print(f'Unknown symbol set \"{sidc[4:6]}\"')
			return ret

		# Digits 10-15 are the 
		entity_code:str = sidc[10:16]
		fallbacks:list = [entity_code, f'{entity_code[0:4]}00', f'{entity_code[0:2]}0000']
		for code_index, code in enumerate(fallbacks):
			if code in ret.symbol_set.entities:
				ret.entity = ret.symbol_set.entities[code]
				break

			print(f'Entity "{entity_code}" not found in symbol set "{ret.symbol_set.names[0]}"; falling back to {fallbacks[code_index + 1] if code_index < 2 else '000000'}', file=sys.stderr)			

		mod_1_set:SymbolSet = ret.symbol_set
		mod_2_set:SymbolSet = ret.symbol_set

		if len(sidc) >= 22:
			if bool(int(sidc[20])): # Digit 20 indicates the sector 1 modifier is common
				mod_1_set = schema.symbol_sets.get('C', mod_1_set)
			if bool(int(sidc[21])): # Digit 21 indicates the sector 2 modifier is common
				mod_2_set = schema.symbol_sets.get('C', mod_2_set)

		if len(sidc) >= 23:
			ret.frame_shape_override = schema.frame_shapes.get(sidc[22], None) # Digit 22 is the frame shape override

		ret.modifier_1 = mod_1_set.m1.get(sidc[16:18], None)
		ret.modifier_2 = mod_2_set.m2.get(sidc[18:20], None)

		return ret

	def get_svg(self, style:str = 'light') -> str:
		# Assemble elements
		elements:list = []

		frame_to_use = self.frame_shape_override if self.frame_shape_override is not None else \
			self.symbol_set.dimension.frame_shape

		SVG_NAMESPACE:str = "http://w3.org/2000/svg";
		elements += frame_to_use.frames[self.affiliation.frame_id]	

		return ''


if __name__ == '__main__':
	TEST_SIDCS:list = [
		'130310001411060000600000000000',
		'130310000011092000600000000000',
		'130560000011020000000000000000',
		'130310021316040007891000000000'
	]

	schema = Schema.parse_from_directory(os.path.join(os.path.dirname(__file__), '..', 'schema'))

	for sidc in TEST_SIDCS:
		symbol = Symbol.from_sidc(sidc=sidc, schema=schema)
		print(symbol)
		svg = symbol.get_svg()
		


