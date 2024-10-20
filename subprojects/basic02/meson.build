project('basic02', 'c', 'cpp')

gst_dep = dependency('gstreamer-1.0')
executable('basic02c', ['basic-tutorial-2.c'], dependencies: gst_dep)

gstmm_dep = [dependency('gstreamermm-1.0'), dependency('glibmm-2.4')]
executable('basic02cpp', ['basic-tutorial-2.cpp'], dependencies: gstmm_dep)
