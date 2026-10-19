#include <gtkmm.h>

#include "core/domain.hpp"
#include "ui/format.hpp"
#include "ui/gtk/main_window.hpp"

#ifdef Error
# undef Error
#endif

#include <algorithm>
#include <format>

namespace {

YTreeMainWindow* g_main_window = nullptr;

class FileRowObject : public Glib::Object
{
public:
  static Glib::RefPtr<FileRowObject> create(std::vector<Glib::ustring> cells)
  {
    return Glib::make_refptr_for_instance<FileRowObject>(
      new FileRowObject(std::move(cells))
    );
  }

  const std::vector<Glib::ustring>& cells() const { return cells_; }

protected:
  explicit FileRowObject(std::vector<Glib::ustring> cells)
  : Glib::ObjectBase(typeid(FileRowObject))
  , Glib::Object()
  , cells_(std::move(cells))
  {
  }

private:
  std::vector<Glib::ustring> cells_;
};

bool HasControl(Gdk::ModifierType state)
{
  return (state & Gdk::ModifierType::CONTROL_MASK) == Gdk::ModifierType::CONTROL_MASK;
}

ui::InputEvent KeyvalToEvent(guint keyval, Gdk::ModifierType state)
{
  using ui::Key;

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

void StyleMono(Gtk::Label& label)
{
  label.set_xalign(0.0f);
  label.add_css_class("monospace");
}

Gtk::Label& MakeHeading(Gtk::Box& parent, const Glib::ustring& text)
{
  auto* label = Gtk::make_managed<Gtk::Label>(text);
  label->set_xalign(0.0f);
  label->add_css_class("heading");
  parent.append(*label);
  return *label;
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

void YTreeMainWindow::bindListView(Gtk::ListView& view)
{
  auto factory = Gtk::SignalListItemFactory::create();
  factory->signal_setup().connect([](const Glib::RefPtr<Gtk::ListItem>& item) {
    auto* label = Gtk::make_managed<Gtk::Label>();
    label->set_xalign(0.0f);
    label->set_ellipsize(Pango::EllipsizeMode::END);
    item->set_child(*label);
  });
  factory->signal_bind().connect([](const Glib::RefPtr<Gtk::ListItem>& item) {
    auto* label = dynamic_cast<Gtk::Label*>(item->get_child());
    auto obj = item->get_item();
    auto* row = dynamic_cast<Gtk::StringObject*>(obj ? obj.get() : nullptr);
    if (label && row)
    {
      label->set_text(row->get_string());
    }
  });
  view.set_factory(factory);
}

void YTreeMainWindow::applyRows(
  Gtk::ListView& view,
  Glib::RefPtr<Gtk::StringList>& model,
  const std::vector<Glib::ustring>& rows,
  int hilight
)
{
  model = Gtk::StringList::create(rows);
  auto selection = Gtk::SingleSelection::create(model);
  if (!rows.empty())
  {
    selection->set_selected(static_cast<guint>(
      std::clamp(hilight, 0, static_cast<int>(rows.size()) - 1)
    ));
  }
  view.set_model(selection);
}

void YTreeMainWindow::clearFileColumns()
{
  auto columns = file_table_.get_columns();
  while (columns && columns->get_n_items() > 0)
  {
    auto col = std::dynamic_pointer_cast<Gtk::ColumnViewColumn>(
      columns->get_object(0)
    );
    if (!col)
    {
      break;
    }
    file_table_.remove_column(col);
  }
}

Glib::RefPtr<Gtk::ColumnViewColumn> YTreeMainWindow::makeFileColumn(
  const FileTableColumn& spec,
  guint column_index
)
{
  const float xalign = spec.xalign;
  auto factory = Gtk::SignalListItemFactory::create();
  factory->signal_setup().connect([xalign](const Glib::RefPtr<Gtk::ListItem>& item) {
    auto* label = Gtk::make_managed<Gtk::Label>();
    label->set_xalign(xalign);
    label->set_ellipsize(Pango::EllipsizeMode::END);
    item->set_child(*label);
  });
  factory->signal_bind().connect(
    [column_index](const Glib::RefPtr<Gtk::ListItem>& item) {
      auto* label = dynamic_cast<Gtk::Label*>(item->get_child());
      auto row = std::dynamic_pointer_cast<FileRowObject>(item->get_item());
      if (!label || !row)
      {
        return;
      }
      if (column_index < row->cells().size())
      {
        label->set_text(row->cells()[column_index]);
      }
      else
      {
        label->set_text({});
      }
    }
  );

  auto column = Gtk::ColumnViewColumn::create(spec.title, factory);
  column->set_expand(spec.expand);
  column->set_resizable(true);
  return column;
}

void YTreeMainWindow::rebuildFileColumns(ViewMode mode)
{
  clearFileColumns();
  const auto specs = FileTableColumnsForMode(mode);
  for (guint i = 0; i < specs.size(); ++i)
  {
    file_table_.append_column(makeFileColumn(specs[i], i));
  }
  file_table_mode_ = mode;
}

YTreeMainWindow::YTreeMainWindow()
{
  set_title("ytree");
  set_default_size(1024, 700);

  dir_model_ = Gtk::StringList::create({});
  bindListView(dir_list_);
  dir_list_.set_model(Gtk::SingleSelection::create(dir_model_));
  dir_list_.set_can_focus(false);
  dir_list_.set_focusable(false);

  file_table_.set_show_column_separators(true);
  file_table_.set_show_row_separators(false);
  file_table_.set_reorderable(false);
  file_table_.set_can_focus(false);
  file_table_.set_focusable(false);
  rebuildFileColumns(ViewMode::MODE_3);

  dir_scroll_.set_child(dir_list_);
  file_scroll_.set_child(file_table_);
  dir_scroll_.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
  file_scroll_.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
  dir_scroll_.set_vexpand(true);
  file_scroll_.set_vexpand(true);
  dir_scroll_.set_hexpand(true);
  file_scroll_.set_hexpand(true);
  dir_scroll_.set_size_request(-1, 120);
  file_scroll_.set_size_request(-1, 120);

  lists_.set_spacing(6);
  lists_.set_homogeneous(true);
  lists_.set_hexpand(true);
  lists_.set_vexpand(true);
  lists_.append(dir_scroll_);
  lists_.append(file_scroll_);

  buildSidebar();
  sidebar_frame_.set_child(sidebar_);
  sidebar_frame_.set_label("Stats");
  sidebar_frame_.set_margin_start(8);
  sidebar_frame_.set_size_request(240, -1);

  body_.set_spacing(0);
  body_.set_vexpand(true);
  body_.append(lists_);
  body_.append(sidebar_frame_);

  path_.set_xalign(0.0f);
  path_.add_css_class("monospace");
  help_line0_.set_xalign(0.0f);
  help_line1_.set_xalign(0.0f);
  help_line0_.add_css_class("monospace");
  help_line1_.add_css_class("monospace");
  help_box_.append(help_line0_);
  help_box_.append(help_line1_);
  status_.set_wrap(true);
  status_.set_xalign(0.0f);

  root_.set_margin(8);
  root_.set_spacing(6);
  root_.append(path_);
  root_.append(body_);
  root_.append(help_box_);
  root_.append(status_);
  set_child(root_);

  auto controller = Gtk::EventControllerKey::create();
  controller->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
  controller->signal_key_pressed().connect(
    sigc::mem_fun(*this, &YTreeMainWindow::on_key_pressed),
    false
  );
  add_controller(controller);

  setStatus("Ready");
  setHelp(DirHelpLines());
  showDiskStatistics();
}

void YTreeMainWindow::buildSidebar()
{
  sidebar_.set_spacing(2);
  sidebar_.set_margin(8);

  MakeHeading(sidebar_, "FILE");
  StyleMono(file_spec_value_);
  file_spec_value_.set_text("[]");
  sidebar_.append(file_spec_value_);

  MakeHeading(sidebar_, "DISK");
  StyleMono(disk_name_value_);
  disk_name_value_.set_text("[]");
  sidebar_.append(disk_name_value_);

  auto* avail_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
  auto* avail_caption = Gtk::make_managed<Gtk::Label>("Avail");
  avail_caption->set_xalign(0.0f);
  StyleMono(avail_value_);
  avail_value_.set_text(FormatStatNumber(0));
  avail_row->append(*avail_caption);
  avail_row->append(avail_value_);
  sidebar_.append(*avail_row);

  stats_heading_.set_text("DISK Statistics");
  stats_heading_.set_xalign(0.0f);
  stats_heading_.add_css_class("heading");
  sidebar_.append(stats_heading_);
  MakeHeading(sidebar_, "Total");
  addSidebarRow("Files", total_files_value_);
  addSidebarRow("Bytes", total_bytes_value_);
  MakeHeading(sidebar_, "Matching");
  addSidebarRow("Files", matching_files_value_);
  addSidebarRow("Bytes", matching_bytes_value_);
  MakeHeading(sidebar_, "Tagged");
  addSidebarRow("Files", tagged_files_value_);
  addSidebarRow("Bytes", tagged_bytes_value_);

  current_heading_.set_text("Current Directory");
  current_heading_.set_xalign(0.0f);
  current_heading_.add_css_class("heading");
  sidebar_.append(current_heading_);
  StyleMono(current_name_value_);
  current_name_value_.set_text("[]");
  sidebar_.append(current_name_value_);
  addSidebarRow("Bytes", current_bytes_value_);
}

void YTreeMainWindow::addSidebarRow(const Glib::ustring& caption, Gtk::Label& value)
{
  auto* row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
  auto* cap = Gtk::make_managed<Gtk::Label>(caption);
  cap->set_xalign(0.0f);
  cap->set_size_request(56, -1);
  StyleMono(value);
  value.set_text(FormatStatNumber(0));
  value.set_hexpand(true);
  row->append(*cap);
  row->append(value);
  sidebar_.append(*row);
}

void YTreeMainWindow::setStatValue(Gtk::Label& label, std::int64_t value)
{
  label.set_text(FormatStatNumber(value));
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
  path_.set_text(std::format("Path: {}", std::string(text)));
}

void YTreeMainWindow::setFileSpec(const std::string& spec)
{
  file_spec_value_.set_text(std::format("[{:<17}]", spec));
}

void YTreeMainWindow::setDiskName(const std::string& name)
{
  disk_name_value_.set_text(std::format("[{:<17}]", name));
}

void YTreeMainWindow::setAvailKb(std::int64_t avail_kb)
{
  setStatValue(avail_value_, avail_kb);
}

void YTreeMainWindow::showDiskStatistics()
{
  stats_heading_.set_text("DISK Statistics");
  current_heading_.set_text("Current Directory");
  setFileSpec(statistic.file_spec);
  setDiskName(statistic.disk_name);
  setAvailKb(statistic.disk_space / 1024);
  setStatValue(total_files_value_, statistic.disk_total_files);
  setStatValue(total_bytes_value_, statistic.disk_total_bytes);
  setStatValue(matching_files_value_, statistic.disk_matching_files);
  setStatValue(matching_bytes_value_, statistic.disk_matching_bytes);
  setStatValue(tagged_files_value_, statistic.disk_tagged_files);
  setStatValue(tagged_bytes_value_, statistic.disk_tagged_bytes);
}

void YTreeMainWindow::showDirStatistics(const ui::DirPanelInfo& info)
{
  stats_heading_.set_text("DIR Statistics");
  current_heading_.set_text("Current File");
  setPath(info.path_line);
  setStatValue(total_files_value_, info.total_files);
  setStatValue(total_bytes_value_, info.total_bytes);
  setStatValue(matching_files_value_, info.matching_files);
  setStatValue(matching_bytes_value_, info.matching_bytes);
  setStatValue(tagged_files_value_, info.tagged_files);
  setStatValue(tagged_bytes_value_, info.tagged_bytes);
}

void YTreeMainWindow::setTagged(const ui::TaggedCounts& counts)
{
  setStatValue(tagged_files_value_, counts.files);
  setStatValue(tagged_bytes_value_, counts.bytes);
}

void YTreeMainWindow::setCurrentDirectory(
  const std::string& name,
  std::int64_t bytes
)
{
  current_heading_.set_text("Current Directory");
  current_name_value_.set_text(std::format("[{:<20}]", name));
  setStatValue(current_bytes_value_, bytes);
}

void YTreeMainWindow::setCurrentFile(const std::string& name, std::int64_t size)
{
  current_heading_.set_text("Current File");
  current_name_value_.set_text(std::format("[{:<20}]", name));
  setStatValue(current_bytes_value_, size);
}

void YTreeMainWindow::setDirRows(const std::vector<Glib::ustring>& rows, int hilight)
{
  applyRows(dir_list_, dir_model_, rows, hilight);
}

void YTreeMainWindow::setFileTable(
  ViewMode mode,
  const std::vector<FileTableRow>& rows,
  int hilight
)
{
  if (!file_table_mode_ || *file_table_mode_ != mode)
  {
    rebuildFileColumns(mode);
  }
  else
  {
    // Link column may appear/disappear when the directory contents change.
    const auto expected = FileTableColumnsForMode(mode).size();
    const auto current = file_table_.get_columns()
      ? file_table_.get_columns()->get_n_items()
      : 0u;
    if (current != expected)
    {
      rebuildFileColumns(mode);
    }
  }

  auto store = Gio::ListStore<FileRowObject>::create();
  for (const auto& row : rows)
  {
    std::vector<Glib::ustring> cells;
    cells.reserve(row.cells.size());
    for (const auto& cell : row.cells)
    {
      cells.emplace_back(cell);
    }
    store->append(FileRowObject::create(std::move(cells)));
  }
  file_model_ = store;

  auto selection = Gtk::SingleSelection::create(store);
  if (!rows.empty())
  {
    selection->set_selected(static_cast<guint>(
      std::clamp(hilight, 0, static_cast<int>(rows.size()) - 1)
    ));
  }
  file_table_.set_model(selection);
}

void YTreeMainWindow::setFileTableMessage(const Glib::ustring& message)
{
  const ViewMode mode = file_table_mode_.value_or(ViewMode::MODE_3);
  rebuildFileColumns(mode);

  FileTableRow row;
  const auto cols = FileTableColumnsForMode(mode);
  row.cells.assign(cols.size(), {});
  if (row.cells.size() >= 2)
  {
    row.cells[1] = message;
  }
  else if (!row.cells.empty())
  {
    row.cells[0] = message;
  }
  setFileTable(mode, {row}, 0);
}
