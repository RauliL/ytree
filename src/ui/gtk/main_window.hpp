#pragma once

#include <gtkmm.h>

#include "core/domain.hpp"
#include "core/ui.hpp"
#include "ui/file_format.hpp"
#include "ui/help.hpp"

#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

class YTreeMainWindow : public Gtk::ApplicationWindow
{
public:
  YTreeMainWindow();

  void pushEvent(ui::InputEvent ev);
  ui::InputEvent waitEvent();
  bool hasPendingEvent() const;

  void pushMouse(ui::MouseFocus pane, ui::MouseAction action, int index);
  ui::MouseEvent takeMouse(ui::MouseFocus focus);

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

  void setDirTable(
    ViewMode mode,
    const std::vector<TableRow>& rows,
    int hilight
  );
  void selectDirRow(int hilight);
  void setFileTable(
    ViewMode mode,
    const std::vector<TableRow>& rows,
    int hilight
  );
  void selectFileRow(int hilight);
  void setFileTableMessage(const Glib::ustring& message);

  Gtk::ColumnView& dirTable() { return dir_table_; }
  Gtk::ColumnView& fileTable() { return file_table_; }

private:
  bool on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
  void buildSidebar();
  void setStatValue(Gtk::Label& label, std::int64_t value);
  void addSidebarRow(const Glib::ustring& caption, Gtk::Label& value);

  void configureColumnView(Gtk::ColumnView& view);
  void clearColumns(Gtk::ColumnView& view);
  Glib::RefPtr<Gtk::ColumnViewColumn> makeColumn(
    ui::MouseFocus pane,
    const TableColumn& spec,
    guint column_index
  );
  void rebuildColumns(
    Gtk::ColumnView& view,
    ui::MouseFocus pane,
    std::optional<ViewMode>& mode_slot,
    ViewMode mode,
    const std::vector<TableColumn>& specs
  );
  void setTable(
    Gtk::ColumnView& view,
    ui::MouseFocus pane,
    Glib::RefPtr<Gio::ListStoreBase>& model_slot,
    std::optional<ViewMode>& mode_slot,
    ViewMode mode,
    const std::vector<TableColumn>& specs,
    const std::vector<TableRow>& rows,
    int hilight
  );
  void selectRow(Gtk::ColumnView& view, int hilight);
  void attachRowClickHandlers(
    ui::MouseFocus pane,
    const Glib::RefPtr<Gtk::ListItem>& item,
    Gtk::Widget& widget
  );

  Gtk::Box root_{Gtk::Orientation::VERTICAL};
  Gtk::Label path_;
  Gtk::Box body_{Gtk::Orientation::HORIZONTAL};
  Gtk::Box lists_{Gtk::Orientation::VERTICAL};
  Gtk::ScrolledWindow dir_scroll_;
  Gtk::ScrolledWindow file_scroll_;
  Gtk::ColumnView dir_table_;
  Gtk::ColumnView file_table_;

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

  Glib::RefPtr<Gio::ListStoreBase> dir_model_;
  Glib::RefPtr<Gio::ListStoreBase> file_model_;
  std::optional<ViewMode> dir_table_mode_;
  std::optional<ViewMode> file_table_mode_;

  mutable std::mutex queue_mutex_;
  std::deque<ui::InputEvent> event_queue_;
  std::optional<ui::MouseFocus> pending_mouse_pane_;
  ui::MouseEvent pending_mouse_{};
};

YTreeMainWindow* GtkMainWindow();
void SetGtkMainWindow(YTreeMainWindow* window);
