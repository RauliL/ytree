#include <cstring>

#include <fcntl.h>

#include "core/api.hpp"
#include "core/commands.hpp"
#include "core/domain.hpp"
#include "core/path.hpp"
#include "core/profile.hpp"
#include "core/stat-macros.hpp"
#include "core/ui.hpp"
#include "ui/mouse.hpp"
#include "ui/curses/curses_ui.hpp"
#include "util/string.hpp"

typedef struct MODIF {
    long pos;
    unsigned char old_char;
    unsigned char new_char;
    struct MODIF *next;
} CHANGES;

static CHANGES *changes;
static int cursor_pos_x;
static int cursor_pos_y;

static long current_line;
static int fd, fd2;
static struct stat fdstat;
static int WLINES, WCOLS, BYTES;
static WINDOW *VIEW, *BORDER;

bool inhex=true;
bool inedit=false;
bool hexoffset=true;


#define CURSOR_CALC_X (10+((cursor_pos_x<(BYTES))? 2:3)+(cursor_pos_x)+(cursor_pos_x/2))
#define CURSOR_POS_X ((inhex)? CURSOR_CALC_X:(WCOLS-BYTES+cursor_pos_x))
#define C_POSX (((cursor_pos_x%2)!=1)? cursor_pos_x:(cursor_pos_x-1))
#define CURSOR_POSX ((inhex)? (C_POSX/2):cursor_pos_x)
#define CANTX(x) ((inhex)? (x*2):x)
#define THECOLOR ((inedit)? COLOR_PAIR(STATS_COLOR):COLOR_PAIR(DIR_COLOR))

static bool ViewFile(DirEntry* dir_entry, const std::filesystem::path& file_path);
static bool ViewArchiveFile(const std::filesystem::path& file_path);

bool View(DirEntry* dir_entry, const std::filesystem::path& file_path)
{
  switch (mode)
  {
    using enum Mode;
    case DISK_MODE:
    case USER_MODE:
      return ViewFile(dir_entry, file_path);

    case TAPE_MODE:
    case RAR_FILE_MODE:
    case SEVENZIP_FILE_MODE:
    case RPM_FILE_MODE:
    case TAR_FILE_MODE:
    case ZOO_FILE_MODE:
    case ZIP_FILE_MODE:
    case LHA_FILE_MODE:
    case ARC_FILE_MODE:
      return ViewArchiveFile(file_path);

    default:
      beep();
  }

  return false;
}

static bool ViewFile(DirEntry* dir_entry, const std::filesystem::path& file_path)
{
  std::string command_line;
  const auto file_p_aux = ShellQuote(file_path.string());
  bool result = false;
  bool notice_mapped = false;

  if (!IsReadable(file_path))
  {
    FormatMessage(
      "View not possible!*\"{}\"*{}",
      file_path.c_str(),
      std::strerror(errno)
    );

    return false;
  }

  if (const auto aux = GetExtViewer(file_path))
  {
    if (const auto pos = aux->find("%s"); pos != std::string::npos)
    {
      command_line = *aux;
      command_line.replace(pos, 2, file_p_aux);
    } else {
      command_line = Join(*aux, file_p_aux);
    }
  } else {
    const auto compress_method = GetFileMethod(file_path);

    if (
      compress_method && (
        *compress_method == CompressMethod::FREEZE_COMPRESS ||
        *compress_method == CompressMethod::COMPRESS_COMPRESS ||
        *compress_method == CompressMethod::GZIP_COMPRESS ||
        *compress_method == CompressMethod::BZIP_COMPRESS ||
        *compress_method == CompressMethod::XZ_COMPRESS ||
        *compress_method == CompressMethod::ZSTD_COMPRESS
      )
    )
    {
      const auto uncompress_command = GetProfileValueOrEmpty(
        *compress_method == CompressMethod::FREEZE_COMPRESS ? "MELT" :
        *compress_method == CompressMethod::COMPRESS_COMPRESS ? "UNCOMPRESS" :
        *compress_method == CompressMethod::GZIP_COMPRESS ? "GNUUNZIP" :
        *compress_method == CompressMethod::BZIP_COMPRESS ? "BUNZIP" :
        *compress_method == CompressMethod::XZ_COMPRESS ? "UNXZ" :
        "UNZSTD"
      );

      command_line = Join(
        uncompress_command,
        "<",
        file_p_aux,
        ERR_TO_STDOUT,
        "|",
        GetProfileValueOrEmpty("PAGER")
      );
    } else {
      command_line = Join(GetProfileValueOrEmpty("PAGER"), file_p_aux);
    }
  }

/* --crb3 01oct02: replicating what I did to <e>dit, eliminate
the problem with jstar-chained-thru-less writing new files to
the ytree starting cwd. new code grabbed from execute.c.
*/


  if (mode == Mode::DISK_MODE)
  {
    const auto cwd = GetcwdOrDot();
    const auto path = GetPath(dir_entry);
    std::error_code ec;

    std::filesystem::current_path(path, ec);
    if (ec)
    {
      FormatMessage("Can't change directory to*\"{}\"", path.string());
    } else {
      result = SystemCall(command_line);
    }
    std::filesystem::current_path(cwd, ec);
    if (ec)
    {
      FormatMessage("Can't change directory to*\"{}\"", cwd.string());
    }
  } else {
    result = SystemCall(command_line);
  }

  if (!result)
  {
    FormatMessage("can't execute*{}", command_line);
  }

  if (notice_mapped)
  {
    UnmapNoticeWindow();
  }

  return result;
}

static bool ViewArchiveFile(const std::filesystem::path& file_path)
{
  std::string command_line;
  std::string buffer = "| ";
  bool result = false;

  if (const auto aux = GetExtViewer(file_path))
  {
    if (aux->find("%s") != std::string::npos)
    {
      buffer += GetProfileValueOrEmpty("PAGER");
    } else {
      // Maybe pipe-able
      buffer += *aux;
    }
  } else {
    buffer += GetProfileValueOrEmpty("PAGER");
  }
  command_line = MakeExtractCommandLine(
    mode == Mode::TAPE_MODE ? statistic.tape_name : statistic.login_path,
    file_path,
    buffer
  );
  result = SystemCall(command_line);
  if (!result)
  {
    FormatMessage("can't execute*{}", command_line.c_str());
  }

  return result;
}

static void printhexline(WINDOW *win, char *line, char *buf, int r, long offset)
{
    int i;
    if (r==0)
    {
    wclrtoeol(win);
    return;
    }
    std::string out = hexoffset
      ? std::format("{:010X}  ", static_cast<int>(offset))
      : std::format("{:010}  ", static_cast<int>(offset));
    for (i = 1; i <= r; i++ )
    {
        if ((i == (BYTES / 2) ) || (i == BYTES ))
        out += std::format("{:02X}  ", static_cast<unsigned char>(buf[i-1]));
        else
        out += std::format("{:02X} ", static_cast<unsigned char>(buf[i-1]));
    }
    for (i = r+1; i <= BYTES; i++)
    {
        buf[i-1]= ' ';
        if ((i == (BYTES / 2) ) || (i == BYTES ))
        out += "    ";
        else
        out += "   ";
    }
    out += ' ';
    *std::format_to(line, "{}", out) = '\0';
    for (i=0; i< WCOLS-BYTES; i++)
    waddch(win, line[i]| THECOLOR);
    for( i=0; i< BYTES; i++)
    std::isprint(buf[i]) ? waddch(win, buf[i] | THECOLOR) :
              waddch(win, ACS_BLOCK | COLOR_PAIR(HIDIR_COLOR));
    return;
}

static void update_line(WINDOW *win, long line)
{
    int r;
    std::vector<unsigned char> buf(BYTES);
    std::vector<char> line_string(WCOLS);
    char mensaje[50];

    std::memset(line_string.data(), ' ', WCOLS);
    line_string[0] = '\0';
    std::memset(buf.data(), ' ', BYTES);
    if (lseek(fd, (line - 1) * BYTES, SEEK_SET)== -1 )
    {
        *std::format_to(mensaje, "Error {} ", line) = '\0';
    std::perror(mensaje);
    std::fflush(stdout);
    return;
    }
    r = read(fd, buf.data(), BYTES);
    printhexline(win, line_string.data(), reinterpret_cast<char*>(buf.data()), r, (line - 1) * (BYTES));
}

static void scroll_down(WINDOW *win)
{
    scrollok(win,true);
    wscrl(win,1);
    scrollok(win,false);
    wmove(win, WLINES - 1 , 0);
    update_line(win, current_line + WLINES - 1);
    wnoutrefresh(win);
    doupdate();
}

static void scroll_up(WINDOW *win)
{
    scrollok(win,true);
    wscrl(win,-1);
    scrollok(win,false);
    wmove(win, 0, 0);
    update_line(win, current_line);
    wnoutrefresh(win);
    doupdate();
}

static void update_all_lines(WINDOW *win, char l)
{
    long i;

    for (i = current_line; i <= current_line + l; i++)
    {
    wmove(win, i - current_line, 0);
    update_line(win, i);
    }
    wnoutrefresh(win);
    doupdate();
}

static void Change2Edit(const std::string& file_path)
{
    int i;

    for(i = WLINES + 4; i < LINES; i++)
    {
    wmove(stdscr,i , 0);
    wclrtoeol(stdscr);
    }
    doupdate();

    Print(stdscr, 0, 0, "File: ", MENU_COLOR);
    Print(stdscr, 0, 6, CutPathname(file_path, WCOLS-5), HIMENUS_COLOR);
    PrintOptions(stdscr, LINES - 3, 0, "(Edit file in hexadecimal mode)");
    PrintOptions(stdscr, LINES - 2, 0, "(Q)uit   (^L) redraw  (<TAB>) change edit mode");
    PrintOptions(stdscr, LINES - 1, 0,
        "(NEXT)-(RIGHT)/(PREV)-(LEFT) page   (HOME)-(END) of line   (DOWN)-(UP) line");
    return;
}

static void Change2View(const std::string& file_path)
{
    int i;

    for(i = WLINES + 4; i < LINES; i++)
    {
    wmove(stdscr,i , 0);
    wclrtoeol(stdscr);
    }
    doupdate();

    Print(stdscr, 0, 0, "File: ", MENU_COLOR);
    Print(stdscr, 0, 6, CutPathname(file_path, WCOLS - 5), HIMENUS_COLOR);
    PrintOptions(stdscr, LINES - 3, 0, "View file in hexadecimal mode");
    PrintOptions(stdscr, LINES - 2, 0, "(Q)uit   (^L) redraw  (E)dit hex");
    PrintOptions(stdscr, LINES - 1, 0,
        "(NEXT)-(RIGHT)/(PREV)-(LEFT) page   (HOME)-(END) of line   (DOWN)-(UP) line");
    return;
}

static void SetupViewWindow(const std::string& file_path)
{
    int i;

    WLINES= LINES - 6;
    WCOLS= COLS - 2;
    if (BORDER)
    delwin(BORDER);
    BORDER=newwin(WLINES + 2, WCOLS + 2, 1, 0);
    if (VIEW)
    delwin(VIEW);
    VIEW=newwin(WLINES, WCOLS, 2, 1);
    keypad(VIEW,true);
    scrollok(VIEW,false);
    clearok(VIEW,true);
    leaveok(VIEW,false);
/*    werase(VIEW);*/
    WbkgdSet(VIEW,COLOR_PAIR(WINDIR_COLOR));
    wclear(VIEW);
    for( i = 0; i < WLINES - 1; i++)
    {
    wmove(VIEW,i,0);
    wclrtoeol(VIEW);
    }
    WbkgdSet(BORDER,COLOR_PAIR(WINDIR_COLOR)|A_BOLD);
    box(BORDER,0,0);
    RefreshWindow(BORDER);
    RefreshWindow(VIEW);
    Change2View(file_path);
    BYTES = (WCOLS - 13) / 4;
    return;

}



static ui::InputEvent ReadViewerEvent()
{
  if (resize_request)
  {
    return ui::InputEvent::Special(ui::Key::Resize);
  }

  ui::InputEvent ev = ui::ReadEvent();
#ifdef VI_KEYS
  ev = EventFromLegacyChar(ViKey(LegacyCode(ev)));
#endif
  if (ev.key == ui::Key::Mouse)
  {
    const int translated = ui::input().translateOverlayMouse(
      ev.raw != 0 ? ev.raw : KEY_MOUSE
    );
    if (translated == KEY_UP)
    {
      return ui::InputEvent::Special(ui::Key::Up);
    }
    if (translated == KEY_DOWN)
    {
      return ui::InputEvent::Special(ui::Key::Down);
    }
    return ui::InputEvent{};
  }
  return ev;
}

static inline unsigned char hexval(unsigned char v)
{
  if (v >= 'a' && v <= 'f')
  {
    v = v - 'a' + 10;
  }
  else if (v >= '0' && v <= '9')
  {
    v = v - '0';
  }

  return v;
}


static void change_char(int ch)
{

    CHANGES *cambio=nullptr;
    char pp=0;
    char mensaje[50];

    cambio = static_cast<CHANGES*>(std::malloc(sizeof(struct MODIF)));
    cambio -> pos = ( (cursor_pos_y + current_line - 1) * BYTES) + CURSOR_POSX;
    if (lseek(fd, cambio -> pos, SEEK_SET)== -1 )
    {
        *std::format_to(mensaje, "Error {} ", std::strerror(errno)) = '\0';
    std::perror(mensaje);
    std::fflush(stdout);
    std::free(cambio);
    return;
    }
    if ((read(fd, &cambio -> old_char,1)==1))

    if (lseek(fd, cambio -> pos, SEEK_SET)!= -1 )
    {
    if (inhex) {
        switch( ch){
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        if ((cursor_pos_x%2)==1)
            pp = (cambio -> old_char & 0xF0) | (hexval(ch));
        else
            pp = (cambio -> old_char & 0x0F) | (hexval(ch) << 4);
        touchwin(VIEW);
        break;
        default:
        beep();
        touchwin(VIEW);
        std::free(cambio);
        return;
        break;
        }
    }else{
        pp = ch;
    }
        if (write(fd, &pp, 1)!= 1)
    {
            *std::format_to(mensaje, "Error al grabar el cambio {} ", std::strerror(errno)) = '\0';
        std::perror(mensaje);
        std::fflush(stdout);
        std::free(cambio);
        return;
    }
    cambio -> new_char = pp;
    cambio -> next = changes;
    changes = cambio;
    }else{
        *std::format_to(mensaje, "Error al posicionar {} ", std::strerror(errno)) = '\0';
        std::perror(mensaje);
        std::fflush(stdout);
        std::free(cambio);
        return;
    }
    else{
        *std::format_to(mensaje, "Error al pre-leer {} ", std::strerror(errno)) = '\0';
        std::perror(mensaje);
        std::fflush(stdout);
        std::free(cambio);
        return;
    }
    return;
}


static void move_right(WINDOW *win)
{
   fstat(fd,&fdstat);
   cursor_pos_x++;
   if (fdstat.st_size > ((cursor_pos_y+current_line-1) * BYTES + CURSOR_POSX )){
    cursor_pos_x--;
    if ( cursor_pos_x < CANTX(BYTES) - 1 ) {
            cursor_pos_x += 1;
        wmove(win, cursor_pos_y, CURSOR_POS_X);
    }else {
        if (fdstat.st_size >= ((current_line+cursor_pos_y) * BYTES) ){
        if (cursor_pos_y < WLINES-1 ) {
            cursor_pos_y++;
            cursor_pos_x = 0;
            wmove(win, cursor_pos_y, CURSOR_POS_X);
        } else {
            current_line++;
            scroll_down(win);
            cursor_pos_x = 0;
            wmove(win, cursor_pos_y, CURSOR_POS_X);
        }
        } else
        beep();
    }
    }else{
    cursor_pos_x--;
    beep();
    }
    return;
}

static void hex_edit(const std::string& file_path)
{
    bool QUIT=false;

    cursor_pos_x = cursor_pos_y = 0;
    fd2 = fd;
    fd=open(file_path.c_str(),O_RDWR);
    if (fd == -1){
    FormatError("Error {}", std::strerror(errno));
    touchwin(VIEW);
        fd = fd2;
    return;
    }
    inedit=true;
    update_all_lines(VIEW,WLINES-1);
    ui::ViewerSurface().leaveOk(false);
    ui::ViewerSurface().setCursorVisible(true);
    ui::ViewerSurface().moveCursor(cursor_pos_y, CURSOR_POS_X);
    ui::ViewerSurface().present();
    while (!QUIT) {
    ui::ViewerSurface().present();
    const ui::InputEvent ev = ReadViewerEvent();
    const ViewerCommand cmd = MapViewerCommand(ev, true, inhex);
       if (cmd == ViewerCommand::Resize || resize_request)
       {
            SetupViewWindow(file_path);
        Change2Edit(file_path);
/*      current_line = oldpos/BYTES;*/
        update_all_lines(VIEW,WLINES-1);
        ui::ViewerSurface().moveCursor(cursor_pos_y, CURSOR_POS_X);
        ui::ViewerSurface().present();
    }

    switch(cmd){
        case ViewerCommand::Quit: QUIT=true;
              break;
        case ViewerCommand::MoveDown: /*ScrollDown();*/
               fstat(fd,&fdstat);
               if (fdstat.st_size > ((cursor_pos_y + current_line - 1) * BYTES + CURSOR_POSX)) {

                if (fdstat.st_size > ((cursor_pos_y + current_line - 1 + 1) * BYTES + CURSOR_POSX)) {

                    if (cursor_pos_y < WLINES-1){
                            wmove(VIEW, ++cursor_pos_y, CURSOR_POS_X);
                            wnoutrefresh(VIEW);
                    } else {
                            ++current_line;
                            scroll_down(VIEW);
                            wmove(VIEW, cursor_pos_y, CURSOR_POS_X);
                            wnoutrefresh(VIEW);
                    }
                } else {
                    /* special case: last line */

                    if (fdstat.st_size > ((cursor_pos_y + current_line - 1 + 1) * BYTES)) {

                        for(cursor_pos_x = 0; (CURSOR_POSX + 1) < (fdstat.st_size % BYTES); cursor_pos_x++);

                        if (cursor_pos_y < WLINES-1){
                            wmove(VIEW, ++cursor_pos_y, CURSOR_POS_X);
                            wnoutrefresh(VIEW);
                        } else {
                            ++current_line;
                            scroll_down(VIEW);
                            wmove(VIEW, cursor_pos_y, CURSOR_POS_X);
                            wnoutrefresh(VIEW);
                        }
                    }
                }
                } else {
                beep();
                }
                break;
        case ViewerCommand::MoveUp: /*ScroollUp();*/
            if (cursor_pos_y > 0)
            {
                wmove(VIEW, --cursor_pos_y, CURSOR_POS_X);
                    wnoutrefresh(VIEW);
            } else if (current_line > 1) {
                current_line--;
                scroll_up(VIEW);
                wmove(VIEW, cursor_pos_y, CURSOR_POS_X);
                    wnoutrefresh(VIEW);
            } else
                beep();
                    break;
        case ViewerCommand::MoveLeft: /* move 1 char left */
                if ( cursor_pos_x > 0 ) {
                    cursor_pos_x-=1;
                wmove(VIEW, cursor_pos_y, CURSOR_POS_X);
                } else if (cursor_pos_y > 0 ) {
                /*cursor_pos_x=ultimo_caracter;*/
                cursor_pos_x=CANTX(BYTES) - 1;
                wmove(VIEW, --cursor_pos_y,CURSOR_POS_X);
                } else if (current_line > 1) {
                current_line--;
                scroll_up(VIEW);
                cursor_pos_x=CANTX(BYTES) - 1;
                wmove(VIEW, cursor_pos_y, CURSOR_POS_X);
                } else
                beep();
            wnoutrefresh(VIEW);
            break;
        case ViewerCommand::PageUp: /*ScrollPageDown();*/
                if (current_line > WLINES)
                current_line -= WLINES;
                else
                if (current_line > 1)
                   current_line = 1;
                else
                    beep();
/*              oldpos = current_line * BYTES;*/
                update_all_lines(VIEW,WLINES);
                wmove(VIEW, cursor_pos_y, CURSOR_POS_X);
                wnoutrefresh(VIEW);
                break;
        case ViewerCommand::MoveRight: move_right(VIEW);
                wnoutrefresh(VIEW);
                    break;
        case ViewerCommand::PageDown: /*ScroollPageUp();*/
                fstat(fd,&fdstat);
                if (fdstat.st_size > ((current_line - 1 + WLINES + cursor_pos_y) * BYTES) + CURSOR_POSX ) {
                current_line += WLINES;
                } else {
                    int n;
                n = fdstat.st_size / BYTES; /* numer of full lines */
                if(fdstat.st_size % BYTES) {
                  n++; /* plus 1 not fully used line */
                }
                if(current_line != n) {
                  current_line = n;
                  cursor_pos_y = 0;
                  for(cursor_pos_x = 0; (CURSOR_POSX + 1) < (fdstat.st_size % BYTES); cursor_pos_x++);
                } else {
                  beep();
                }
                }
/*              oldpos = current_line * BYTES;*/
                update_all_lines(VIEW,WLINES);
                wmove(VIEW, cursor_pos_y, CURSOR_POS_X);
                wnoutrefresh(VIEW);
                    break;
        case ViewerCommand::Home:
                if (CURSOR_POSX > 0) {
                cursor_pos_x = 0;
                wmove(VIEW, cursor_pos_y, CURSOR_POS_X);
                } else
                beep();
                wnoutrefresh(VIEW);
                        break;
        case ViewerCommand::End:
             fstat(fd,&fdstat);
             if ( ((cursor_pos_y + current_line) * BYTES) > fdstat.st_size ) {
                    cursor_pos_x = CANTX(fdstat.st_size % BYTES) - 1;
             } else {
                cursor_pos_x = CANTX(BYTES)-1;
             }
             wmove(VIEW, cursor_pos_y, CURSOR_POS_X);
             wnoutrefresh(VIEW);
                     break;
        case ViewerCommand::TogglePane:
            /* move cursor to the the other part of the window*/
            if (inhex){
                inhex=false;
                cursor_pos_x=cursor_pos_x/2;
            }else{
                inhex=true;
                cursor_pos_x=cursor_pos_x*2;
            }
            wmove(VIEW, cursor_pos_y, CURSOR_POS_X);
            wnoutrefresh(VIEW);
            break;
        case ViewerCommand::Redraw:
            ui::ViewerSurface().forceFullRedraw();
            RefreshWindow(stdscr);
            break;

        case ViewerCommand::CharInput:
            change_char(ev.code);
            wmove(VIEW, cursor_pos_y, 0);
            update_line(VIEW, current_line+cursor_pos_y);
            move_right(VIEW);
            wmove(VIEW, cursor_pos_y, CURSOR_POS_X);
            wnoutrefresh(VIEW);
            break;
        case ViewerCommand::Idle:
        case ViewerCommand::Resize:
        case ViewerCommand::Edit:
        case ViewerCommand::Unknown:
            break;
    }
    }
    ui::ViewerSurface().setCursorVisible(false);
    close(fd);
    fd=fd2;
    inedit=false;
    return;
}


bool InternalView(const std::filesystem::path& file_path)
{
    long oldpos;
    bool QUIT=false;

    hexoffset = GetProfileValueOrEmpty("HEXEDITOFFSET") == "HEX";

    if (stat(file_path.c_str(), &fdstat)!=0)
    return false;
    if (!(S_ISREG(fdstat.st_mode)) || S_ISBLK(fdstat.st_mode))
    return false;
    fd=open(file_path.c_str(),O_RDONLY);
    if (fd == -1)
    return false;
    SetupViewWindow(file_path);
    current_line = 1;
    oldpos = 1;
    update_all_lines(VIEW,WLINES-1);
    while (!QUIT) {
    const ui::InputEvent ev = ReadViewerEvent();
    const ViewerCommand cmd = MapViewerCommand(ev, false, true);
       if (cmd == ViewerCommand::Resize || resize_request)
       {
            SetupViewWindow(file_path);
        current_line = oldpos/BYTES;
        update_all_lines(VIEW,WLINES-1);
    }

    switch(cmd){
        case ViewerCommand::Quit: QUIT=true;
              break;
        case ViewerCommand::Edit: Change2Edit(file_path);
                  hex_edit(file_path);
              update_all_lines(VIEW,WLINES-1);
              Change2View(file_path);
              break;
        case ViewerCommand::MoveDown: /*ScrollDown();*/
               fstat(fd,&fdstat);
               if (fdstat.st_size > (current_line * BYTES) )
               {
                current_line++;
                oldpos = current_line * BYTES;
                scroll_down(VIEW);
                }
                else
                beep();
                break;
        case ViewerCommand::MoveUp: /*ScroollUp();*/
            if (current_line > 1)
            {
                current_line--;
                oldpos = current_line * BYTES;
                scroll_up(VIEW);
            }
            else
                beep();
                    break;
        case ViewerCommand::MoveLeft:
        case ViewerCommand::PageUp: /*ScrollPageDown();*/
                if (current_line > WLINES)
                current_line -= WLINES;
                else
                if (current_line > 1)
                   current_line = 1;
                else
                    beep();
                oldpos = current_line * BYTES;
                update_all_lines(VIEW,WLINES);
                break;
        case ViewerCommand::MoveRight:
        case ViewerCommand::PageDown: /*ScroollPageUp();*/
                fstat(fd,&fdstat);
                if (fdstat.st_size > ((current_line - 1 + WLINES) * BYTES) ) {
                current_line += WLINES;
                } else {
                    int n;
                n = fdstat.st_size / BYTES; /* numer of full lines */
                if(fdstat.st_size % BYTES) {
                  n++; /* plus 1 not fully used line */
                }
                if(current_line != n) {
                  current_line = n;
                } else {
                  beep();
                }
                }
                oldpos = current_line * BYTES;
                update_all_lines(VIEW,WLINES);
                    break;
        case ViewerCommand::Home: /*ScrollHome();*/
                if (current_line > 1)
                {
                current_line = 1;
                oldpos = current_line * BYTES;
                update_all_lines(VIEW,WLINES-1);
                }else
                beep();
                        break;
        case ViewerCommand::End: /*ScrollEnd();*/
             fstat(fd,&fdstat);
             if (fdstat.st_size >= BYTES * 2)
                current_line = (fdstat.st_size - BYTES) / BYTES;
             else
                beep();
            oldpos = current_line * BYTES;
            update_all_lines(VIEW,WLINES);
                    break;
        case ViewerCommand::Redraw:
            ui::ViewerSurface().forceFullRedraw();
            RefreshWindow(stdscr);
            break;
        case ViewerCommand::Unknown:
        case ViewerCommand::Idle:
        case ViewerCommand::TogglePane:
        case ViewerCommand::CharInput:
        case ViewerCommand::Resize:
            break;
    }
    }
    wmove(stdscr, 0, 0);
    wclrtoeol(stdscr);
    Print(stdscr, 0, 0, "Path: ", MENU_COLOR);
    if (COLS > 10)
    {
      const auto width = static_cast<std::size_t>(COLS - 10);
      Print(
        stdscr,
        0,
        6,
        std::format("{:<{}}", FormFilename(statistic.path, width), width),
        HIMENUS_COLOR
      );
    }
    for (int row = LINES - 3; row < LINES; ++row)
    {
      if (row >= 0)
      {
        wmove(stdscr, row, 0);
        wclrtoeol(stdscr);
      }
    }
    delwin(VIEW);
    delwin(BORDER);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    close(fd);
    return true;
}

namespace {

class NcursesViewerSurface final : public ui::IViewerSurface
{
public:
  void present() override
  {
    if (VIEW)
    {
      wnoutrefresh(VIEW);
    }
    doupdate();
  }

  void moveCursor(int row, int col) override
  {
    if (VIEW)
    {
      wmove(VIEW, row, col);
      wnoutrefresh(VIEW);
    }
  }

  void setCursorVisible(bool visible) override
  {
    curs_set(visible ? 1 : 0);
  }

  void leaveOk(bool enabled) override
  {
    if (VIEW)
    {
      leaveok(VIEW, enabled);
    }
  }

  void forceFullRedraw() override
  {
    clearok(stdscr, true);
  }

  void touch() override
  {
    if (VIEW)
    {
      touchwin(VIEW);
    }
  }
};

NcursesViewerSurface g_ncurses_viewer_surface;

} // namespace

ui::IViewerSurface& NcursesViewerSurfaceInstance()
{
  return g_ncurses_viewer_surface;
}
