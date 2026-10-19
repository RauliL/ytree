#pragma once

#include <gtkmm.h>

#include "core/ui.hpp"
#include "ui/help.hpp"

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

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

  void setFileSpec(const std::string& spec);
  void setDiskName(const std::string& name);
  void setAvailKb(std::int64_t avail_kb);
  void showDiskStatistics();
  void showDirStatistics(const ui::DirPanelInfo& info);
  void setTagged(const ui::TaggedCounts& counts);
  void setCurrentDirectory(const std::string& name, std::int64_t bytes);
  void setCurrentFile(const std::string& name, std::int64_t size);

  void setDirRows(const std::vector<Glib::ustring>& rows, int hilight);
  void setFileRows(const std::vector<Glib::ustring>& rows, int hilight);

  Gtk::ListView& dirList() { return dir_list_; }
  Gtk::ListView& fileList() { return file_list_; }

private:
  bool on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
  void buildSidebar();
  void setStatValue(Gtk::Label& label, std::int64_t value);
  void addSidebarRow(const Glib::ustring& caption, Gtk::Label& value);
  void bindListView(Gtk::ListView& view);
  void applyRows(
    Gtk::ListView& view,
    Glib::RefPtr<Gtk::StringList>& model,
    const std::vector<Glib::ustring>& rows,
    int hilight
  );

  Gtk::Box root_{Gtk::Orientation::VERTICAL};
  Gtk::Label path_;
  Gtk::Box body_{Gtk::Orientation::HORIZONTAL};
  Gtk::Box lists_{Gtk::Orientation::VERTICAL};
  Gtk::ScrolledWindow dir_scroll_;
  Gtk::ScrolledWindow file_scroll_;
  Gtk::ListView dir_list_;
  Gtk::ListView file_list_;

  Gtk::Frame sidebar_frame_;
  Gtk::Box sidebar_{Gtk::Orientation::VERTICAL};
  Gtk::Label file_spec_value_;
  Gtk::Label disk_name_value_;
  Gtk::Label avail_value_;
  Gtk::Label stats_heading_;
  Gtk::Label total_files_value_;
  Gtk::Label total_bytes_value_;
  Gtk::Label matching_files_value_;
  Gtk::Label matching_bytes_value_;
  Gtk::Label tagged_files_value_;
  Gtk::Label tagged_bytes_value_;
  Gtk::Label current_heading_;
  Gtk::Label current_name_value_;
  Gtk::Label current_bytes_value_;

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
