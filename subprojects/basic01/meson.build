project('basic01', 'c', 'cpp')

gst_dep = dependency('gstreamer-1.0')
executable('basic01c', ['basic-tutorial-1.c'], dependencies: gst_dep)

gstmm_dep = [dependency('gstreamermm-1.0'), dependency('glibmm-2.4')]
executable('basic01cpp', ['basic-tutorial-1.cpp'], dependencies: gstmm_dep)
