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

class TableRowObject : public Glib::Object
{
public:
  static Glib::RefPtr<TableRowObject> create(std::vector<Glib::ustring> cells)
  {
    return Glib::make_refptr_for_instance<TableRowObject>(
      new TableRowObject(std::move(cells))
    );
  }

  const std::vector<Glib::ustring>& cells() const { return cells_; }

protected:
  explicit TableRowObject(std::vector<Glib::ustring> cells)
  : Glib::ObjectBase(typeid(TableRowObject))
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

void YTreeMainWindow::configureColumnView(Gtk::ColumnView& view)
{
  view.set_show_column_separators(true);
  view.set_show_row_separators(false);
  view.set_reorderable(false);
  view.set_can_focus(false);
  view.set_focusable(false);
}

void YTreeMainWindow::clearColumns(Gtk::ColumnView& view)
{
  auto columns = view.get_columns();
  while (columns && columns->get_n_items() > 0)
  {
    auto col = std::dynamic_pointer_cast<Gtk::ColumnViewColumn>(
      columns->get_object(0)
    );
    if (!col)
    {
      break;
    }
    view.remove_column(col);
  }
}

void YTreeMainWindow::attachRowClickHandlers(
  ui::MouseFocus pane,
  const Glib::RefPtr<Gtk::ListItem>& item,
  Gtk::Widget& widget
)
{
  auto left = Gtk::GestureClick::create();
  left->set_button(1);
  left->signal_pressed().connect(
    [this, pane, item](int n_press, double, double) {
      const int index = static_cast<int>(item->get_position());
      if (index < 0)
      {
        return;
      }
      pushMouse(
        pane,
        n_press >= 2 ? ui::MouseAction::Activate : ui::MouseAction::Select,
        index
      );
    }
  );
  widget.add_controller(left);

  auto right = Gtk::GestureClick::create();
  right->set_button(3);
  right->signal_pressed().connect(
    [this, pane, item](int, double, double) {
      const int index = static_cast<int>(item->get_position());
      if (index < 0)
      {
        return;
      }
      pushMouse(pane, ui::MouseAction::Tag, index);
    }
  );
  widget.add_controller(right);
}

Glib::RefPtr<Gtk::ColumnViewColumn> YTreeMainWindow::makeColumn(
  ui::MouseFocus pane,
  const TableColumn& spec,
  guint column_index
)
{
  const float xalign = spec.xalign;
  auto factory = Gtk::SignalListItemFactory::create();
  factory->signal_setup().connect(
    [this, pane, xalign](const Glib::RefPtr<Gtk::ListItem>& item) {
      auto* box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
      box->set_hexpand(true);
      auto* label = Gtk::make_managed<Gtk::Label>();
      label->set_xalign(xalign);
      label->set_ellipsize(Pango::EllipsizeMode::END);
      label->set_hexpand(true);
      box->append(*label);
      item->set_child(*box);
      attachRowClickHandlers(pane, item, *box);
    }
  );
  factory->signal_bind().connect(
    [column_index](const Glib::RefPtr<Gtk::ListItem>& item) {
      auto* box = dynamic_cast<Gtk::Box*>(item->get_child());
      auto* label = box ? dynamic_cast<Gtk::Label*>(box->get_first_child()) : nullptr;
      auto row = std::dynamic_pointer_cast<TableRowObject>(item->get_item());
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

void YTreeMainWindow::rebuildColumns(
  Gtk::ColumnView& view,
  ui::MouseFocus pane,
  std::optional<ViewMode>& mode_slot,
  ViewMode mode,
  const std::vector<TableColumn>& specs
)
{
  clearColumns(view);
  for (guint i = 0; i < specs.size(); ++i)
  {
    view.append_column(makeColumn(pane, specs[i], i));
  }
  mode_slot = mode;
}

void YTreeMainWindow::setTable(
  Gtk::ColumnView& view,
  ui::MouseFocus pane,
  Glib::RefPtr<Gio::ListStoreBase>& model_slot,
  std::optional<ViewMode>& mode_slot,
  ViewMode mode,
  const std::vector<TableColumn>& specs,
  const std::vector<TableRow>& rows,
  int hilight
)
{
  if (!mode_slot || *mode_slot != mode)
  {
    rebuildColumns(view, pane, mode_slot, mode, specs);
  }
  else
  {
    const auto current = view.get_columns() ? view.get_columns()->get_n_items() : 0u;
    if (current != specs.size())
    {
      rebuildColumns(view, pane, mode_slot, mode, specs);
    }
  }

  auto store = Gio::ListStore<TableRowObject>::create();
  for (const auto& row : rows)
  {
    std::vector<Glib::ustring> cells;
    cells.reserve(row.cells.size());
    for (const auto& cell : row.cells)
    {
      cells.emplace_back(cell);
    }
    store->append(TableRowObject::create(std::move(cells)));
  }
  model_slot = store;

  auto selection = Gtk::SingleSelection::create(store);
  view.set_model(selection);
  if (!rows.empty())
  {
    selectRow(view, hilight);
  }
}

void YTreeMainWindow::selectRow(Gtk::ColumnView& view, int hilight)
{
  auto selection = std::dynamic_pointer_cast<Gtk::SingleSelection>(view.get_model());
  if (!selection || selection->get_n_items() == 0)
  {
    return;
  }
  const guint idx = static_cast<guint>(std::clamp(
    hilight,
    0,
    static_cast<int>(selection->get_n_items()) - 1
  ));
  selection->set_selected(idx);
  view.scroll_to(
    idx,
    {},
    Gtk::ListScrollFlags::SELECT | Gtk::ListScrollFlags::FOCUS
  );
}

YTreeMainWindow::YTreeMainWindow()
{
  set_title("ytree");
  set_default_size(1024, 700);

  configureColumnView(dir_table_);
  configureColumnView(file_table_);
  rebuildColumns(
    dir_table_,
    ui::MouseFocus::Dir,
    dir_table_mode_,
    ViewMode::MODE_3,
    DirTableColumnsForMode(ViewMode::MODE_3)
  );
  rebuildColumns(
    file_table_,
    ui::MouseFocus::File,
    file_table_mode_,
    ViewMode::MODE_3,
    FileTableColumnsForMode(ViewMode::MODE_3)
  );

  dir_scroll_.set_child(dir_table_);
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

void YTreeMainWindow::pushMouse(
  ui::MouseFocus pane,
  ui::MouseAction action,
  int index
)
{
  std::lock_guard lock(queue_mutex_);
  pending_mouse_pane_ = pane;
  pending_mouse_ = {};
  pending_mouse_.action = action;
  pending_mouse_.index = index;
  event_queue_.push_back(ui::InputEvent::Special(ui::Key::Mouse));
}

ui::MouseEvent YTreeMainWindow::takeMouse(ui::MouseFocus focus)
{
  std::lock_guard lock(queue_mutex_);
  if (!pending_mouse_pane_)
  {
    return {};
  }

  const ui::MouseFocus pane = *pending_mouse_pane_;
  const ui::MouseEvent event = pending_mouse_;
  pending_mouse_pane_.reset();
  pending_mouse_ = {};

  if (pane != focus)
  {
    ui::MouseEvent switched{};
    switched.action = (pane == ui::MouseFocus::Dir)
      ? ui::MouseAction::SwitchToDir
      : ui::MouseAction::SwitchToFile;
    return switched;
  }
  return event;
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

void YTreeMainWindow::setDirTable(
  ViewMode mode,
  const std::vector<TableRow>& rows,
  int hilight
)
{
  setTable(
    dir_table_,
    ui::MouseFocus::Dir,
    dir_model_,
    dir_table_mode_,
    mode,
    DirTableColumnsForMode(mode),
    rows,
    hilight
  );
}

void YTreeMainWindow::selectDirRow(int hilight)
{
  selectRow(dir_table_, hilight);
}

void YTreeMainWindow::setFileTable(
  ViewMode mode,
  const std::vector<TableRow>& rows,
  int hilight
)
{
  setTable(
    file_table_,
    ui::MouseFocus::File,
    file_model_,
    file_table_mode_,
    mode,
    FileTableColumnsForMode(mode),
    rows,
    hilight
  );
}

void YTreeMainWindow::selectFileRow(int hilight)
{
  selectRow(file_table_, hilight);
}

void YTreeMainWindow::setFileTableMessage(const Glib::ustring& message)
{
  const ViewMode mode = file_table_mode_.value_or(ViewMode::MODE_3);
  TableRow row;
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
