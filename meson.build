# Define the project
project('milsymbol', 'cpp',
	default_options: ['buildtype=debug', 'cpp_std=c++20', 'warning_level=0'],
	version: '1.0.0'
)

# Source files - auto-generated with another glob file
cpp_and_c_sources = [
    'cpp/src/example.cpp',
    'cpp/src/Symbol.cpp',
    'cpp/src/DrawCommands.cpp'
]

cpp_and_c_headers = [
    'cpp/include/Types.hpp',
    'cpp/include/Colors.hpp',
    'cpp/include/BBox.hpp',
    'cpp/include/Symbol.hpp',
    'cpp/include/DrawCommands.hpp',
    'cpp/include/Schema.hpp',
    'cpp/include/Constants.hpp',
    'cpp/include/eternal.hpp',
    'cpp/include/SymbolStyle.hpp'
]

# Gather source files
include_directories = [
    'cpp/include'
]

# C++ example target
example_target = executable('milsymbol-example',
    sources: [cpp_and_c_sources, cpp_and_c_headers],
    include_directories: include_directories,
    override_options : ['cpp_std=c++20', 'warning_level=0'],
    extra_files: cpp_and_c_headers
)

# C++ library target
library_target = library('milsymbol',
    sources: ['cpp/src/Symbol.cpp', 'cpp/src/DrawCommands.cpp'],
    include_directories: include_directories,
    override_options: ['cpp_std=c++20', 'warning_level=0'],
    extra_files: ['cpp/include/Symbol.hpp', 'cpp/include/Types.hpp', 'cpp/include/Constants.hpp'],
    install: true
)

# Headers for install
install_headers(
    'cpp/include/Symbol.hpp',
    'cpp/include/Types.hpp',
    'cpp/include/Constants.hpp',
    subdir: 'milsymbol'
)

milsymbol_cpp = declare_dependency(
    link_with: library_target,
    include_directories: include_directories
)
