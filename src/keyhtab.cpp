#include "ytree.h"
#include "tilde.h"

static std::vector<std::string> Mtchs;
static int total_matches  = 0;
static int cursor_pos     = 0;
static int disp_begin_pos = 1;

static std::string LongestCommonPrefix(const std::vector<std::string>& matches)
{
  if (matches.empty())
  {
    return {};
  }

  std::string prefix = matches.front();
  for (std::size_t i = 1; i < matches.size(); ++i)
  {
    const auto& candidate = matches[i];
    const auto n = std::min(prefix.size(), candidate.size());
    std::size_t j = 0;
    while (j < n && prefix[j] == candidate[j])
    {
      ++j;
    }
    prefix.resize(j);
    if (prefix.empty())
    {
      break;
    }
  }
  return prefix;
}

static std::vector<std::string> filename_completion_matches(const std::string& text)
{
  const auto expanded = tilde_expand(text);

  std::string dir_part;
  std::string name_prefix;
  std::string display_prefix;

  const auto sep = expanded.find_last_of('/');

  std::error_code ec;
  std::vector<std::string> matches;

  if (sep == std::string::npos)
  {
    dir_part = ".";
    name_prefix = expanded;
    display_prefix.clear();
  } else {
    dir_part = expanded.substr(0, sep);
    if (dir_part.empty())
    {
      dir_part = "/";
    }
    name_prefix = expanded.substr(sep + 1);
    display_prefix = expanded.substr(0, sep + 1);
  }

  if (!std::filesystem::is_directory(dir_part, ec))
  {
    return {};
  }

  for (
    std::filesystem::directory_iterator it(dir_part, ec), end;
    !ec && it != end;
    it.increment(ec)
  )
  {
    const auto name = it->path().filename().string();

    if (name == "." || name == "..")
    {
      continue;
    }
    else if (name.compare(0, name_prefix.size(), name_prefix) != 0)
    {
      continue;
    }

    auto entry = display_prefix + name;
    std::error_code entry_ec;

    if (std::filesystem::is_directory(it->path(), entry_ec))
    {
      entry.push_back('/');
    }
    matches.push_back(std::move(entry));
  }

  if (matches.empty())
  {
    return {};
  }

  std::sort(matches.begin(), matches.end());

  const auto common = LongestCommonPrefix(matches);
  std::vector<std::string> result;

  result.reserve(matches.size() + 1);
  result.push_back(common);
  result.insert(
    result.end(),
    std::make_move_iterator(matches.begin()),
    std::make_move_iterator(matches.end())
  );

  return result;
}

void PrintMtchEntry(int entry_no, int y, int color,
                   int start_x, int *hide_left, int *hide_right)
{
  int     n;
  char    buffer[BUFSIZ];
  char    *line_ptr;
  int     window_width;
  int     window_height;
  int     ef_window_width;


  GetMaxYX(matches_window, &window_height, &window_width);
  ef_window_width = window_width - 2; /* Effektive Window-Width */

#ifdef NO_HIGHLIGHT
  ef_window_width = window_width - 3; /* Effektive Window-Width */
#else
  ef_window_width = window_width - 2; /* Effektive Window-Width */
#endif

  *hide_left = *hide_right = 0;

  if (entry_no >= 0 && static_cast<std::size_t>(entry_no) < Mtchs.size())
  {
    std::strncpy(buffer, Mtchs[entry_no].c_str(), BUFSIZ - 3);
    buffer[BUFSIZ - 3] = '\0';
    n = std::strlen(buffer);
    wmove(matches_window,y,1);

    if(n <= ef_window_width) {

      /* will completely fit into window */
      /*---------------------------------*/

      line_ptr = buffer;
    } else {
      /* does not completely fit into window;
       * ==> use start_x
       */

      if(n > (start_x + ef_window_width))
        line_ptr = &buffer[start_x];
      else
        line_ptr = &buffer[n - ef_window_width];

      *hide_left = start_x;
      *hide_right = n - start_x - ef_window_width;

      line_ptr[ef_window_width] ='\0';
    }

#ifdef NO_HIGHLIGHT
    {
      const auto display = std::string(line_ptr) + ((color == HIMTCH_COLOR) ? " <" : "  ");
      WAddStr(matches_window, display);
    }
#else
#ifdef COLOR_SUPPORT
    WbkgdSet(matches_window, COLOR_PAIR(color)|A_BOLD);
#else
    if(color == HIMTCH_COLOR)
      wattrset(matches_window, A_REVERSE);
#endif /* COLOR_SUPPORT */
    WAddStr(matches_window, line_ptr);
#ifdef COLOR_SUPPORT
    WbkgdSet(matches_window, COLOR_PAIR(WINMTCH_COLOR)| A_BOLD);
#else
    if(color == HIMTCH_COLOR)
      wattrset(matches_window, 0);
#endif /* COLOR_SUPPORT */
#endif /* NO_HIGHLIGHT */
  }
  return;
}

int DisplayMatches()
{
  int i, hilight_no, p_y;
  int hide_left, hide_right;

  hilight_no = disp_begin_pos + cursor_pos;
  p_y = -1;
  werase(matches_window);
  for(i=0; i < MATCHES_WINDOW_HEIGHT; i++)
  {
    if (disp_begin_pos + i >= total_matches ) break;
    if (disp_begin_pos + i != hilight_no )
        PrintMtchEntry(disp_begin_pos + i, i, MTCH_COLOR,
                0, &hide_left, &hide_right);
    else
      p_y = i;
  }
  if(p_y >= 0) {
    PrintMtchEntry(disp_begin_pos + p_y, p_y, HIMTCH_COLOR,
            0, &hide_left, &hide_right);
  }
  return 0;
}

std::optional<std::string> GetMatches(const std::string& base)
{
  int     ch;
  int     start_x;
  std::optional<std::string> RetVal;
  int     hide_left, hide_right;

  Mtchs.clear();

  const auto tmpval = tilde_expand(base);
  auto match_list = filename_completion_matches(tmpval);
  if (match_list.empty())
  {
    return std::nullopt;
  }

  if (tmpval != match_list[0])
  {
    return match_list[0];
  }

  total_matches = static_cast<int>(match_list.size());
  if (total_matches == 1)
  {
    return std::nullopt;
  }

  Mtchs = std::move(match_list);

  disp_begin_pos = 1;
  cursor_pos     = 0;
  start_x        = 0;
  /* leaveok(stdscr, true); */
  DisplayMatches();

  do
  {
    RefreshWindow(matches_window);
    doupdate();
    ch = Getch();
    ch = TranslateOverlayMouse(ch);

    if(ch != -1 && ch != KEY_RIGHT && ch != KEY_LEFT) {
      if(start_x) {
        start_x = 0;
  PrintMtchEntry(disp_begin_pos + cursor_pos,
           cursor_pos, HIMTCH_COLOR,
           start_x, &hide_left, &hide_right);
      }
    }

    switch( ch )
    {
      case -1:       RetVal = std::nullopt;
                     break;

      case ' ':      break;  /* Quick-Key */

      case KEY_RIGHT: start_x++;
          PrintMtchEntry(disp_begin_pos + cursor_pos,
                   cursor_pos, HIMTCH_COLOR,
                         start_x, &hide_left, &hide_right);
          if(hide_right < 0)
            start_x--;
          break;

      case KEY_LEFT:  if(start_x > 0)
                  start_x--;
          PrintMtchEntry(disp_begin_pos + cursor_pos,
                   cursor_pos, HIMTCH_COLOR,
                         start_x, &hide_left, &hide_right);
          break;

      case '\t':
      case KEY_DOWN: if (disp_begin_pos + cursor_pos+1 >= total_matches)
               {
           beep();
         }
         else
         { if( cursor_pos + 1 < MATCHES_WINDOW_HEIGHT )
           {
       PrintMtchEntry(disp_begin_pos + cursor_pos,
          cursor_pos, MTCH_COLOR,
                            start_x, &hide_left, &hide_right);
       cursor_pos++;
       PrintMtchEntry(disp_begin_pos + cursor_pos,
          cursor_pos, HIMTCH_COLOR,
                            start_x, &hide_left, &hide_right);
                       }
           else
           {
       PrintMtchEntry(disp_begin_pos + cursor_pos,
          cursor_pos, MTCH_COLOR,
                            start_x, &hide_left, &hide_right);
       scroll(matches_window);
       disp_begin_pos++;
       PrintMtchEntry(disp_begin_pos + cursor_pos,
          cursor_pos, HIMTCH_COLOR,
                            start_x, &hide_left, &hide_right);
                       }
         }
                     break;
      case KEY_BTAB:
      case KEY_UP  : if( disp_begin_pos + cursor_pos - 1 < 1 )
         {   beep(); }
         else
         {
           if( cursor_pos - 1 >= 0 )
           {
       PrintMtchEntry(disp_begin_pos + cursor_pos,
          cursor_pos, MTCH_COLOR,
                            start_x, &hide_left, &hide_right);
       cursor_pos--;
       PrintMtchEntry(disp_begin_pos + cursor_pos,
          cursor_pos, HIMTCH_COLOR,
                            start_x, &hide_left, &hide_right);
                       }
           else
           {
       PrintMtchEntry(disp_begin_pos + cursor_pos,
          cursor_pos, MTCH_COLOR,
                            start_x, &hide_left, &hide_right);
       wmove(matches_window, 0, 0);
       winsertln(matches_window);
       disp_begin_pos--;
       PrintMtchEntry(disp_begin_pos + cursor_pos,
          cursor_pos, HIMTCH_COLOR,
                            start_x, &hide_left, &hide_right);
                       }
         }
                     break;
      case KEY_NPAGE:
               if( disp_begin_pos + cursor_pos >= total_matches - 1 )
         {  beep();  }
         else
         {
           if( cursor_pos < MATCHES_WINDOW_HEIGHT - 1 )
           {
       PrintMtchEntry(disp_begin_pos + cursor_pos,
          cursor_pos, MTCH_COLOR,
                            start_x, &hide_left, &hide_right);
             if( disp_begin_pos + MATCHES_WINDOW_HEIGHT > total_matches  - 1 )
         cursor_pos = total_matches - disp_begin_pos - 1;
       else
         cursor_pos = MATCHES_WINDOW_HEIGHT - 1;
       PrintMtchEntry(disp_begin_pos + cursor_pos,
          cursor_pos, HIMTCH_COLOR,
                            start_x, &hide_left, &hide_right);
           }
           else
           {
       if( disp_begin_pos + cursor_pos + MATCHES_WINDOW_HEIGHT < total_matches )
       {
         disp_begin_pos += MATCHES_WINDOW_HEIGHT;
         cursor_pos = MATCHES_WINDOW_HEIGHT - 1;
       }
       else
       {
         disp_begin_pos = total_matches - MATCHES_WINDOW_HEIGHT;
         if( disp_begin_pos < 1 ) disp_begin_pos = 1;
         cursor_pos = total_matches - disp_begin_pos - 1;
       }
                         DisplayMatches();
           }
         }
                     break;
      case KEY_PPAGE:
         if( disp_begin_pos + cursor_pos <= 1 )
         {  beep();  }
         else
         {
           if( cursor_pos > 0 )
           {
       PrintMtchEntry(disp_begin_pos + cursor_pos,
          cursor_pos, MTCH_COLOR,
                            start_x, &hide_left, &hide_right);
       cursor_pos = 0;
       PrintMtchEntry(disp_begin_pos + cursor_pos,
          cursor_pos, HIMTCH_COLOR,
                            start_x, &hide_left, &hide_right);
           }
           else
           {
       if( (disp_begin_pos -= MATCHES_WINDOW_HEIGHT) < 1 )
       {
         disp_begin_pos = 1;
       }
                         cursor_pos = 0;
                         DisplayMatches();
           }
         }
                     break;
      case KEY_HOME: if( disp_begin_pos == 1 && cursor_pos == 0 )
         {   beep();    }
         else
         {
           disp_begin_pos = 1;
           cursor_pos     = 0;
                       DisplayMatches();
         }
                     break;
      case KEY_END :
                     disp_begin_pos = std::max(1, total_matches - MATCHES_WINDOW_HEIGHT);
         cursor_pos     = total_matches - disp_begin_pos - 1;
                     DisplayMatches();
                     break;
      case LF :
      case CR :
                     RetVal = Mtchs[disp_begin_pos + cursor_pos];
                     break;

      case ESC:      RetVal = std::nullopt;
                     break;

      default :      beep();
         break;
    } /* switch */
  } while(ch != CR && ch != ESC && ch != -1);
  /* leaveok(stdscr, false); */
  Mtchs.clear();
  touchwin(stdscr);
  return RetVal;
}
