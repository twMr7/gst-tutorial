/* gstreamermm - a C++ wrapper for gstreamer
 *
 * Supplement to Basic Tutorial 3: Dynamic Source
 *
 * Simple example to demonstrate dynamically adding and removing source elements
 * to a playing pipeline.
 */

#include <gstreamermm.h>
#include <glibmm/main.h>
#include <iostream>
#include <cstdlib>

using Glib::RefPtr;

// Expose objects to be used in callbacks
Glib::RefPtr<Glib::MainLoop> mainloop;
RefPtr<Gst::Element> source;
RefPtr<Gst::Element> sink;
RefPtr<Gst::Pipeline> pipeline;

// This function is used to receive asynchronous messages in the main loop.
bool on_bus_message(const RefPtr<Gst::Bus>&,
    const RefPtr<Gst::Message>& message)
{
  switch(message->get_message_type())
  {
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
        RefPtr<Gst::MessageStateChanged> msgSC {RefPtr<Gst::MessageStateChanged>::cast_static(message)};
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

bool on_timeout()
{
	static int pattern = 0;

	source->set_state(Gst::STATE_NULL);
  pipeline->remove(source);
  // Create a new source
  source = Gst::ElementFactory::create_element("videotestsrc", "source");
  pattern = (pattern < 25) ? (pattern + 1) : 0;
  source->set_property("pattern", pattern);
  source->set_property("is_live", true);
  // Rebuild the pipeline
  pipeline->add(source);
  source->link(sink);
	source->set_state(Gst::STATE_PLAYING);

	return true;
}

int main(int argc, char** argv)
{
  // Initialize gstreamermm:
  Gst::init(argc, argv);

  // Create elements
  source = Gst::ElementFactory::create_element("videotestsrc", "source");
  sink = Gst::ElementFactory::create_element("autovideosink", "sink");

  // Create the empty pipeline
  pipeline = Gst::Pipeline::create("test-pipeline");

  if (!source || !sink || !pipeline)
  {
    std::cerr << "Pipeline or one of the elements could not be created." << std::endl;
    return EXIT_FAILURE;
  }

  try
  {
    // add the elements to the pipeline before linking them
    pipeline->add(source)->add(sink);
    // Link the source and sink
    source->link(sink);
  }
	catch (const std::exception& ex)
  {
		std::cerr << "Exception occured during preparing pipeline: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }

  // Set the URI to play
  source->set_property("pattern", 0);

  // Create the main loop.
  mainloop = Glib::MainLoop::create();

  // Get the bus and watch the messages
  RefPtr<Gst::Bus> bus {pipeline->get_bus()};
  bus->add_watch(sigc::ptr_fun(&on_bus_message));

  // start play back and listen to events
  if (pipeline->set_state(Gst::STATE_PLAYING) == Gst::STATE_CHANGE_FAILURE)
  {
    std::cerr << "Unable to set the pipeline to the playing state." << std::endl;
    return EXIT_FAILURE;
  }

	Glib::signal_timeout().connect(sigc::ptr_fun(&on_timeout), 1000);

  // Now set the playbin to the PLAYING state and start the main loop:
  std::cout << "Running." << std::endl;
  mainloop->run();

  // Clean up nicely:
  std::cout << "Returned. Stopping pipeline." << std::endl;
  pipeline->set_state(Gst::STATE_NULL);

  return EXIT_SUCCESS;
}
