project('basic03', 'c', 'cpp')

gst_dep = dependency('gstreamer-1.0')
executable('basic03c', ['basic-tutorial-3.c'], dependencies: gst_dep)

gstmm_dep = [dependency('gstreamermm-1.0'), dependency('glibmm-2.4')]
executable('basic03cpp', ['basic-tutorial-3.cpp'], dependencies: gstmm_dep)
