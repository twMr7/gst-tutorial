# Python Development Environment Setup for GStreamer

The default method for installing packages from distros is normally fine.
[venv](https://docs.python.org/3/library/venv.html) is usually the way to go instead
if you need to `pip install` the latest packages and want to avoid conflicts with system packages.
.

## Gst-Python Installation for System Python

Install GStreamer GObject Introspection overrides for Python 3 (*python3-gst-1.0*).
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
/usr/lib/python3/dist-packages/gi/overrides/_gi_gst.cpython-312-x86_64-linux-gnu.so
/usr/share
/usr/share/doc
/usr/share/doc/python3-gst-1.0
/usr/share/doc/python3-gst-1.0/changelog.Debian.gz
/usr/share/doc/python3-gst-1.0/copyright
```

## Gst-Python Installation for Venv

Download matched version:
- Ubuntu 22.04 : [gst-python-1.20](https://gstreamer.freedesktop.org/src/gst-python/gst-python-1.20.7.tar.xz)
- Ubuntu 24.04 : [gst-python-1.24](https://gstreamer.freedesktop.org/src/gst-python/gst-python-1.24.9.tar.xz)

Make sure you also have *python-gi-dev* installed on system.

```shell
$ sudo apt install python-gi-dev
```

### Build The Shared Module

```shell
$ cd gst-python-1.24.9
$ meson setup builddir
$ meson compile -v -C builddir
```

I haven't figured out how to configure the installation path yet, so
just copy files manually to the correct override location.

```shell
$ cp gi/overrides/Gst*.py <your-venv-root>/lib/python3.12/site-packages/gi/overrides/
$ cp builddir/gi/overrides/_gi_gst.cpython-312-x86_64-linux-gnu.so <your-venv-root>/lib/python3.10/site-packages/gi/overrides/
```
