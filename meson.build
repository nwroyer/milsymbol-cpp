# Define the project
project('milsymbol', 'cpp',
	default_options: ['buildtype=debug', 'cpp_std=c++20', 'warning_level=0'],
	version: '1.0.0'
)

# Source files - auto-generated with another glob file
cpp_and_c_sources = [
    'src/example.cpp',
    'src/Symbol.cpp',
    'src/DrawCommands.cpp'
]

cpp_and_c_headers = [
    'include/Types.hpp',
    'include/Colors.hpp',
    'include/SymbolGeometries.hpp',
    'include/BBox.hpp',
    'include/Symbol.hpp',
    'include/DrawCommands.hpp',
    'include/Schema.hpp',
    'include/Constants.hpp',
    'include/eternal.hpp',
    'include/SymbolStyle.hpp'
]

# Gather source files
include_directories = [
    'include'
]

# Create the targets
example_target = executable('milsymbol-example',
    sources: [cpp_and_c_sources, cpp_and_c_headers],
    include_directories: include_directories,
    override_options : ['cpp_std=c++20', 'warning_level=0'],
    extra_files: cpp_and_c_headers
)

# Library target
library_target = library('milsymbol',
    sources: ['src/Symbol.cpp', 'src/DrawCommands.cpp'],
    include_directories: include_directories,
    override_options: ['cpp_std=c++20', 'warning_level=0'],
    extra_files: ['include/Symbol.hpp', 'include/Types.hpp', 'include/Constants.hpp'],
    install: true
)

# Headers for install
install_headers(
    'include/Symbol.hpp',
    'include/Types.hpp',
    'include/Constants.hpp',
    subdir: 'milsymbol'
)
