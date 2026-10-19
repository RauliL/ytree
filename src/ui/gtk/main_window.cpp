#include <gtkmm.h>

#include "ui/gtk/main_window.hpp"

#ifdef Error
# undef Error
#endif

namespace {

YTreeMainWindow* g_main_window = nullptr;

bool HasControl(Gdk::ModifierType state)
{
  return (state & Gdk::ModifierType::CONTROL_MASK) == Gdk::ModifierType::CONTROL_MASK;
}

ui::InputEvent KeyvalToEvent(guint keyval, Gdk::ModifierType state)
{
  using ui::Key;

  // Ctrl+Letter → curses-style control codes ('A'&0x1f … 'Z'&0x1f).
  if (HasControl(state))
  {
    const guint upper = gdk_keyval_to_upper(keyval);
    if (upper >= GDK_KEY_A && upper <= GDK_KEY_Z)
    {
      return ui::InputEvent::CharKey(static_cast<int>(upper) & 0x1f);
    }
  }

  switch (keyval)
  {
    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
      return ui::InputEvent::Special(Key::Up);
    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
      return ui::InputEvent::Special(Key::Down);
    case GDK_KEY_Left:
    case GDK_KEY_KP_Left:
      return ui::InputEvent::Special(Key::Left);
    case GDK_KEY_Right:
    case GDK_KEY_KP_Right:
      return ui::InputEvent::Special(Key::Right);
    case GDK_KEY_Page_Up:
    case GDK_KEY_KP_Page_Up:
      return ui::InputEvent::Special(Key::PageUp);
    case GDK_KEY_Page_Down:
    case GDK_KEY_KP_Page_Down:
      return ui::InputEvent::Special(Key::PageDown);
    case GDK_KEY_Home:
    case GDK_KEY_KP_Home:
      return ui::InputEvent::Special(Key::Home);
    case GDK_KEY_End:
    case GDK_KEY_KP_End:
      return ui::InputEvent::Special(Key::End);
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
      return ui::InputEvent::Special(Key::Enter);
    case GDK_KEY_Escape:
      return ui::InputEvent::Special(Key::Escape);
    case GDK_KEY_Tab:
      return ui::InputEvent::Special(Key::Tab);
    case GDK_KEY_ISO_Left_Tab:
      return ui::InputEvent::Special(Key::BackTab);
    case GDK_KEY_space:
      return ui::InputEvent::Special(Key::Space);
    default:
      break;
  }

  if (keyval >= GDK_KEY_F1 && keyval <= GDK_KEY_F12)
  {
    return ui::InputEvent{
      Key::Function,
      static_cast<int>(keyval - GDK_KEY_F1 + 1),
      0,
      {}
    };
  }

  const gunichar ch = gdk_keyval_to_unicode(keyval);
  if (ch != 0 && ch < 128)
  {
    return ui::InputEvent::CharKey(static_cast<int>(ch));
  }

  return ui::InputEvent{};
}

} // namespace

YTreeMainWindow* GtkMainWindow()
{
  return g_main_window;
}

void SetGtkMainWindow(YTreeMainWindow* window)
{
  g_main_window = window;
}

YTreeMainWindow::YTreeMainWindow()
{
  set_title("ytree");
  set_default_size(960, 640);

  dir_model_ = Gtk::StringList::create({});
  file_model_ = Gtk::StringList::create({});

  auto dir_factory = Gtk::SignalListItemFactory::create();
  dir_factory->signal_setup().connect([](const Glib::RefPtr<Gtk::ListItem>& item) {
    item->set_child(*Gtk::make_managed<Gtk::Label>());
  });
  dir_factory->signal_bind().connect([](const Glib::RefPtr<Gtk::ListItem>& item) {
    auto* label = dynamic_cast<Gtk::Label*>(item->get_child());
    auto row = std::dynamic_pointer_cast<Gtk::StringObject>(item->get_item());
    if (label && row)
    {
      label->set_text(row->get_string());
      label->set_xalign(0.0f);
    }
  });

  auto file_factory = Gtk::SignalListItemFactory::create();
  file_factory->signal_setup().connect([](const Glib::RefPtr<Gtk::ListItem>& item) {
    item->set_child(*Gtk::make_managed<Gtk::Label>());
  });
  file_factory->signal_bind().connect([](const Glib::RefPtr<Gtk::ListItem>& item) {
    auto* label = dynamic_cast<Gtk::Label*>(item->get_child());
    auto row = std::dynamic_pointer_cast<Gtk::StringObject>(item->get_item());
    if (label && row)
    {
      label->set_text(row->get_string());
      label->set_xalign(0.0f);
    }
  });

  auto dir_selection = Gtk::SingleSelection::create(dir_model_);
  auto file_selection = Gtk::SingleSelection::create(file_model_);
  dir_list_.set_model(dir_selection);
  file_list_.set_model(file_selection);
  dir_list_.set_factory(dir_factory);
  file_list_.set_factory(file_factory);

  dir_scroll_.set_child(dir_list_);
  file_scroll_.set_child(file_list_);
  dir_scroll_.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
  file_scroll_.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);

  paned_.set_start_child(dir_scroll_);
  paned_.set_end_child(file_scroll_);
  paned_.set_position(420);
  paned_.set_vexpand(true);

  title_.add_css_class("title-1");
  help_line0_.set_xalign(0.0f);
  help_line1_.set_xalign(0.0f);
  help_line0_.set_selectable(false);
  help_line1_.set_selectable(false);
  help_line0_.add_css_class("monospace");
  help_line1_.add_css_class("monospace");
  help_box_.set_spacing(0);
  help_box_.append(help_line0_);
  help_box_.append(help_line1_);
  status_.set_wrap(true);

  root_.set_margin(8);
  root_.set_spacing(6);
  root_.append(title_);
  root_.append(path_);
  root_.append(disk_);
  root_.append(filespec_);
  root_.append(paned_);
  root_.append(help_box_);
  root_.append(status_);
  set_child(root_);

  auto controller = Gtk::EventControllerKey::create();
  controller->signal_key_pressed().connect(
    sigc::mem_fun(*this, &YTreeMainWindow::on_key_pressed),
    false
  );
  add_controller(controller);

  setStatus("Ready");
  setHelp(DirHelpLines());
}

bool YTreeMainWindow::on_key_pressed(
  guint keyval,
  guint /*keycode*/,
  Gdk::ModifierType state
)
{
  const auto ev = KeyvalToEvent(keyval, state);
  if (ev.key == ui::Key::None)
  {
    return false;
  }
  pushEvent(ev);
  return true;
}

void YTreeMainWindow::pushEvent(ui::InputEvent ev)
{
  std::lock_guard lock(queue_mutex_);
  event_queue_.push_back(std::move(ev));
}

ui::InputEvent YTreeMainWindow::waitEvent()
{
  auto context = Glib::MainContext::get_default();
  while (true)
  {
    {
      std::lock_guard lock(queue_mutex_);
      if (!event_queue_.empty())
      {
        auto ev = event_queue_.front();
        event_queue_.pop_front();
        return ev;
      }
    }
    context->iteration(true);
  }
}

bool YTreeMainWindow::hasPendingEvent() const
{
  std::lock_guard lock(queue_mutex_);
  return !event_queue_.empty();
}

void YTreeMainWindow::setStatus(const Glib::ustring& text)
{
  status_.set_text(text);
}

void YTreeMainWindow::setHelp(const HelpLines& lines)
{
  help_line0_.set_text(lines[0]);
  help_line1_.set_text(lines[1]);
}

void YTreeMainWindow::clearHelp()
{
  help_line0_.set_text({});
  help_line1_.set_text({});
}

void YTreeMainWindow::setPath(const Glib::ustring& text)
{
  path_.set_text(text);
}

void YTreeMainWindow::setFileSpec(const Glib::ustring& text)
{
  filespec_.set_text(text);
}

void YTreeMainWindow::setDiskName(const Glib::ustring& text)
{
  disk_.set_text(text);
}
