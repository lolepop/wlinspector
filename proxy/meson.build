project('wlinspector', 'c', 'cpp', default_options : ['c_std=c11', 'cpp_std=c++20'])

if get_option('buildtype').startswith('debug')
  add_project_arguments('-DDEBUG', language : 'cpp')
endif

libsdbusplus_proj = subproject('sdbusplus')
libsdbusplus_dep = libsdbusplus_proj.get_variable('sdbusplus_dep')

deps = [
    dependency('wayland-client'),
    libsdbusplus_dep
]

library('wlinspectorproxy', 'src/main.cpp', dependencies: deps)
