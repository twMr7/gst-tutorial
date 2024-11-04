#!/usr/bin/env python3

"""
Supplement to Basic Tutorial 3: Dynamic Source

Simple example to demonstrate dynamically adding and removing source elements
to a playing pipeline.
"""

import sys
import logging

import gi
gi.require_version("Gst", "1.0")
gi.require_version("GLib", "2.0")
from gi.repository import GLib, Gst

logging.basicConfig(level=logging.INFO, format="[%(levelname)8s] - %(message)s")
logger = logging.getLogger(__name__)


class ProbeData:
    def __init__(self, pipeline, source, sink, index):
        self.pipeline = pipeline
        self.source = source
        self.sink = sink
        self.index = index


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


def on_timeout(pdata):
    pdata.source.set_state(Gst.State.NULL)
    pdata.pipeline.remove(pdata.source)
    # Create a new source
    pdata.source = Gst.ElementFactory.make("videotestsrc", "source")
    pdata.index = pdata.index + 1 if pdata.index < 25 else 0
    pdata.source.props.pattern = pdata.index
    pdata.source.props.is_live = True
    # rebuild the pipeline
    pdata.pipeline.add(pdata.source)
    pdata.source.link(pdata.sink)
    pdata.source.set_state(Gst.State.PLAYING)
    # start the next timeout
    GLib.timeout_add_seconds(1, on_timeout, pdata)


def main(argv):
    # Initialize GStreamer
    Gst.init(argv[1:])

    # Create the elements
    source = Gst.ElementFactory.make("videotestsrc", "source")
    sink = Gst.ElementFactory.make("autovideosink", "sink")

    # Create the empty pipeline
    pipeline = Gst.Pipeline.new("test-pipeline")

    if not pipeline or not source or not sink:
        logger.error("Not all elements could be created.")
        return -1

    # Build the pipeline.
    pipeline.add(source, sink)
    # Link the source and sink
    if not source.link(sink):
        logger.error("Elements could not be linked.")
        return -2

    # Set the pattern property of the "videotestsrc"
    source.set_property("pattern", 0)

    loop = GLib.MainLoop()

    # Listen to the bus
    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect("message", on_bus_message, loop)

    # Prepare data for callback
    pdata = ProbeData(pipeline, source, sink, 0)

    GLib.timeout_add_seconds(1, on_timeout, pdata)

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
