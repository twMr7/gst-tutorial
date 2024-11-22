project('basic04', 'c', 'cpp')

gst_dep = dependency('gstreamer-1.0')
executable('basic04c', ['basic-tutorial-4.c'], dependencies: gst_dep)

gstmm_dep = [dependency('gstreamermm-1.0'), dependency('glibmm-2.4')]
executable('basic04cpp', ['basic-tutorial-4.cpp'], dependencies: gstmm_dep)
