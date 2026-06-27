project('kommando', 'cpp', version: '0.0.1', default_options: ['cpp_std=c++11'])

sources = []

kommando_lib = library('kommando', sources: sources)
kommando_dep = declare_dependency(link_with: kommando_lib)

executable('sandbox', sources: 'sandbox/main.cpp', dependencies: kommando_dep)