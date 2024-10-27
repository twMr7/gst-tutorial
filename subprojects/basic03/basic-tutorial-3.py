#!/usr/bin/env python3

"""
Basic Tutorial 3: Dynamic Pipelines
"""

import sys
import logging

import gi
gi.require_version("Gst", "1.0")
gi.require_version("GLib", "2.0")
from gi.repository import GLib, Gst

logging.basicConfig(level=logging.INFO, format="[%(levelname)8s] - %(message)s")
logger = logging.getLogger(__name__)

class CustomData:
    """CustomData contains all the information needed to pass to callbacks
    """
    def __init__(self, pipeline, source, convert, resample, sink):
        self.pipeline = pipeline
        self.source = source
        self.convert = convert
        self.resmaple = resample
        self.sink = sink


def on_pad_added(src, new_pad, data):
    """This function will be called by the pad-added signal
    """
    sink_pad = data.convert.get_static_pad("sink")
    # If our converter is already linked, we have nothing to do here
    if (sink_pad.is_linked()):
        logger.error("We are already linked. Ignoring.")
        return

    # Retrieves the current capabilities of the new pad
    new_pad_caps = new_pad.get_current_caps()
    logger.info(f"Received new pad '{new_pad_caps[0].get_name()}' from '{src.get_name()}'")

    # Check the new pad's type
    new_pad_struct = new_pad_caps.get_structure(0)
    new_pad_type = new_pad_struct.get_name()
    if not new_pad_type.startswith("audio/x-raw"):
        logger.info(f"It has type '{new_pad_type}' which is not raw audio. Ignoring.")
        return

    try:
        new_pad.link(sink_pad)
    except:
        logger.error(f"Type is '{new_pad_type}' but link failed.")
    else:
        logger.info(f"Link succeeded (type '{new_pad_type}')")


def on_bus_message(bus, message, loop):
    if message.type == Gst.MessageType.EOS:
        logger.info("End-of-stream reached.")
        loop.quit()
    elif message.type == Gst.MessageType.ERROR:
        err, debug_info = message.parse_error()
        logger.error(f"Error received from element {message.src.get_name()}: {err.message}")
        logger.error(f"Debugging information: {debug_info if debug_info else 'none'}")
        loop.quit()
    elif message.type == Gst.MessageType.STATE_CHANGED:
        # We are only interested in state-changed messages from the pipeline
        old_state, new_state, pending_state = message.parse_state_changed()
        if ("test-pipeline" == message.src.get_name()):
            logger.info(f"Pipeline state changed: {old_state.value_nick} -> {new_state.value_nick}")
    else:
        logger.debug(f"Unhandled message type {message.type}.")
    return True


def main(argv):
    # Initialize GStreamer
    Gst.init(None)

    uri = "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm"
    if len(argv) != 2:
        logger.info(f"usage: {argv[0]} <uri>")
        logger.info(f"missing uri argument, use default uri instead.")
    elif Gst.uri_is_valid(argv[1]):
        uri = argv[1]

    # Create the elements
    source = Gst.ElementFactory.make("uridecodebin", "source")
    convert = Gst.ElementFactory.make("audioconvert", "convert")
    resample = Gst.ElementFactory.make("audioresample", "resample")
    sink = Gst.ElementFactory.make("autoaudiosink", "sink")

    # Create the empty pipeline
    pipeline = Gst.Pipeline.new("test-pipeline")

    # Build the pipeline.
    pipeline.add(source, convert, resample, sink)
    # Note that we are NOT linking the source at this point. We will do it later
    Gst.Element.link_many(convert, resample, sink)
    # Set the URI to play
    source.set_property("uri", uri)
    # Prepare data for callback
    pdata = CustomData(pipeline, source, convert, resample, sink)
    # Connect to the pad-added signal
    source.connect("pad-added", on_pad_added, pdata)

    loop = GLib.MainLoop()

    # Listen to the bus
    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect("message", on_bus_message, loop)

    # start play back and listen to events
    ret = pipeline.set_state(Gst.State.PLAYING)
    if ret == Gst.StateChangeReturn.FAILURE:
        logger.error("Unable to set the pipeline to the playing state.")
        return -1

    try:
        loop.run()
    except:
        logger.error("Unhandled exception.")

    # cleanup
    pipeline.set_state(Gst.State.NULL)


if __name__ == '__main__':
    sys.exit(main(sys.argv))
