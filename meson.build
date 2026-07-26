project(
    'kommando',
    'c',
    version: '0.0.1',
    default_options: ['c_std=c23', 'default_library=both'],
)

sources = ['src/args.c', 'src/list.c']

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

executable('sandbox', sources: 'sandbox/main.c', dependencies: kommando_dep)
