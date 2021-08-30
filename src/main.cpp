/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/main.c,v 1.20 2008/07/24 17:43:04 werner Exp $
 *
 * Hauptmodul
 *
 ***************************************************************************/


#include "ytree.h"



static char buffer[PATH_LENGTH+1];
static char path[PATH_LENGTH+1];


int main(int argc, char **argv)
{
  const char *p;
  int argi;
  std::optional<std::string> config_file;
  std::optional<std::string> history_file;

#if (!defined(sun) && !defined(__DJGPP__))
  setlocale(LC_ALL, "");
#endif

  p = DEFAULT_TREE;
  for (argi = 1; argi < argc; argi++)
  {
    if (*(argv[argi]) != '-')
    {
      p = argv[argi];
      break;
    }
    switch (*(argv[argi]+1))
    {
      case 'p':
      case 'P':
        if (*(argv[argi] + 2) <= ' ')
        {
          config_file = argv[++argi];
        } else {
          config_file = argv[argi] + 2;
        }
        break;

      case 'h':
      case 'H':
        if (*(argv[argi] + 2) <= ' ')
        {
          history_file = argv[++argi];
        } else {
          history_file = argv[argi] + 2;
        }
        break;

      default:
        std::printf(
          "Usage: %s [-p profile_file] [-h hist_file] [initial_dir]\n",
          argv[0]
        );
        std::exit(EXIT_FAILURE);
        break;
    }
  }

  if (Init(config_file, history_file))
  {
    std::exit(EXIT_FAILURE);
  }

  if (*p != FILE_SEPARATOR_CHAR)
  {
    /* rel. Pfad */
    /*-----------*/
    if (const auto cwd = Getcwd())
    {
      std::strcpy(buffer, cwd->c_str());
    }
    std::strcat(buffer, FILE_SEPARATOR_STRING);
    std::strcat(buffer, p);
    p = buffer;
  }

  /* Normalize path */

  NormPath( p, path );

  statistic.login_path[0] = '\0';
  statistic.path[0] = '0';

  if( LoginDisk( path ) == -1 )
  {
    endwin();
    exit( 1 );
  }

  while( 1 )
  {
    if( HandleDirWindow(statistic.tree) == 'q' ) Quit();
  }
}


