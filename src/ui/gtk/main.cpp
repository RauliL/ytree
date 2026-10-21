#include <clocale>
#include <cstdlib>

#include <sys/stat.h>

#include <gtkmm.h>

#include "core/api.hpp"
#include "core/profile.hpp"
#include "ui/gtk/gtk_frontend.hpp"
#include "ui/gtk/main_window.hpp"
#include "ui/ui.hpp"

#ifdef Error
# undef Error
#endif

static void RegisterGtkBackends()
{
  ui::set_backend(
    &GtkDialogsInstance(),
    &GtkSessionInstance(),
    &GtkInputInstance(),
    &GtkShellViewInstance(),
    &GtkDirViewInstance(),
    &GtkFileViewInstance(),
    &GtkViewerSurfaceInstance()
  );
}

static void LoadProfile(
  const std::optional<std::filesystem::path>& configuration_file,
  const std::optional<std::filesystem::path>& history_file
)
{
  user_umask = umask(0);
  ReadProfile(configuration_file);
  (void)history_file;
  SetFileMode(static_cast<ViewMode>(GetIntProfileValue("FILEMODE")));
  statistic.kind_of_sort = {SortKey::Name, SortOrder::Ascending};
  bypass_small_window = GetBooleanProfileValue("NOSMALLWINDOW");
  initial_directory = GetProfileValue("INITIALDIR");
}

int main(int argc, char** argv)
{
  std::setlocale(LC_ALL, "");

  // Parse ytree options before Gtk::Application sees argv; otherwise a path
  // argument is treated as an unknown GApplication option / open request.
  const auto opts = ParseAppOptions(argc, argv);
  auto app = Gtk::Application::create(
    "org.ytree.gtk",
    Gio::Application::Flags::NON_UNIQUE
  );

  app->signal_activate().connect([app, opts]() {
    auto* window = new YTreeMainWindow();
    SetGtkMainWindow(window);
    RegisterGtkBackends();
    LoadProfile(opts.config_file, opts.history_file);

    const auto path = ResolveStartupPath(opts.initial_dir);
    statistic.login_path.clear();
    statistic.path.clear();

    if (!LoginDisk(path))
    {
      std::exit(1);
    }

    window->setPath(path);
    app->add_window(*window);
    window->present();

    Glib::signal_idle().connect_once([]() {
      while (true)
      {
        if (HandleDirWindow(statistic.tree.get()) == 'q')
        {
          Quit();
          break;
        }
      }
    });
  });

  // Only pass the program name to GTK so our [-p/-h] [path] args are not
  // re-parsed or rejected by GApplication.
  char* gtk_argv[] = { argv[0], nullptr };
  int gtk_argc = 1;
  return app->run(gtk_argc, gtk_argv);
}
