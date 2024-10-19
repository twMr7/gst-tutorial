# GStreamer Tutorial

This repository contains *GStreamer* tutorial examples for C, C++, and Python.
The examples' source files are originally from the official repository of the respective programming languages.

- **C**: copied from the Gstreamer's repository - 
[tutorials](https://gitlab.freedesktop.org/gstreamer/gstreamer/-/tree/main/subprojects/gst-docs/examples/tutorials)
- **C++**: modified from the examples in
[gstreamermm-1.10](https://download.gnome.org/sources/gstreamermm/).
- **Python**: modified from the examples in the macthing version of
[gst-python source](https://gstreamer.freedesktop.org/src/gst-python/)

## Development Environment Setup

All codes are built and verified on Ubuntu 22.04.
[`Meson`](https://mesonbuild.com/) is used to build all the C and C++ codes.

GStreamer is built on top of the `GObject` and `GLib`.
Refer to the GStreamer documentation for the
[installation guide](https://gstreamer.freedesktop.org/documentation/installing/index.html)
to set up the C development environment.

Additional packages are required for C++.

```shell
$ sudo apt install libglibmm-2.4-1v5 libglibmm-2.4-dev libgstreamermm-1.0-1 libgstreamermm-1.0-dev
```

For Python, setup a working [PyGObject](https://pygobject.gnome.org/devguide/dev_environ.html) environment,
then install additional packages for GStreamer.

```shell
$ sudo apt python3-gst-1.0 gir1.2-gstreamer-1.0
```
