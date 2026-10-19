#include <gtkmm.h>

#include "core/domain.hpp"
#include "core/ui.hpp"
#include "ui/gtk/main_window.hpp"
#include "ui/help.hpp"
#include "ui/ui.hpp"

#include <cstring>
#include <optional>
#include <string>

#ifdef Error
# undef Error
#endif

namespace {

int RunDialog(Gtk::Dialog& dialog)
{
  int response = Gtk::ResponseType::CANCEL;
  bool done = false;
  auto connection = dialog.signal_response().connect(
    [&](int r) {
      response = r;
      done = true;
      dialog.set_visible(false);
    }
  );
  dialog.set_modal(true);
  dialog.present();
  auto context = Glib::MainContext::get_default();
  while (!done)
  {
    context->iteration(true);
  }
  // connection lives until dialog returns
  return response;
}

class GtkDialogs final : public ui::IDialogs
{
public:
  void message(const std::string& msg) override
  {
    show_alert("Message", msg);
  }

  void warning(const std::string& msg) override
  {
    show_alert("Warning", msg);
  }

  void error(
    const std::string& msg,
    const std::string& module,
    int line
  ) override
  {
    show_alert(
      "Internal Error",
      msg + "\nIn module \"" + module + "\" line " + std::to_string(line)
    );
  }

  bool confirm(const std::string& message) override
  {
    return choice(message + " (Y/N)", "YN\033") == 'Y';
  }

  int choice(const std::string& msg, const char* term) override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setStatus(msg);
    }

    while (true)
    {
      const auto ev = ui::ReadEvent();
      if (ev.key == ui::Key::Escape)
      {
        return ESC;
      }
      if (ev.key == ui::Key::Enter && std::strchr(term, '\r'))
      {
        return '\r';
      }
      if (ev.key == ui::Key::Char)
      {
        int c = ev.code;
        if (c >= 'a' && c <= 'z')
        {
          c = c - 'a' + 'A';
        }
        if (std::strchr(term, c))
        {
          return c;
        }
      }
    }
  }

  std::optional<std::string> prompt(const ui::PromptRequest& req) override
  {
    Gtk::Dialog dialog("Input");
    if (auto* win = GtkMainWindow())
    {
      dialog.set_transient_for(*win);
    }
    dialog.set_default_size(420, 120);

    auto* box = dialog.get_content_area();
    auto* label = Gtk::make_managed<Gtk::Label>(std::string(req.label));
    auto* entry = Gtk::make_managed<Gtk::Entry>();
    entry->set_text(req.initial);
    entry->set_hexpand(true);
    entry->set_activates_default(true);
    box->set_spacing(8);
    box->set_margin(12);
    box->append(*label);
    box->append(*entry);
    dialog.add_button("_Cancel", Gtk::ResponseType::CANCEL);
    auto* ok = dialog.add_button("_OK", Gtk::ResponseType::OK);
    dialog.set_default_response(Gtk::ResponseType::OK);
    if (ok != nullptr)
    {
      dialog.set_default_widget(*ok);
    }
    entry->signal_activate().connect(
      [&dialog]()
      {
        dialog.response(Gtk::ResponseType::OK);
      }
    );
    entry->grab_focus();

    if (RunDialog(dialog) != Gtk::ResponseType::OK)
    {
      return std::nullopt;
    }
    return std::string(entry->get_text());
  }

  std::optional<ui::AsToPromptResult> promptAsTo(
    const ui::AsToPromptRequest& req
  ) override
  {
    auto as_value = prompt(ui::PromptRequest{req.header, req.as_initial, 0, 0});
    if (!as_value)
    {
      return std::nullopt;
    }
    auto to_value = prompt(ui::PromptRequest{"TO", {}, 0, 0});
    if (!to_value)
    {
      return std::nullopt;
    }
    return ui::AsToPromptResult{*as_value, *to_value};
  }

  bool editFileMode(std::string& modus, const char* /*term*/) override
  {
    auto value = prompt(ui::PromptRequest{"Mode", modus, 0, 0});
    if (!value)
    {
      return false;
    }
    modus = *value;
    return true;
  }

  void hitReturnToContinue() override
  {
    choice("Hit return to continue", "\r\n\033");
  }

  void clearHelp() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->clearHelp();
    }
  }

  void clearPromptLines() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setStatus({});
    }
  }

  void refreshDiskStats() override {}

  void refreshChrome() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->queue_draw();
    }
  }

  void beep() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->error_bell();
    }
  }

  bool keyPressed() override
  {
    if (auto* win = GtkMainWindow())
    {
      return win->hasPendingEvent();
    }
    return false;
  }

  bool escapePressed() override
  {
    return false;
  }

private:
  void show_alert(const std::string& title, const std::string& msg)
  {
    Gtk::Dialog dialog(title);
    if (auto* win = GtkMainWindow())
    {
      dialog.set_transient_for(*win);
    }
    auto* box = dialog.get_content_area();
    auto* label = Gtk::make_managed<Gtk::Label>(msg);
    label->set_wrap(true);
    box->set_margin(12);
    box->append(*label);
    dialog.add_button("_OK", Gtk::ResponseType::OK);
    dialog.set_default_response(Gtk::ResponseType::OK);
    RunDialog(dialog);
  }
};

class GtkSession final : public ui::ISession
{
public:
  void suspend() override {}
  void resume(bool) override {}
  void shutdown() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->close();
    }
  }
  void initClock() override {}
  void suspendClock() override {}
};

class GtkInput final : public ui::IInput
{
public:
  ui::InputEvent readEvent() override
  {
    if (auto* win = GtkMainWindow())
    {
      return win->waitEvent();
    }
    return ui::InputEvent{};
  }

  int readKey() override
  {
    const auto ev = readEvent();
    if (ev.key == ui::Key::Resize)
    {
      return -1;
    }
    if (ev.key == ui::Key::Char || ev.key == ui::Key::Function)
    {
      return ev.code;
    }
    if (ev.key == ui::Key::Enter)
    {
      return '\r';
    }
    if (ev.key == ui::Key::Escape)
    {
      return ESC;
    }
    return ev.raw;
  }

  ui::MouseEvent decodeMouse(ui::MouseFocus) override
  {
    return {};
  }

  int translateOverlayMouse(int ch) override
  {
    return ch;
  }
};

class GtkShellView final : public ui::IShellView
{
public:
  bool refreshDirWindow() override
  {
    return RefreshDirWindow();
  }

  bool internalView(const std::filesystem::path& file_path) override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setStatus("View: " + file_path.string());
    }
    return true;
  }

  void saveHistory() override {}

  void displayDirHelp() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setHelp(DirHelpLines());
    }
  }

  void displayFileHelp() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setHelp(FileHelpLines());
    }
  }

  void displayMenu() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setStatus("ytree GTK");
    }
  }

  void displayAvailBytes() override {}

  void displayFileSpec() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setFileSpec("Filespec: " + statistic.file_spec);
    }
  }

  void displayDiskName() override
  {
    if (auto* win = GtkMainWindow())
    {
      win->setDiskName(disk_statistic.disk_name);
    }
  }

  void relayout() override {}

  void present() override
  {
    while (Glib::MainContext::get_default()->pending())
    {
      Glib::MainContext::get_default()->iteration(false);
    }
  }
};

GtkDialogs g_dialogs;
GtkSession g_session;
GtkInput g_input;
GtkShellView g_shell;

} // namespace

ui::IDialogs& GtkDialogsInstance()
{
  return g_dialogs;
}

ui::ISession& GtkSessionInstance()
{
  return g_session;
}

ui::IInput& GtkInputInstance()
{
  return g_input;
}

ui::IShellView& GtkShellViewInstance()
{
  return g_shell;
}

bool View(DirEntry*, const std::filesystem::path& file_path)
{
  return ui::shell().internalView(file_path);
}

bool ViewHex(const std::filesystem::path& file_path)
{
  if (auto* win = GtkMainWindow())
  {
    win->setStatus("Hex view (stub): " + file_path.string());
  }
  return true;
}

std::optional<SortSpec> GetKindOfSort()
{
  return statistic.kind_of_sort;
}
