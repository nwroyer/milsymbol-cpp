
# code_start = 4
# for i in range(1, 10):
# 	staff = '''\t"{code:02}": {{
# 			"names": ["functional staff area J-{i}", "G-{i}", "S-{i}"],
# 			"cat": "capability",
# 			"icon": [{{"textm2": "J{i}"}}]
# 	}},'''.format(code=code_start + i - 1, i=i)

# 	print(staff)

# code_start = 13
# for i in range(3, 12):
# 	staff = '''\t"{code:02}": {{
# 		"names": ["rank O-{i}", "O-{i}", "OF-{of}", "rank OF-{of}"],
# 		"cat": "capability",
# 		"icon": [{{"textm2": "O-{i}"}}],
# 		"alt icon": [{{"textm2": "OF-{of}"}}]
# 	}},'''.format(code=code_start + i - 1, i=i, of=i-1)

# 	print(staff)

# code_start = 25
# for index, i in enumerate(range(1, 10)):
# 	staff = '''\t"{code:02}": {{
# 		"names": ["rank E-{i}", "E-{i}", "OR-{i}", "rank OR-{i}"],
# 		"cat": "command level",
# 		"icon": [{{"textm2": "E-{i}"}}],
# 		"alt icon": [{{"textm2": "OR-{i}"}}]
# 	}},'''.format(code=code_start + index, i=i)

# 	print(staff)

code_start = 34
for index, i in enumerate(range(1, 6)):
	staff = '''\t"{code:02}": {{
		"names": ["rank WO-{i}", "W-{i}", "rank W-{i}", "WO-{i}"],
		"cat": "capability",
		"icon": [{{"textm2": "WO-{i}"}}]
	}},'''.format(code=code_start + index, i=i)

	print(staff)