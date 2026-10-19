#pragma once

#include <gtkmm.h>

#include "core/ui.hpp"
#include "ui/help.hpp"

#include <deque>
#include <mutex>

class YTreeMainWindow : public Gtk::ApplicationWindow
{
public:
  YTreeMainWindow();

  void pushEvent(ui::InputEvent ev);
  ui::InputEvent waitEvent();
  bool hasPendingEvent() const;

  void setStatus(const Glib::ustring& text);
  void setHelp(const HelpLines& lines);
  void clearHelp();
  void setPath(const Glib::ustring& text);
  void setFileSpec(const Glib::ustring& text);
  void setDiskName(const Glib::ustring& text);

  Gtk::ListView& dirList() { return dir_list_; }
  Gtk::ListView& fileList() { return file_list_; }
  Gtk::Label& statusLabel() { return status_; }

  Glib::RefPtr<Gtk::StringList> dirModel() { return dir_model_; }
  Glib::RefPtr<Gtk::StringList> fileModel() { return file_model_; }

private:
  bool on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);

  Gtk::Box root_{Gtk::Orientation::VERTICAL};
  Gtk::Label title_{"ytree"};
  Gtk::Label path_;
  Gtk::Label disk_;
  Gtk::Label filespec_;
  Gtk::Paned paned_{Gtk::Orientation::HORIZONTAL};
  Gtk::ScrolledWindow dir_scroll_;
  Gtk::ScrolledWindow file_scroll_;
  Gtk::ListView dir_list_;
  Gtk::ListView file_list_;
  Gtk::Box help_box_{Gtk::Orientation::VERTICAL};
  Gtk::Label help_line0_;
  Gtk::Label help_line1_;
  Gtk::Label status_;

  Glib::RefPtr<Gtk::StringList> dir_model_;
  Glib::RefPtr<Gtk::StringList> file_model_;

  mutable std::mutex queue_mutex_;
  std::deque<ui::InputEvent> event_queue_;
};

YTreeMainWindow* GtkMainWindow();
void SetGtkMainWindow(YTreeMainWindow* window);
