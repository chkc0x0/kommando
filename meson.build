project(
    'kommando',
    'cpp',
    version: '0.0.1',
    default_options: ['cpp_std=c++20'],
)

sources = []

host_os = host_machine.system()

if host_os == 'windows'
    # windows stuff later
else
    sources += ['src/core/terminal_posix.cpp']
endif

kommando_lib = library(
    'kommando',
    sources: sources,
    include_directories: include_directories('kommando'),
    install: true,
)

kommando_dep = declare_dependency(
    link_with: kommando_lib,
    include_directories: include_directories('kommando'),
)

executable(
    'sandbox',
    sources: 'sandbox/main.cpp',
    dependencies: kommando_dep
)