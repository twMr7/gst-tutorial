#!/usr/bin/env python3

"""
Basic Tutorial 4: Time management

This tutorial shows how to use GStreamer time-related facilities. In particular:
- How to query the pipeline for information like stream position or duration.
- How to seek (jump) to a different position (time) inside the stream.
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
    def __init__(self, playbin, loop):
        self.playbin = playbin
        self.loop = loop
        self.playing = False
        self.seekable = False;
        self.seek_done = False;
        self.duration = Gst.CLOCK_TIME_NONE;


def format_gst_time(nsec):
    """Convert nanoseconds to H:M:S.ns format
    """
    t_sec, remain_nsec = divmod(nsec, 1000000000)
    hours, remain_sec = divmod(t_sec, 3600)
    minutes, seconds = divmod(remain_sec, 60)
    return f"{hours:02d}:{minutes:02d}:{seconds:02d}.{remain_nsec:09d}"


def on_bus_message(bus, message, data):
    if message.type == Gst.MessageType.EOS:
        logger.info("End-of-stream reached.")
        data.loop.quit()
    elif message.type == Gst.MessageType.ERROR:
        err, debug_info = message.parse_error()
        logger.error(f"Error received from element {message.src.get_name()}: {err.message}")
        logger.error(f"Debugging information: {debug_info if debug_info else 'none'}")
        data.loop.quit()
    elif message.type == Gst.MessageType.DURATION_CHANGED:
        # The duration has changed, mark the current one as invalid
        data.duration = Gst.CLOCK_TIME_NONE
    elif message.type == Gst.MessageType.STATE_CHANGED:
        # We are only interested in state-changed messages from the pipeline
        old_state, new_state, pending_state = message.parse_state_changed()
        if (message.src == data.playbin):
            logger.info(f"Pipeline state changed: {old_state.value_nick} -> {new_state.value_nick}")
            # Remember whether we are in the PLAYING state or not
            data.playing = True if Gst.State.PLAYING == new_state else False
            if data.playing:
                # We just moved to PLAYING. Check if seeking is possible
                query = Gst.Query.new_seeking(Gst.Format.TIME)
                data.playbin.query(query)
                format, data.seekable, segment_start, segment_end = query.parse_seeking()
                if data.seekable:
                    logger.info(f"Seeking is ENABLED from {format_gst_time(segment_start)} " \
                                f"to {format_gst_time(segment_end)}")
                else:
                    logger.info("Seeking is DISABLED for this stream.")
    else:
        logger.debug(f"Unhandled message type {message.type}.")
    return True


def on_timeout(data):
    # only if playing
    if data.playing:
        # Query the current position of the stream
        query_ok, position = data.playbin.query_position(Gst.Format.TIME)
        if not query_ok:
            logger.error("Could not query current position.")
            position = Gst.CLOCK_TIME_NONE

        # If we didn't know it yet, query the stream duration
        if data.duration == Gst.CLOCK_TIME_NONE:
            query_ok, data.duration = data.playbin.query_duration(Gst.Format.TIME)
            if not query_ok:
                logger.error("Could not query current duration.")
                data.duration = Gst.CLOCK_TIME_NONE
        
        # Print current position and total duration
        print(f"Position {format_gst_time(position)} / {format_gst_time(data.duration)} \r",
                end='', file=sys.stdout)

        # If seeking is enabled, we have not done it yet, and the time is right, seek
        if (data.seekable) and (not data.seek_done) and (position > 10 * Gst.SECOND):
            logger.info("Reached 10s, performing seek...")
            data.playbin.seek_simple(Gst.Format.TIME, Gst.SeekFlags.FLUSH | Gst.SeekFlags.KEY_UNIT, 30 * Gst.SECOND)
            data.seek_done = True

    # start the next timeout
    GLib.timeout_add(100, on_timeout, data)


def main(argv):
    # Initialize GStreamer
    Gst.init(None)

    uri = "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm"
    if len(argv) != 2:
        logger.info(f"usage: {argv[0]} <uri>")
        logger.info(f"missing uri argument, use default uri instead.")
    elif Gst.uri_is_valid(argv[1]):
        uri = argv[1]

    playbin = Gst.ElementFactory.make("playbin", None)
    if not playbin:
        sys.stderr.write("The playbin element could not be created.\n")
        sys.exit(1)

    # Set the URI to play
    playbin.set_property('uri', uri)

    loop = GLib.MainLoop()

    # Prepare data for callback
    data = CustomData(playbin, loop)

    # Listen to the bus
    bus = playbin.get_bus()
    bus.add_signal_watch()
    bus.connect("message", on_bus_message, data)

    # timeout of 100 milliseconds
    GLib.timeout_add(100, on_timeout, data)

    # start play back and listen to events
    ret = playbin.set_state(Gst.State.PLAYING)
    if ret == Gst.StateChangeReturn.FAILURE:
        logger.error("Unable to set the playbin to the playing state.")
        return -1

    try:
        loop.run()
    except:
        logger.error("Unhandled exception.")

    # cleanup
    playbin.set_state(Gst.State.NULL)


if __name__ == '__main__':
    sys.exit(main(sys.argv))
