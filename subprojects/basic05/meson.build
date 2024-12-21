project('basic05', 'c', 'cpp')

gst_dep = dependency('gstreamer-1.0')
gtk_dep = dependency('gtk+-3.0')
executable('basic05c', ['basic-tutorial-5.c'], dependencies: [gst_dep, gtk_dep])

gstmm_dep = [dependency('gstreamermm-1.0'), dependency('glibmm-2.4')]
gtkmm_dep = dependency('gtkmm-3.0')
executable('basic05cpp', ['basic-tutorial-5.cpp'], dependencies: [gstmm_dep, gtkmm_dep])
