/* gstreamermm - a C++ wrapper for gstreamer
 *
 * Basic Tutorial 4: Time management
 *
 * This tutorial shows how to use GStreamer time-related facilities. In particular:
 * - How to query the pipeline for information like stream position or duration.
 * - How to seek (jump) to a different position (time) inside the stream.
 */

#include <gstreamermm.h>
#include <glibmm/main.h>
#include <iostream>
#include <iomanip>
#include <cstdlib>

using Glib::RefPtr;

// Expose objects to be used in callbacks
RefPtr<Glib::MainLoop> mainloop;
RefPtr<Gst::Element> playbin;
static bool playing {false};
static bool seekable {false};
static bool seek_done {false};
static gint64 duration {(gint64)Gst::CLOCK_TIME_NONE};

static std::ostringstream format_gst_time(gint64 gst_time)
{
  std::ostringstream oss_gst_time (std::ostringstream::out);

  oss_gst_time << std::right << std::setfill('0') <<
    std::setw(3) << Gst::get_hours(gst_time) << ":" <<
    std::setw(2) << Gst::get_minutes(gst_time) << ":" <<
    std::setw(2) << Gst::get_seconds(gst_time) << "." <<
    std::setw(9) << std::left << Gst::get_fractional_seconds(gst_time);
  return oss_gst_time;
}

// This function is used to receive asynchronous messages in the main loop.
bool on_bus_message(const RefPtr<Gst::Bus>&,
    const RefPtr<Gst::Message>& message)
{
  switch (message->get_message_type()) {
    case Gst::MESSAGE_EOS:
      std::cout << std::endl << "End of stream" << std::endl;
      mainloop->quit();
      return false;
    case Gst::MESSAGE_ERROR:
    {
      RefPtr<Gst::MessageError> msgError {RefPtr<Gst::MessageError>::cast_static(message)};
      if (msgError)
      {
        Glib::Error err {msgError->parse_error()};
        std::string debug_info {msgError->parse_debug()};
        std::cerr << "Error received from element " << message->get_source()->get_name() << ": " <<
            err.what() << std::endl;
        if (!debug_info.empty())
          std::cout << "Debugging information: " << debug_info << std::endl;
      }
      else
      {
        std::cerr << "Error." << std::endl;
      }
      mainloop->quit();
      return false;
    }
    case Gst::MESSAGE_DURATION_CHANGED:
      /* The duration has changed, mark the current one as invalid */
      duration = Gst::CLOCK_TIME_NONE;
      break;
    case Gst::MESSAGE_STATE_CHANGED:
    {
      // We are only interested in state-changed messages from the playbin 
      if (RefPtr<Gst::Element>::cast_dynamic(message->get_source()) == playbin)
      {
        auto state_get_name = [] (Gst::State state) -> std::string {
          return gst_element_state_get_name(static_cast<GstState>(state));
        };
        RefPtr<Gst::MessageStateChanged> msgSC {RefPtr<Gst::MessageStateChanged>::cast_static(message)};
        Gst::State old_state {msgSC->parse_old_state()};
        Gst::State new_state {msgSC->parse_new_state()};
        std::cout << "Pipeline state changed: " <<
            state_get_name(old_state) << " -> " <<
            state_get_name(new_state) << std::endl;
        /* Remember whether we are in the PLAYING state or not */
        playing = (new_state == Gst::STATE_PLAYING);
        if (playing)
        {
          /* We just moved to PLAYING. Check if seeking is possible */
          Gst::Format format {Gst::FORMAT_TIME};
          RefPtr<Gst::Query> query {Gst::Query::create_seeking(format)};
          if (playbin->query(query))
          {
            gint64 segment_start {0}, segment_end {0};
            RefPtr<Gst::QuerySeeking> seek_query = RefPtr<Gst::QuerySeeking>::cast_static(query);
            seek_query->parse(format, seekable, segment_start, segment_end);
            if (seekable)
              std::cout << "Seeking is ENABLED from " << format_gst_time(segment_start).str() <<
                " to " << format_gst_time(segment_end).str() << std::endl;
            else
              std::cout << "Seeking is DISABLED for this stream." << std::endl;
          }
        }
      }
      break;
    }
    default:
        //std::cout << "Unhandled message type: " << message->get_message_type() << std::endl;
      break;
  }

  return true;
}

bool on_timeout()
{
  // only if playing
  if (playing)
  {
    /* Query the current position of the stream */
    gint64 position {0};
    if (!playbin->query_position(Gst::FORMAT_TIME, position))
      std::cerr << "Could not query current position." << std::endl;

    /* If we didn't know it yet, query the stream duration */
    if (duration == (gint64)Gst::CLOCK_TIME_NONE)
    {
      if (!playbin->query_duration(Gst::FORMAT_TIME, duration))
        std::cerr << "Could not query current duration." << std::endl;
    }

    /* Print current position and total duration */
    std::cout << format_gst_time(position).str() << "/" <<
      format_gst_time(duration).str() << "\r" << std::flush;

    // If seeking is enabled, we have not done it yet, and the time is right, seek
    if (seekable && !seek_done && position > 10 * (gint64)Gst::SECOND)
    {
      std::cout << "Reached 10s, performing seek..." << std::endl;
      playbin->seek(Gst::FORMAT_TIME, Gst::SEEK_FLAG_FLUSH | Gst::SEEK_FLAG_KEY_UNIT, 30 * Gst::SECOND);
      seek_done = true;
    }
  }

	return true;
}

int main(int argc, char** argv)
{
  // Initialize gstreamermm:
  Gst::init(argc, argv);

  // default uri
  Glib::ustring uri {"https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm"};

  // Take the commandline argument and ensure that it is a uri:
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " <uri>" << std::endl;
    std::cout << "missing uri argument, use default uri instead." << std::endl;
  }
  else if (Gst::URIHandler::uri_is_valid(argv[1]))
  {
    uri = argv[1];
  }

  playbin = Gst::ElementFactory::create_element("playbin");

  if (!playbin)
  {
    std::cerr << "The playbin element could not be created." << std::endl;
    return EXIT_FAILURE;
  }

  // Set the URI to play
  playbin->set_property("uri", uri);

  // Create the main loop.
  mainloop = Glib::MainLoop::create();

  // Get the bus and watch the messages
  RefPtr<Gst::Bus> bus {playbin->get_bus()};
  bus->add_watch(sigc::ptr_fun(&on_bus_message));

  // start play back and listen to events
  if (playbin->set_state(Gst::STATE_PLAYING) == Gst::STATE_CHANGE_FAILURE)
  {
    std::cerr << "Unable to set the pipeline to the playing state." << std::endl;
    return EXIT_FAILURE;
  }

  // timeout of 100 milliseconds
	Glib::signal_timeout().connect(sigc::ptr_fun(&on_timeout), 100);

  // Now set the playbin to the PLAYING state and start the main loop:
  std::cout << "Running." << std::endl;
  mainloop->run();

  // Clean up nicely:
  std::cout << "Returned. Stopping pipeline." << std::endl;
  playbin->set_state(Gst::STATE_NULL);

  return EXIT_SUCCESS;
}
