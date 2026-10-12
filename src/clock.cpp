#include "ytree.h"

#include <chrono>

void InitClock()
{
#if defined(CLOCK_SUPPORT)
  struct itimerval value;
  struct itimerval ovalue;

  print_time = true;

  if (getitimer(ITIMER_REAL, &value) != 0)
  {
    FormatError("gettimer() failed:*{}", std::strerror(errno));
  }

  value.it_interval.tv_sec = CLOCK_INTERVAL;
  value.it_value.tv_sec = CLOCK_INTERVAL;
  value.it_interval.tv_usec = 0;

  if (setitimer(ITIMER_REAL, &value, &ovalue) != 0)
  {
    FormatError("setitimer() failed:*{}", std::strerror(errno));
  }

  ClockHandler(0);
#endif
}

void ClockHandler(int sig)
{
#if defined(CLOCK_SUPPORT)
  if (COLS > 15 && print_time)
  {
    std::optional<std::string> time_label;
#if defined(YTREE_HAS_STD_CHRONO_TZ)
    const auto local_now = std::chrono::zoned_time{
      std::chrono::current_zone(),
      std::chrono::system_clock::now()
    };
    time_label = std::format("[time {:%H:%M:%S}]", local_now);
#else
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local_tm{};
    char buffer[32];
    if(localtime_r(&now, &local_tm) != nullptr
        && std::strftime(buffer, sizeof(buffer), "[time %H:%M:%S]", &local_tm) != 0)
      time_label = buffer;
#endif

    if(time_label)
    {
# if defined(COLOR_SUPPORT)
      mvwaddch(time_window, 0, 0, ACS_RTEE | COLOR_PAIR(MENU_COLOR) | A_BOLD);
      mvwaddch(time_window, 0, 14, ACS_LTEE | COLOR_PAIR(MENU_COLOR) | A_BOLD);
# else
      mvwaddch(time_window, 0, 0, ACS_RTEE);
      mvwaddch(time_window, 0, 14, ACS_LTEE);
# endif
      PrintMenuOptions(
        time_window,
        0,
        1,
        *time_label,
        MENU_COLOR,
        HIMENUS_COLOR
      );
    }
  }
  std::signal(SIGALRM, ClockHandler);
#endif
}

void SuspendClock()
{
#if defined(CLOCK_SUPPORT)
  struct itimerval value;
  struct itimerval ovalue;

  value.it_interval.tv_sec = 0;
  value.it_value.tv_sec = 0;
  value.it_interval.tv_usec = 0;
  setitimer(ITIMER_REAL, &value, &ovalue);
  std::signal(SIGALRM, SIG_IGN);
#endif
}
