/* gstreamermm - a C++ wrapper for gstreamer
 *
 * Basic Tutorial 3: Dynamic Pipelines
 */

#include <gstreamermm.h>
#include <glibmm/main.h>
#include <glibmm/stringutils.h>
#include <iostream>
#include <cstdlib>

Glib::RefPtr<Glib::MainLoop> mainloop;

// This function is used to receive asynchronous messages in the main loop.
bool on_bus_message(const Glib::RefPtr<Gst::Bus>& /* bus */,
    const Glib::RefPtr<Gst::Message>& message)
{
  switch (message->get_message_type()) {
    case Gst::MESSAGE_EOS:
      std::cout << std::endl << "End of stream" << std::endl;
      mainloop->quit();
      return false;
    case Gst::MESSAGE_ERROR:
    {
      Glib::RefPtr<Gst::MessageError> msgError {Glib::RefPtr<Gst::MessageError>::cast_static(message)};
      if (msgError)
      {
        Glib::Error err {msgError->parse_error()};
        std::string debug_info {msgError->parse_debug()};
        std::cerr << "Error received from element " << message->get_source()->get_name() << ": " <<
            err.what() << std::endl;
        if (!debug_info.empty())
        {
          std::cout << "Debugging information: " << debug_info << std::endl;
        }
      }
      else
      {
        std::cerr << "Error." << std::endl;
      }
      mainloop->quit();
      return false;
    }
    case Gst::MESSAGE_STATE_CHANGED:
    {
      // We are only interested in state-changed messages from the pipeline
      if ("test-pipeline" == message->get_source()->get_name())
      {
        auto state_get_name = [] (Gst::State state) -> std::string {
          return gst_element_state_get_name(static_cast<GstState>(state));
        };
        Glib::RefPtr<Gst::MessageStateChanged> msgSC {Glib::RefPtr<Gst::MessageStateChanged>::cast_static(message)};
        Gst::State old_state {msgSC->parse_old_state()};
        Gst::State new_state {msgSC->parse_new_state()};
        std::cout << "Pipeline state changed: " <<
            state_get_name(old_state) << " -> " <<
            state_get_name(new_state) << std::endl;
      }
      break;
    }
    default:
        //std::cout << "Unhandled message type: " << message->get_message_type() << std::endl;
      break;
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

  // Create elements
  Glib::RefPtr<Gst::Element> source {Gst::ElementFactory::create_element("uridecodebin", "source")},
    convert {Gst::ElementFactory::create_element("audioconvert", "convert")},
    resample {Gst::ElementFactory::create_element("audioresample", "resample")},
    sink {Gst::ElementFactory::create_element("autoaudiosink", "sink")};

  if (!source || !convert || !resample || !sink)
  {
    std::cerr << "One of the elements could not be created." << std::endl;
    return EXIT_FAILURE;
  }

  // Create the empty pipeline
  Glib::RefPtr<Gst::Pipeline> pipeline {Gst::Pipeline::create("test-pipeline")};

  // add the elements to the pipeline before linking them
  try
  {
    pipeline->add(source)->add(convert)->add(resample)->add(sink);
  }
  catch (std::runtime_error& ex)
  {
    std::cerr << "Exception while adding: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }

  // Link the elements
  try
  {
    // We link the elements converter, resample and sink, but we DO NOT link them with the source,
    // since at this point it contains no source pads. We do it later in a pad-added signal handler.
    convert->link(resample)->link(sink);
  }
  catch(const std::runtime_error& ex)
  {
    std::cout << "Exception while linking elements: " << ex.what() << std::endl;
  }

  // Set the uri property.
  source->set_property("uri", uri);
  // Signal handler for on-pad-added signal of source element
  // Here we use lambda to expose local variables that are needed
  source->signal_pad_added().connect(
    [convert] (const Glib::RefPtr<Gst::Pad> &new_pad)
    {
      Glib::RefPtr<Gst::Pad> sink_pad {convert->get_static_pad("sink")};
      // If our converter is already linked, we have nothing to do here
      if (sink_pad->is_linked())
      {
        std::cout << "sink pad of convert is already linked. Ignoring." << std::endl;
        return;
      }
      // Retrieves the current capabilities of the new pad
      Glib::RefPtr<Gst::Caps> new_pad_caps {new_pad->get_current_caps()}; 
      Glib::ustring media_type {new_pad_caps->get_structure(0).get_name()};
      std::cout << "Received new pad " << new_pad->get_name() << ", media type: " << media_type << std::endl;
      // Check the new pad's type
      if (Glib::str_has_prefix(media_type, "audio/x-raw"))
      {
        Gst::PadLinkReturn ret = new_pad->link(sink_pad);
        if (ret != Gst::PAD_LINK_OK && ret != Gst::PAD_LINK_WAS_LINKED)
        {
          std::cerr << "Linking of pads " << new_pad->get_name() << " and " <<
              sink_pad->get_name() << " failed." << std::endl;
        }
      }
      else
      {
        std::cout << "Media type is not raw audio. Ignoring." << std::endl;
      }
    });

  // Create the main loop.
  mainloop = Glib::MainLoop::create();

  // Get the bus and watch the messages
  Glib::RefPtr<Gst::Bus> bus {pipeline->get_bus()};
  bus->add_watch(sigc::ptr_fun(&on_bus_message));

  // start play back and listen to events
  if (pipeline->set_state(Gst::STATE_PLAYING) == Gst::STATE_CHANGE_FAILURE)
  {
    std::cerr << "Unable to set the pipeline to the playing state." << std::endl;
    return EXIT_FAILURE;
  }

  // Now set the playbin to the PLAYING state and start the main loop:
  std::cout << "Running." << std::endl;
  mainloop->run();

  // Clean up nicely:
  std::cout << "Returned. Stopping pipeline." << std::endl;
  pipeline->set_state(Gst::STATE_NULL);

  return EXIT_SUCCESS;
}
