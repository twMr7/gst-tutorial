# Python Development Environment Setup for GStreamer

The default method for installing packages from distros is normally fine.
[venv](https://docs.python.org/3/library/venv.html) is usually the way to go instead
if you need to `pip install` the latest packages and want to avoid conflicts with system packages.
.

## Gst-Python Installation for System Python

Install GStreamer GObject Introspection overrides for Python 3.
Note that this package works for system-level of Python, see the next section for Python venv.

```shell
$ sudo apt install python3-gst-1.0
$ dpkg -L python3-gst-1.0
/.
/usr
/usr/lib
/usr/lib/python3
/usr/lib/python3/dist-packages
/usr/lib/python3/dist-packages/gi
/usr/lib/python3/dist-packages/gi/overrides
/usr/lib/python3/dist-packages/gi/overrides/Gst.py
/usr/lib/python3/dist-packages/gi/overrides/GstAudio.py
/usr/lib/python3/dist-packages/gi/overrides/GstPbutils.py
/usr/lib/python3/dist-packages/gi/overrides/GstVideo.py
/usr/lib/python3/dist-packages/gi/overrides/_gi_gst.cpython-310-x86_64-linux-gnu.so
/usr/share
/usr/share/doc
/usr/share/doc/python3-gst-1.0
/usr/share/doc/python3-gst-1.0/changelog.Debian.gz
/usr/share/doc/python3-gst-1.0/copyright
```

## Gst-Python Installation for Venv

Download matched version:
- [gst-python-1.20](https://gstreamer.freedesktop.org/src/gst-python/gst-python-1.20.0.tar.xz)

```shell
$ cd gst-python-1.20.0
$ meson setup builddir
The Meson build system
Version: 1.5.2
...
Run-time dependency pygobject-3.0 found: NO (tried pkgconfig and cmake)
Looking for a fallback subproject for the dependency pygobject-3.0

meson.build:23:16: ERROR: Neither a subproject directory nor a pygobject.wrap file was found.
```

```shell
$ sudo apt install python-gi-dev
$ dpkg -L python-gi-dev
/.
/usr
/usr/include
/usr/include/pygobject-3.0
/usr/include/pygobject-3.0/pygobject.h
/usr/lib
/usr/lib/x86_64-linux-gnu
/usr/lib/x86_64-linux-gnu/pkgconfig
/usr/lib/x86_64-linux-gnu/pkgconfig/pygobject-3.0.pc
/usr/share
/usr/share/doc
/usr/share/doc/python-gi-dev
/usr/share/doc/python-gi-dev/copyright
/usr/share/doc/python-gi-dev/changelog.Debian.gz
```

```shell
$ meson setup builddir
The Meson build system
Version: 1.5.2
Source dir: /home/jchang/Downloads/gst-python-1.20.0
Build dir: /home/jchang/Downloads/gst-python-1.20.0/builddir
Build type: native build
Project name: gst-python
Project version: 1.20.0
C compiler for the host machine: ccache cc (gcc 11.4.0 "cc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0")
C linker for the host machine: cc ld.bfd 2.38
C++ compiler for the host machine: ccache c++ (gcc 11.4.0 "c++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0")
C++ linker for the host machine: c++ ld.bfd 2.38
Host machine cpu family: x86_64
Host machine cpu: x86_64
Found pkg-config: YES (/usr/bin/pkg-config) 0.29.2
Run-time dependency gstreamer-1.0 found: YES 1.20.3
Run-time dependency gstreamer-base-1.0 found: YES 1.20.3
Run-time dependency gmodule-2.0 found: YES 2.72.4
Run-time dependency pygobject-3.0 found: YES 3.42.1
Program python3 found: YES (/home/jchang/.venv/mr7/bin/python3)
Run-time dependency python found: YES 3.10
Message: python_abi_flags = 
Message: pylib_loc = /usr/lib/python3.10/config-3.10-x86_64-linux-gnu
Message: pygobject overrides directory = /usr/local/lib/python3/dist-packages/gi/overrides
Configuring config.h using configuration
Program pkg-config found: YES (/usr/bin/pkg-config)
Run-time dependency gstreamer-plugins-base-1.0 found: YES 1.20.1
Build targets in project: 2

Found ninja-1.10.1 at /usr/bin/ninja

$ ninja -v -C builddir
```

```shell
$ meson configure -Dpygi-overrides-dir="~/.venv/mr7/lib/python3.10/site-packages/gi/overrides"

ERROR: No valid build directory found, cannot modify options.
```

copy files manually to the corrrect override location.

```shell
$ cp gi/overrides/Gst*.py ~/.venv/mr7/lib/python3.10/site-packages/gi/overrides/
$ cp builddir/gi/overrides/_gi_gst.cpython-310-x86_64-linux-gnu.so ~/.venv/mr7/lib/python3.10/site-packages/gi/overrides/
```
