/* gstreamermm - a C++ wrapper for gstreamer
 *
 * Basic Tutorial 5:  GUI toolkit integration
 * - How to tell GStreamer to output video to a particular window (instead of creating its own window)
 * - How to continuously refresh the GUI with information from GStreamer.
 * - How to update the GUI from the multiple threads of GStreamer, an operation forbidden on most GUI toolkits.
 * - A mechanism to subscribe only to the messages you are interested in, instead of being notified of all of them.
 */

#include <gstreamermm.h>
#include <glibmm.h>
#include <gtkmm.h>
#include <iostream>
#include <sstream>

using Glib::RefPtr;
using Gst::Element;
using Gst::Bus;
using Gst::Message;
using Gst::State;
using Gtk::Application;
using Gtk::Widget;
using Gtk::Button;
using Gtk::HScale;
using Gtk::Box;
using Gtk::TextView;

static void PlayBin_signal_tags_changed_callback(Element* self, gint p0, void* data)
{
  using SlotType = sigc::slot<void, int>;

  auto obj = dynamic_cast<Element*>(Glib::ObjectBase::_get_current_wrapper((GObject*) self));
  // Do not try to call a signal on a disassociated wrapper.
  if (obj)
  {
    try
    {
      if (const auto slot = Glib::SignalProxyNormal::data_to_slot(data))
        (*static_cast<SlotType*>(slot))(p0);
    }
    catch (...)
    {
      Glib::exception_handlers_invoke();
    }
  }
}

static const Glib::SignalProxyInfo PlayBin_signal_video_tags_changed_info =
{
  "video-tags-changed",
  (GCallback) &PlayBin_signal_tags_changed_callback,
  (GCallback) &PlayBin_signal_tags_changed_callback
};

static const Glib::SignalProxyInfo PlayBin_signal_audio_tags_changed_info =
{
  "audio-tags-changed",
  (GCallback) &PlayBin_signal_tags_changed_callback,
  (GCallback) &PlayBin_signal_tags_changed_callback
};


static const Glib::SignalProxyInfo PlayBin_signal_text_tags_changed_info =
{
  "text-tags-changed",
  (GCallback) &PlayBin_signal_tags_changed_callback,
  (GCallback) &PlayBin_signal_tags_changed_callback
};


class PlayerWindow: public Gtk::Window
{
public:
  PlayerWindow(const RefPtr<Element>& playbin);
  ~PlayerWindow();

protected:
  bool on_delete_event(GdkEventAny* any_event);
  void on_tags_changed();
  void on_button_play();
  void on_button_pause();
  void on_button_stop();
  void on_slider_value_changed();

  void create_ui();
  bool refresh_ui();
  void analyze_streams();
  bool on_bus_message(const RefPtr<Bus>& bus, const RefPtr<Message>& message);

protected:
  Button play_button;
  Button pause_button;
  Button stop_button;
  HScale slider;
  TextView streams_list;
  sigc::connection slider_value_changed_sigconn;
  Box top_hbox;
  Box controls_hbox;
  Box main_vbox;

protected:
  RefPtr<Element> m_playbin;
  RefPtr<Widget> sink_widget;
  guint watch_id;
  State stream_state;
  gint64 stream_duration;
};


PlayerWindow::PlayerWindow(const RefPtr<Element>& playbin)
  : play_button{}
  , pause_button{}
  , stop_button{}
  , slider{ 0, 100, 1 }
  , streams_list{}
  , top_hbox{ Gtk::ORIENTATION_HORIZONTAL, 0 }
  , controls_hbox{ Gtk::ORIENTATION_HORIZONTAL, 0 }
  , main_vbox{ Gtk::ORIENTATION_VERTICAL, 0 }
  , m_playbin{ playbin }
  , stream_state{ Gst::STATE_NULL}
  , stream_duration{ (gint64)Gst::CLOCK_TIME_NONE }
{
  m_playbin = playbin;

  /* Here we create the GTK Sink element which will provide us with a GTK widget where
   * GStreamer will render the video at and we can add to our UI.
   * Try to create the OpenGL version of the video sink, and fallback if that fails */
  auto video_sink = Gst::ElementFactory::create_element("glsinkbin");
  auto gtkgl_sink = Gst::ElementFactory::create_element("gtkglsink");

  if (video_sink && gtkgl_sink)
  {
    std::cout << "Successfully created GTK GL Sink" << std::endl;
    video_sink->set_property("sink", gtkgl_sink);
    /* The gtkglsink creates the gtk widget for us. This is accessible through a property.
     * So we get it and use it later to add it to our gui. */
    gtkgl_sink->get_property("widget", sink_widget);
  }
  else
  {
    std::cout << "Could not create gtkglsink, falling back to gtksink." << std::endl;
    video_sink = Gst::ElementFactory::create_element("gtksink");
    video_sink->get_property("widget", sink_widget);
  }

  m_playbin->set_property("video-sink", video_sink);

  /* Connect to interesting signals in m_playbin */
  Glib::SignalProxy<void>(m_playbin.operator->(), &PlayBin_signal_video_tags_changed_info).connect(
      sigc::mem_fun(*this, &PlayerWindow::on_tags_changed));
  Glib::SignalProxy<void>(m_playbin.operator->(), &PlayBin_signal_audio_tags_changed_info).connect(
      sigc::mem_fun(*this, &PlayerWindow::on_tags_changed));
  Glib::SignalProxy<void>(m_playbin.operator->(), &PlayBin_signal_text_tags_changed_info).connect(
      sigc::mem_fun(*this, &PlayerWindow::on_tags_changed));

  create_ui();

  // Get the bus from the pipeline:
  RefPtr<Bus> bus {m_playbin->get_bus()};
  // Add a bus watch to receive messages from the pipeline's bus:
  watch_id = bus->add_watch(sigc::mem_fun(*this, &PlayerWindow::on_bus_message));

  // start play back and listen to events
  if (m_playbin->set_state(Gst::STATE_PLAYING) == Gst::STATE_CHANGE_FAILURE)
  {
    std::cerr << "Unable to set the pipeline to the playing state." << std::endl;
    close();
  }

  // timeout of 500 milliseconds
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &PlayerWindow::refresh_ui), 500);
}


PlayerWindow::~PlayerWindow()
{
  m_playbin->get_bus()->remove_watch(watch_id);
  m_playbin->set_state(Gst::STATE_NULL);
}


/* This function is called when the main window is closed */
bool PlayerWindow::on_delete_event(GdkEventAny* any_event)
{
  m_playbin->set_state(Gst::STATE_READY);
  return false;
}


/* This function is called when new metadata is discovered in the stream */
void PlayerWindow::on_tags_changed()
{
  /* We are possibly in a GStreamer working thread, so we notify the main
   * thread of this event through a message in the bus */
  m_playbin->post_message(Gst::MessageApplication::create(m_playbin, Gst::Structure("tag-changed")));
}


/* This creates all the GTK+ widgets that compose our application, and registers the callbacks */
void PlayerWindow::create_ui()
{
  play_button.set_image_from_icon_name("media-playback-start", Gtk::ICON_SIZE_SMALL_TOOLBAR);
  play_button.signal_clicked().connect(sigc::mem_fun(*this, &PlayerWindow::on_button_play));

  pause_button.set_image_from_icon_name("media-playback-pause", Gtk::ICON_SIZE_SMALL_TOOLBAR);
  pause_button.signal_clicked().connect(sigc::mem_fun(*this, &PlayerWindow::on_button_pause));

  stop_button.set_image_from_icon_name("media-playback-stop", Gtk::ICON_SIZE_SMALL_TOOLBAR); 
  stop_button.signal_clicked().connect(sigc::mem_fun(*this, &PlayerWindow::on_button_stop));

  slider.set_draw_value(false);
  slider_value_changed_sigconn = slider.signal_value_changed().connect(
      sigc::mem_fun(*this, &PlayerWindow::on_slider_value_changed));

  /* sink widget and text list of stream in a horizontal box */
  //auto top_hbox = Box(Gtk::ORIENTATION_HORIZONTAL, 0);
  top_hbox.pack_start(*sink_widget.get(), true, true, 0);
  top_hbox.pack_start(streams_list, false, false, 2);

  /* buttons and slider in a horizontal box */
  //auto controls_hbox = Box(Gtk::ORIENTATION_HORIZONTAL, 0);
  controls_hbox.pack_start(play_button, false, false, 2);
  controls_hbox.pack_start(pause_button, false, false, 2);
  controls_hbox.pack_start(stop_button, false, false, 2);
  controls_hbox.pack_start(slider, true, true, 2);

  /* the main box for the layout all other boxes */
  //auto main_vbox = Box(Gtk::ORIENTATION_VERTICAL, 0);
  main_vbox.pack_start(top_hbox, true, true, 0);
  main_vbox.pack_start(controls_hbox, false, false, 0);

  /* add the main layout of all widgets to this window */
  add(main_vbox);
  set_default_size(640, 480);
  show_all();
}


/* This function is called when the PLAY button is clicked */
void PlayerWindow::on_button_play()
{
  m_playbin->set_state(Gst::STATE_PLAYING);
}


/* This function is called when the PAUSE button is clicked */
void PlayerWindow::on_button_pause()
{
  m_playbin->set_state(Gst::STATE_PAUSED);
}


/* This function is called when the STOP button is clicked */
void PlayerWindow::on_button_stop()
{
  m_playbin->set_state(Gst::STATE_READY);
}


/* This function is called when the slider changes its position. We perform a seek to the
 * new position here. */
void PlayerWindow::on_slider_value_changed()
{
  gint64 value = (gint64)slider.get_value();
  m_playbin->seek(Gst::FORMAT_TIME, Gst::SEEK_FLAG_FLUSH | Gst::SEEK_FLAG_KEY_UNIT, gint64(value * Gst::SECOND));
}


bool PlayerWindow::refresh_ui()
{
  /* We do not want to update anything unless we are in the PAUSED or PLAYING states */
  if (stream_state < Gst::STATE_PAUSED)
    return true;

  /* If we didn't know it yet, query the stream duration */
  if (stream_duration == (gint64)Gst::CLOCK_TIME_NONE)
  {
    if (m_playbin->query_duration(Gst::FORMAT_TIME, stream_duration))
    {
      slider.set_range(0.0, (double)stream_duration / Gst::SECOND);
    }
    else
    {
      std::cerr << "Could not query current duration." << std::endl;
    }
  }

  /* Query the current position of the stream */
  gint64 current {0};
  if (m_playbin->query_position(Gst::FORMAT_TIME, current))
  {
    slider_value_changed_sigconn.block();
    slider.set_value((double)current / Gst::SECOND);
    slider_value_changed_sigconn.unblock();
  }

  return true;
}


void PlayerWindow::analyze_streams()
{
  auto text = streams_list.get_buffer();
  text->set_text("");

  gint n_video;
  m_playbin->get_property("n-video", n_video);
  for (gint i = 0; i < n_video; i++)
  {
    GstTagList* tags;
    g_signal_emit_by_name(m_playbin->gobj(), "get-video-tags", i, &tags, static_cast<void*>(0));
    if (tags)
    {
      auto video_tags {Glib::wrap_taglist(tags, true)};

      std::string codec_str;
      video_tags.get(Gst::TAG_VIDEO_CODEC, codec_str);

      std::ostringstream ostr;
      ostr << "Video stream " << i << ":" << std::endl
          << "    codec: " << codec_str << std::endl;
      text->insert_at_cursor(ostr.str());
    }
  }

  gint n_audio;
  m_playbin->get_property("n-audio", n_audio);
  for (gint i = 0; i < n_audio; i++)
  {
    GstTagList* tags;
    g_signal_emit_by_name(m_playbin->gobj(), "get-audio-tags", i, &tags, static_cast<void*>(0));
    if (tags)
    {
      auto audio_tags {Glib::wrap_taglist(tags, true)};

      std::string codec_str, language_str;
      guint bitrate;
      audio_tags.get(Gst::TAG_AUDIO_CODEC, codec_str);
      audio_tags.get(Gst::TAG_LANGUAGE_CODE, language_str);
      audio_tags.get(Gst::TAG_BITRATE, bitrate);

      std::ostringstream ostr;
      ostr << "Audio stream " << i << ":" << std::endl
          << "    codec: " << codec_str << std::endl
          << "    language: " << language_str << std::endl
          << "    bitrate: " << bitrate << std::endl;
      text->insert_at_cursor(ostr.str());
    }
  }

  gint n_text;
  m_playbin->get_property("n-text", n_text);
  for (gint i = 0; i < n_text; i++)
  {
    GstTagList* tags;
    g_signal_emit_by_name(m_playbin->gobj(), "get-text-tags", i, &tags, static_cast<void*>(0));
    if (tags)
    {
      auto text_tags {Glib::wrap_taglist(tags, true)};

      std::string language_str;
      text_tags.get(Gst::TAG_LANGUAGE_CODE, language_str);

      std::ostringstream ostr;
      ostr << "Subtitle stream " << i << ":" << std::endl
          << "    language: " << language_str << std::endl;
      text->insert_at_cursor(ostr.str());
    }
  }
}


bool PlayerWindow::on_bus_message(const RefPtr<Gst::Bus>& bus, const RefPtr<Message>& message)
{
  switch (message->get_message_type()) {
    case Gst::MESSAGE_EOS:
    {
      std::cout << std::endl << "End of stream" << std::endl;
      on_button_stop();
      return false;
    }
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
      on_button_stop();
      return false;
    }
    case Gst::MESSAGE_STATE_CHANGED:
    {
      // We are only interested in state-changed messages from the m_playbin 
      if (RefPtr<Element>::cast_dynamic(message->get_source()) == m_playbin)
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
        stream_state = new_state;
        if (old_state == Gst::STATE_READY && new_state == Gst::STATE_PAUSED) {
          /* For extra responsiveness, we refresh the GUI as soon as we reach the PAUSED state */
          refresh_ui();
        }
      }
      break;
    }
    case Gst::MESSAGE_APPLICATION:
    {
      if ("tag-changed" == message->get_structure().get_name())
        analyze_streams();
      break;
    }
    default:
        //std::cout << "Unhandled message type: " << message->get_message_type() << std::endl;
      break;
  }

  return true;
}


int main (int argc, char **argv)
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

  auto playbin {Gst::ElementFactory::create_element("playbin")};

  if (!playbin)
  {
    std::cerr << "The playbin could not be created." << std::endl;
    return EXIT_FAILURE;
  }

  // Set the URI to play
  playbin->set_property("uri", uri);

  // create Gtk::Application object and initialize gtkmm
  auto app {Application::create(argc, argv, "org.gtkmm.gstreamermm.player")};

  // create a Gtk::Window object
  PlayerWindow player {playbin};

  // enter gtkmm main processing loop and show the player window object
  return app->run(player);
}
