#include "ytree.h"

void InitClock()
{
#if defined(CLOCK_SUPPORT)
  struct itimerval value;
  struct itimerval ovalue;

  print_time = true;

  if (getitimer(ITIMER_REAL, &value) != 0)
  {
    ErrorPrintf("gettimer() failed:*%s", std::strerror(errno));
  }

  value.it_interval.tv_sec = CLOCK_INTERVAL;
  value.it_value.tv_sec = CLOCK_INTERVAL;
  value.it_interval.tv_usec = 0;

  if (setitimer(ITIMER_REAL, &value, &ovalue) != 0)
  {
    ErrorPrintf("setitimer() failed:*%s", std::strerror(errno));
  }

  ClockHandler(0);
#endif
}

void ClockHandler(int sig)
{
#if defined(CLOCK_SUPPORT)
  if (COLS > 15 && print_time)
  {
    std::time_t hora;
    struct tm* hora_tm;
    char strtm[23];

    std::time(&hora);
    hora_tm = std::localtime(&hora);

    std::snprintf(
      strtm,
      sizeof(strtm),
      "[time %.2d:%.2d:%.2d]",
      hora_tm->tm_hour,
      hora_tm->tm_min,
      hora_tm->tm_sec
    );
# if defined(COLOR_SUPPORT)
    mvwaddch(time_window, 0, 0, ACS_RTEE | COLOR_PAIR(MENU_COLOR) | A_BOLD);
    mvwaddch(time_window, 0, 14, ACS_LTEE | COLOR_PAIR(MENU_COLOR) | A_BOLD);
# else
    mvwaddch(time_window, 0, 0, ACS_RTEE);
    mvwaddch(time_window, 0, 14, ACS_LTEE);
# endif
    PrintMenuOptions(time_window, 0, 1, strtm, MENU_COLOR, HIMENUS_COLOR);
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
