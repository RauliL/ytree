#include "ytree.h"
#include "xmalloc.h"

#include <unordered_map>

static const std::unordered_map<std::string, CompressMethod> file_extensions =
{
  { ".TAP", CompressMethod::TAPE_DIR_NO_COMPRESS },
  { ".tap", CompressMethod::TAPE_DIR_NO_COMPRESS },
  { ".TAP.F", CompressMethod::TAPE_DIR_FREEZE_COMPRESS },
  { ".tap.F", CompressMethod::TAPE_DIR_FREEZE_COMPRESS },
  { ".TAP.Z", CompressMethod::TAPE_DIR_COMPRESS_COMPRESS },
  { ".tap.Z", CompressMethod::TAPE_DIR_COMPRESS_COMPRESS },
  { ".TAP.z", CompressMethod::TAPE_DIR_GZIP_COMPRESS },
  { ".tap.z", CompressMethod::TAPE_DIR_GZIP_COMPRESS },
  { ".tap.gz", CompressMethod::TAPE_DIR_GZIP_COMPRESS },
  { ".tap.bz2", CompressMethod::TAPE_DIR_BZIP_COMPRESS },
  { ".TAP.BZ2", CompressMethod::TAPE_DIR_BZIP_COMPRESS },
  { ".F", CompressMethod::  FREEZE_COMPRESS },
  { ".TFR", CompressMethod::FREEZE_COMPRESS },
  { ".Faa", CompressMethod::MULTIPLE_FREEZE_COMPRESS },
  { ".Z", CompressMethod::  COMPRESS_COMPRESS },
  { ".TZ", CompressMethod:: COMPRESS_COMPRESS },
  { ".TZR", CompressMethod::COMPRESS_COMPRESS },
  { ".Xaa", CompressMethod::MULTIPLE_COMPRESS_COMPRESS },
  { ".bz2", CompressMethod::BZIP_COMPRESS },
  { ".z", CompressMethod::  GZIP_COMPRESS },
  { ".gz", CompressMethod:: GZIP_COMPRESS },
  { ".tz", CompressMethod:: GZIP_COMPRESS },
  { ".tzr", CompressMethod::GZIP_COMPRESS },
  { ".tgz", CompressMethod::GZIP_COMPRESS },
  { ".TGZ", CompressMethod::GZIP_COMPRESS },
  { ".taz", CompressMethod::GZIP_COMPRESS },
  { ".TAZ", CompressMethod::GZIP_COMPRESS },
  { ".tpz", CompressMethod::GZIP_COMPRESS },
  { ".TPZ", CompressMethod::GZIP_COMPRESS },
  { ".xaa", CompressMethod::MULTIPLE_GZIP_COMPRESS },
  { ".zoo", CompressMethod::ZOO_COMPRESS },
  { ".ZOO", CompressMethod::ZOO_COMPRESS },
  { ".lzh", CompressMethod::LHA_COMPRESS },
  { ".LZH", CompressMethod::LHA_COMPRESS },
  { ".arc", CompressMethod::ARC_COMPRESS },
  { ".ARC", CompressMethod::ARC_COMPRESS },
  { ".rar", CompressMethod::RAR_COMPRESS },
  { ".RAR", CompressMethod::RAR_COMPRESS },
  { ".jar", CompressMethod::ZIP_COMPRESS },
  { ".zip", CompressMethod::ZIP_COMPRESS },
  { ".ZIP", CompressMethod::ZIP_COMPRESS },
  { ".JAR", CompressMethod::ZIP_COMPRESS },
  { ".rpm", CompressMethod::RPM_COMPRESS },
  { ".RPM", CompressMethod::RPM_COMPRESS },
  { ".spm", CompressMethod::RPM_COMPRESS },
  { ".SPM", CompressMethod::RPM_COMPRESS }
};

char *GetPath(const DirEntry *dir_entry, char *buffer)
{
  char     help_buffer[PATH_LENGTH + 1];

  *buffer = '\0';

  for (auto de_ptr = dir_entry; de_ptr; de_ptr = de_ptr->up_tree)
  {
    *help_buffer = '\0';
    if( de_ptr->up_tree ) (void) strcat( help_buffer, FILE_SEPARATOR_STRING );
    if( strcmp( de_ptr->name, FILE_SEPARATOR_STRING ) )
      (void) strcat( help_buffer, de_ptr->name );
    (void) strcat( help_buffer, buffer );
    (void) strcpy( buffer, help_buffer );
  }

  /* if( *buffer == '\0' ) (void) strcpy( buffer, FILE_SEPARATOR_STRING ); */

  return( buffer );
}





char *GetFileNamePath(FileEntry *file_entry, char *buffer)
{
  (void) GetPath( file_entry->dir_entry, buffer );
  if( *buffer && strcmp( buffer, FILE_SEPARATOR_STRING ) )
    (void) strcat( buffer, FILE_SEPARATOR_STRING );
  return( strcat( buffer, file_entry->name ) );
}



char *GetRealFileNamePath(FileEntry *file_entry, char *buffer)
{
  char *sym_name;

  if( mode == DISK_MODE || mode == USER_MODE )
    return( GetFileNamePath( file_entry, buffer ) );

  if( S_ISLNK( file_entry->stat_struct.st_mode ) )
  {
    sym_name = &file_entry->name[ strlen( file_entry->name ) + 1 ];
    if( *sym_name == FILE_SEPARATOR_CHAR )
      return( strcpy( buffer, sym_name ) );
  }

  (void) GetPath( file_entry->dir_entry, buffer );
  if( *buffer && strcmp( buffer, FILE_SEPARATOR_STRING ) )
    (void) strcat( buffer, FILE_SEPARATOR_STRING );
  if( S_ISLNK( file_entry->stat_struct.st_mode ) )
    return( strcat( buffer, &file_entry->name[ strlen( file_entry->name ) + 1 ] ) );
  else
    return( strcat( buffer, file_entry->name ) );
}






int GetDirEntry(DirEntry *tree,
                DirEntry *current_dir_entry,
                char *dir_path,
                DirEntry **dir_entry,
                char *to_path
	       )
{
  char dest_path[PATH_LENGTH+1];
  std::string current_path;
  char help_path[PATH_LENGTH+1];
  char *token, *old;
  DirEntry *de_ptr, *sde_ptr;
  int n;

  *dir_entry = NULL;
  *to_path   = '\0';

   strcpy(to_path, dir_path);

  if (const auto cwd = Getcwd())
  {
    current_path = *cwd;
  } else {
    ErrorPrintf("Getcwd failed*%s", std::strerror(errno));

    return -1;
  }

  if( *dir_path != FILE_SEPARATOR_CHAR )
  {
    if( chdir( GetPath( current_dir_entry, help_path ) ) )
    {
      ERROR_MSG( "Chdir Failed" );
      return( -1 );
    }
  }

  if( chdir( dir_path ) )
  {
#ifdef DEBUG
    (void) sprintf( message, "Invalid Path!*\"%s\"", dir_path );
    MESSAGE( message );
#endif
    return( -3 );
  }

  if (*dir_path != FILE_SEPARATOR_CHAR)
  {
    if (const auto cwd = Getcwd())
    {
      std::strcpy(dest_path, cwd->c_str());
    }
    std::strcpy(to_path, dest_path);
  } else {
    std::strcpy(dest_path, dir_path);
  }

  if (chdir(current_path.c_str()))
  {
    ERROR_MSG("chdir() failed; Can't resume");

    return -1;
  }

  n = strlen( tree->name );
  if( !strcmp(tree->name, FILE_SEPARATOR_STRING) ||
      (!strncmp( tree->name, dest_path, n )     &&
        ( dest_path[n] == FILE_SEPARATOR_CHAR || dest_path[n] == '\0' ) ) )
  {
    /* Pfad befindet sich im (Sub)-Tree */
    /*----------------------------------*/

    de_ptr = tree;
    token = Strtok_r( &dest_path[n], FILE_SEPARATOR_STRING, &old );
    while( token )
    {
      for( sde_ptr = de_ptr->sub_tree; sde_ptr; sde_ptr = sde_ptr->next )
      {
        if( !strcmp( sde_ptr->name, token ) )
	{
	  /* Subtree gefunden */
	  /*------------------*/

	  de_ptr = sde_ptr;
	  break;
	}
      }
      if( sde_ptr == NULL )
      {
#ifdef DEBUG
	(void) sprintf( message, "Can't find directory; token=%s", token );
	ERROR_MSG( message );
#endif
	return( -3 );
      }
      token = Strtok_r( NULL, FILE_SEPARATOR_STRING, &old );
    }
    *dir_entry = de_ptr;
  }
  return( 0 );
}




int GetFileEntry(DirEntry *de_ptr, char *file_name, FileEntry **file_entry)
{
  FileEntry *fe_ptr;

  *file_entry = NULL;

  for( fe_ptr = de_ptr->file; fe_ptr; fe_ptr = fe_ptr->next )
  {
    if( !strcmp( fe_ptr->name, file_name ) )
    {
      /* Eintrag gefunden */
      /*------------------*/

      *file_entry = fe_ptr;
      break;
    }
  }
  return( 0 );
}





char *GetAttributes(unsigned short modus, char *buffer)
{
  char *save_buffer = buffer;

       if( S_ISREG( modus ) )  *buffer++ = '-';
  else if( S_ISDIR( modus ) )  *buffer++ = 'd';
  else if( S_ISCHR( modus ) )  *buffer++ = 'c';
  else if( S_ISBLK( modus ) )  *buffer++ = 'b';
  else if( S_ISFIFO( modus ) ) *buffer++ = 'p';
  else if( S_ISLNK( modus ) )  *buffer++ = 'l';
  else if( S_ISSOCK( modus ) ) *buffer++ = 's';  /* ??? */
  else                         *buffer++ = '?';  /* unknown */

  if( modus & S_IRUSR ) *buffer++ = 'r';
  else *buffer++ = '-';

  if( modus & S_IWUSR ) *buffer++ = 'w';
  else *buffer++ = '-';

  if( modus & S_IXUSR ) *buffer++ = 'x';
  else *buffer++ = '-';

  if( modus & S_ISUID ) *(buffer - 1) = 's';


  if( modus & S_IRGRP ) *buffer++ = 'r';
  else *buffer++ = '-';

  if( modus & S_IWGRP ) *buffer++ = 'w';
  else *buffer++ = '-';

  if( modus & S_IXGRP ) *buffer++ = 'x';
  else *buffer++ = '-';

  if( modus & S_ISGID ) *(buffer - 1) = 's';


  if( modus & S_IROTH ) *buffer++ = 'r';
  else *buffer++ = '-';

  if( modus & S_IWOTH ) *buffer++ = 'w';
  else *buffer++ = '-';

  if( modus & S_IXOTH ) *buffer++ = 'x';
  else *buffer++ = '-';

  *buffer = '\0';

  return( save_buffer );
}



char *CTime(time_t f_time, char *buffer)
{
  char   *cptr;
  time_t now;

  if( (now = time( NULL )) == -1 )
  {
    ERROR_MSG( "time() failed" );
    exit( 1 );
  }

  cptr = ctime( &f_time );
  (void) strncpy( buffer, cptr+4, 12 );
  buffer[12] = '\0';

  if( (now - f_time) > 31536000L )
  {
    /* Differenz groesser als 1 Jahr */
    /*-------------------------------*/

    (void) strncpy( &buffer[7], cptr + 19, 5 );

  }

  return( buffer );
}




void PrintSpecialString(WINDOW *win, int y, int x, const char *str, int color)
{
  int ch;

  if(x < 0 || y < 0) {
     /* screen too small */
    return;
  }

  wmove( win, y, x);

  for( ; *str; str++ )
  {
    if ( (!iscntrl(*str)) || (!isspace(*str)) || (*str==' ') )
    switch( *str )
    {
      case '1': ch = ACS_ULCORNER; break;
      case '2': ch = ACS_URCORNER; break;
      case '3': ch = ACS_LLCORNER; break;
      case '4': ch = ACS_LRCORNER; break;
      case '5': ch = ACS_TTEE;     break;
      case '6': ch = ACS_LTEE;     break;
      case '7': ch = ACS_RTEE;     break;
      case '8': ch = ACS_BTEE;     break;
      case '9': ch = ACS_LARROW;   break;
      case '|': ch = ACS_VLINE;    break;
      case '-': ch = ACS_HLINE;    break;
      default:  ch = PRINT(*str);
    }
    else
            ch = ACS_BLOCK;

#ifdef COLOR_SUPPORT
    wattrset( win, COLOR_PAIR(color) | A_BOLD );
#endif /* COLOR_SUPPORT */
    waddch( win, ch );
#ifdef COLOR_SUPPORT
    wattrset( win, 0 );
#endif /* COLOR_SUPPORT */
  }
}


void Print(WINDOW *win, int y, int x, const char *str, int color)
{
 int ch;

  if(x < 0 || y < 0) {
     /* screen too small */
    return;
  }
  wmove(win, y, x);
  for( ; *str; str++ )
  {
    ch = PRINT((int) *str);

#ifdef COLOR_SUPPORT
    wattrset( win, COLOR_PAIR(color) | A_BOLD);
#endif /* COLOR_SUPPORT */
    waddch(win, ch );
#ifdef COLOR_SUPPORT
    wattrset( win, 0);
#endif /* COLOR_SUPPORT */
  }
}


void PrintOptions(WINDOW *win, int y, int x, const char *str)
{
  int ch;
  int color, hi_color, lo_color;

  if(x < 0 || y < 0) {
     /* screen too small */
    return;
  }

#ifdef COLOR_SUPPORT
     lo_color = MENU_COLOR;
     hi_color = HIMENUS_COLOR;
#else
     lo_color = A_NORMAL;
     hi_color = A_BOLD;
#endif

  color = lo_color;

  for( ; *str; str++ )
  {
    ch = (int) *str;

    switch( *str ) {
        case '(': color = hi_color;  continue;
	case ')': color = lo_color;  continue;

#ifdef COLOR_SUPPORT
	case ']': color = lo_color;  continue;
	case '[': color = hi_color;  continue;
#else
	case ']':
	case '[': /* ignore */ continue;
#endif

        case '1': ch = ACS_ULCORNER; break;
        case '2': ch = ACS_URCORNER; break;
        case '3': ch = ACS_LLCORNER; break;
        case '4': ch = ACS_LRCORNER; break;
        case '5': ch = ACS_TTEE;     break;
        case '6': ch = ACS_LTEE;     break;
        case '7': ch = ACS_RTEE;     break;
        case '8': ch = ACS_BTEE;     break;
        case '9': ch = ACS_LARROW;   break;
        case '|': ch = ACS_VLINE;    break;
        case '-': ch = ACS_HLINE;    break;
        default:  ch = PRINT(*str);
     }

#ifdef COLOR_SUPPORT
    wattrset( win, COLOR_PAIR(color) | A_BOLD);
#else
    wattrset( win, color);
#endif
     mvwaddch( win, y, x++, ch );
     wattrset( win, 0 );
   }
}


void PrintMenuOptions(WINDOW *win,int y, int x, char *str, int ncolor, int hcolor)
{
  int ch;
  int color, hi_color, lo_color;
  char *sbuf, buf[2];

  sbuf = (char *)malloc(strlen(str)+1);
  sbuf[0] = '\0';
  buf[1] = '\0';

  if(x < 0 || y < 0) {
     /* screen too small */
    return;
  }

#ifdef COLOR_SUPPORT
     lo_color = MENU_COLOR;
     hi_color = HIMENUS_COLOR;
#else
     lo_color = A_NORMAL;
     hi_color = A_REVERSE;
#endif

  color = lo_color;
  wmove(win, y, x);

  for( ; *str; str++ )
  {
    ch = (int) *str;

    switch( ch ) {
        case '(': color = hi_color;
#ifdef COLOR_SUPPORT
                  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
#else
                  WAttrAddStr( win, color, sbuf);
#endif
		  strcpy(sbuf, "");
	          continue;

	case ')': color = lo_color;
#ifdef COLOR_SUPPORT
                  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
#else
                  WAttrAddStr( win, color, sbuf);
#endif
		  strcpy(sbuf, "");
	          continue;

#ifdef COLOR_SUPPORT
	case ']': color = lo_color;
                  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
		  strcpy(sbuf, "");
	          continue;
	case '[': color = hi_color;
                  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
		  strcpy(sbuf, "");
	          continue;
#else
	case ']':
	case '[': /* ignore */ continue;
#endif
        default : buf[0] = PRINT(*str);
		  strcat(sbuf, buf);
    }
  }

#ifdef COLOR_SUPPORT
  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
#else
  WAttrAddStr( win, color, sbuf);
#endif
  free(sbuf);
}


/*****************************************************************************
 *                              FormFilename                                 *
 *****************************************************************************/

char *FormFilename(char *dest, char *src, unsigned int max_len)
{
  int i;
  int begin;
  unsigned int l;

  l = strlen(src);
  begin = 0;

  if( l <= max_len )
    return( strcpy( dest, src ) );
  else
  {
    for(i=0; i < (int) max_len - 4; i++)
      if( src[l - i] == FILE_SEPARATOR_CHAR || src[l - i] == '\\' )
        begin = l - i;
    (void) strcpy( dest, "/..." );
    return( strcat(dest, &src[begin] ) );
  }
}


/*****************************************************************************
 *                              CutFilename                                  *
 *****************************************************************************/

char *CutFilename(char *dest, char *src, unsigned int max_len)
{
  unsigned int l;
  char *tmp;

  l = StrVisualLength(src);

  if( l <= max_len )
    return( strcpy( dest, src ) );
  else
  {
    tmp = StrLeft(src, max_len - 3);
    sprintf(dest, "%s...", tmp);
    free(tmp);
    return( dest );
  }
}

/*****************************************************************************
 *                              CutPathname                                  *
 *****************************************************************************/
char* CutPathname(char* dest, const std::string& src, std::size_t max_len)
{
  const auto l = src.length();

  if (l <= max_len)
  {
    return std::strcpy(dest, src.c_str());
  }
  std::strcpy(dest, "...");
  std::strncat(dest, src.substr(l - max_len + 3).c_str(), max_len - 3);

  return dest;
}

/*****************************************************************************
 *                                  Fnsplit                                  *
 *****************************************************************************/

/* Aufsplitten des Dateinamens in die einzelnen Komponenten */

void Fnsplit(char *path, char *dir, char *name)
{
  int  i;
  char *name_begin;
  char *trunc_name;

  while( *path == ' ' || *path == '\t' ) path++;

  while( strchr(path, FILE_SEPARATOR_CHAR ) || strchr(path, '\\') )
    *(dir++) = *(path++);

  *dir = '\0';

  name_begin = path;
  trunc_name = name;

  for(i=0; i < PATH_LENGTH && *path; i++ )
    *(name++) = *(path++);

  *name = '\0';

  if (i == PATH_LENGTH && *path)
  {
    WarningPrintf(
      "filename too long:*%s*truncating to*%s",
		  name_begin,
      trunc_name
    );
  }
}



int BuildFilename( char *in_filename,
		   char *pattern,
		   char *out_filename
		 )
{
  char *cptr;
  int  result = 0;


  for( ; *pattern; pattern++ )
  {
    if( *pattern == '*' )
    {
      cptr = in_filename;
      for( ; (*out_filename = *cptr); out_filename++, cptr++ );
    }
    else
    {
      *out_filename++ = *pattern;
    }
  }

  *out_filename = '\0';

  return( result );
}

std::optional<CompressMethod> GetFileMethod(const std::string& filename)
{
  const auto length = filename.length();

  for (const auto& entry : file_extensions)
  {
    const auto& extension = entry.first;
    const auto extension_length = extension.length();

    if (length >= extension_length &&
        !filename.substr(length - extension_length).compare(extension))
    {
      return entry.second;
    }
  }

  return std::nullopt;
}

void NormPath( const char *in_path, char *out_path )
{
  const char* s;
  char* d;
  char *old, *opath;
  int  level;
  auto in_path_dup = MallocOrAbort<char>(std::strlen(in_path) + 1);

  level = 0;
  opath = out_path;

  if( *in_path == FILE_SEPARATOR_CHAR ) {
    s = in_path + 1;
    *opath++ = FILE_SEPARATOR_CHAR;
  } else {
    s = in_path;
  }

  for( d=in_path_dup; *s; d++ ) {
    *d = *s++;
    while( *d == FILE_SEPARATOR_CHAR && *s == FILE_SEPARATOR_CHAR )
      s++;
  }
  *d = '\0';

  d = opath;
  s = Strtok_r( in_path_dup, FILE_SEPARATOR_STRING, &old );
  while( s ) {
    if( strcmp( s, "." ) ) {		/* skip "." */
      if( !strcmp( s, ".." ) ) {	/* optimize ".." */
        if( level > 0 ) {
          if( level == 1 ) {
	    d = out_path;
	  } else {
	    for( d -= 2; *d != FILE_SEPARATOR_CHAR; d-- )
	      ;
	    d++;
	  }
        } else {
          /* level <= 0 */
	  *d++ = '.';
	  *d++ = '.';
	  *d++ = FILE_SEPARATOR_CHAR;
        }
        level--;
      } else {				/* add component */
        strcpy( d, s );
        d += strlen( s );
        *d++ = FILE_SEPARATOR_CHAR;
        level++;
      }
    }
    s = Strtok_r( NULL, FILE_SEPARATOR_STRING, &old );
  }
  if( level != 0 )
    d--;
  *d = '\0';
  if( *out_path == '\0' )
    strcpy(out_path, "." );

  free( in_path_dup );
}



/* reentrantes strtok */
char *Strtok_r( char *str, const char *delim, char **old )
{
  char *result;
  int  l, m;

  if( str == NULL )
    str = *old;

  if( str == NULL )
    return( NULL );

  l = strlen( str );
  if( ( result = strtok( str, delim ) ) != NULL ) {
    m = strlen( result );
    if( (m + 1) >= l)
      *old = NULL;
    else
      *old = result + m + 1;

  } else
    *old = NULL;

  return( result );
}




void GetMaxYX(WINDOW *win, int *height, int *width)
{
  if( win == dir_window )
  {
    *height = MAXIMUM(DIR_WINDOW_HEIGHT, 1);
    *width  = MAXIMUM(DIR_WINDOW_WIDTH, 1);
  }
  else if( win == small_file_window )
  {
    *height = MAXIMUM(FILE_WINDOW_1_HEIGHT, 1);
    *width  = MAXIMUM(FILE_WINDOW_1_WIDTH, 1);
  }
  else if( win == big_file_window )
  {
    *height = MAXIMUM(FILE_WINDOW_2_HEIGHT, 1);
    *width  = MAXIMUM(FILE_WINDOW_2_WIDTH, 1);
  }
  else if( win == f2_window )
  {
    *height = MAXIMUM(F2_WINDOW_HEIGHT - 1, 1); /* fake for separator line */
    *width  = MAXIMUM(F2_WINDOW_WIDTH, 1);
  }
  else if( win == history_window )
  {
    *height = MAXIMUM(HISTORY_WINDOW_HEIGHT, 1);
    *width  = MAXIMUM(HISTORY_WINDOW_WIDTH, 1);
  }
  else
  {
    ERROR_MSG( "Unknown Window-ID*ABORT" );
    exit( 1 );
  }
}


int Strrcmp(char *s1, char* s2)/*compares in reverse order 2 strings*/
{
   int aux;
   int l1 = strlen(s1);
   int l2 = strlen(s2);

   for (aux = 0; aux <= l2; aux++)
   {
      if ((l1 - aux) < 0)
	return(-1);
      if (s1[l1 - aux] > s2[l2 - aux])
         return(1);
      else if (s1[l1 - aux] < s2[l2 - aux])
         return(-1);
   }
   return(0);
}

char* Strdup(const std::string& src)
{
  const auto result = static_cast<char*>(std::malloc(src.length() + 1));

  std::strncpy(result, src.c_str(), src.length());

  return result;
}

char* Strndup(const std::string& src, const std::size_t len)
{
  const auto l = std::min(src.length(), len);
  const auto result = static_cast<char*>(std::malloc(l + 1));

  std::memcpy(
    static_cast<void*>(result),
    static_cast<const void*>(src.c_str()),
    l
  );
  result[l] = 0;

  return result;
}

std::optional<std::string> GetExtension(const std::string& filename)
{
  const auto pos = filename.rfind('.');

  if (pos == std::string::npos || pos == 0)
  {
    return std::nullopt;
  }

  return filename.substr(pos + 1);
}

std::string ShellEscape(const std::string& src)
{
  static const std::string esc_chars = "\\!\"#$&'()*;<>?[]^`{|}~";
  std::string result;

  for (const auto& c : src)
  {
    switch (c)
    {
      case '\t':
        result += "\\t";
        break;

      case '\n':
        result += "\\n";
        break;

      case '\r':
        result += "\\r";
        break;

      default:
        if (esc_chars.find(c) != std::string::npos)
        {
          result += '\\';
        }
        result += c;
    }
  }

  return result;
}

int BuildUserFileEntry(FileEntry *fe_ptr,
			int max_filename_len, int max_linkname_len,
			const char *tmpl, int linelen, char *line)
{
  char attributes[11];
  char modify_time[13];
  char change_time[13];
  char access_time[13];
  char format1[60];
  char format2[60];
  int  n;
  char owner[OWNER_NAME_MAX + 1];
  char group[GROUP_NAME_MAX + 1];
  const char* sym_link_name = nullptr;
  const char* sptr;
  char* dptr;
  char tag;
  char buffer[4096]; /* enough??? */


  if( fe_ptr && S_ISLNK( fe_ptr->stat_struct.st_mode ) )
    sym_link_name = &fe_ptr->name[strlen(fe_ptr->name)+1];
  else
    sym_link_name = "";


  tag = (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ';
  (void) GetAttributes( fe_ptr->stat_struct.st_mode, attributes);

  (void) CTime( fe_ptr->stat_struct.st_mtime, modify_time );
  (void) CTime( fe_ptr->stat_struct.st_ctime, change_time );
  (void) CTime( fe_ptr->stat_struct.st_atime, access_time );

  if (const auto owner_name_ptr = GetPasswdName(fe_ptr->stat_struct.st_uid))
  {
    std::strncpy(owner, owner_name_ptr->c_str(), sizeof(owner));
  } else {
    std::snprintf(owner, sizeof(owner), "%d", fe_ptr->stat_struct.st_uid);
  }
  if (const auto group_name_ptr = GetPasswdName(fe_ptr->stat_struct.st_gid))
  {
    std::strncpy(group, group_name_ptr->c_str(), sizeof(group));
  } else {
    std::snprintf(group, sizeof(group), "%d", fe_ptr->stat_struct.st_gid);
  }

  sprintf(format1, "%%-%ds", max_filename_len);
  sprintf(format2, "%%-%ds", max_linkname_len);

  for(sptr=tmpl, dptr=buffer; *sptr; ) {

    if(*sptr == '%') {
      sptr++;
      if(!strncmp(sptr, TAGSYMBOL_VIEWNAME, 3)) {
        *dptr = tag; n=1;
      } else if(!strncmp(sptr, FILENAME_VIEWNAME, 3)) {
        n = sprintf(dptr, format1, fe_ptr->name);
      } else if(!strncmp(sptr, ATTRIBUTE_VIEWNAME, 3)) {
        n = sprintf(dptr, "%10s", attributes);
      } else if(!strncmp(sptr, LINKCOUNT_VIEWNAME, 3)) {
        n = sprintf(dptr, "%3d", (int)fe_ptr->stat_struct.st_nlink);
      } else if(!strncmp(sptr, FILESIZE_VIEWNAME, 3)) {
        n = sprintf(dptr, "%7lld", (long long) fe_ptr->stat_struct.st_size);
      } else if(!strncmp(sptr, MODTIME_VIEWNAME, 3)) {
        n = sprintf(dptr, "%12s", modify_time);
      } else if(!strncmp(sptr, SYMLINK_VIEWNAME, 3)) {
        n = sprintf(dptr, format2, sym_link_name);
      } else if(!strncmp(sptr, UID_VIEWNAME, 3)) {
        n = sprintf(dptr, "%-8s", owner);
      } else if(!strncmp(sptr, GID_VIEWNAME, 3)) {
        n = sprintf(dptr, "%-8s", group);
      } else if(!strncmp(sptr, INODE_VIEWNAME, 3)) {
        n = sprintf(dptr, "%7lld", (long long)fe_ptr->stat_struct.st_ino);
      } else if(!strncmp(sptr, ACCTIME_VIEWNAME, 3)) {
        n = sprintf(dptr, "%12s", access_time);
      } else if(!strncmp(sptr, CHGTIME_VIEWNAME, 3)) {
        n = sprintf(dptr, "%12s", change_time);
      } else {
	n = -1;
      }
      if(n == -1) {
        *dptr++ = '%';
	} else {
        dptr += n;
        if(*sptr) sptr++;
        if(*sptr) sptr++;
        if(*sptr) sptr++;
      }
    } else {
      *dptr++ = *sptr++;
    }
  }
  *dptr = '\0';
  strncpy(line, buffer, linelen);
  line[linelen - 1] = '\0';
  return(0);
}



int GetUserFileEntryLength( int max_filename_len, int max_linkname_len, const char *tmpl)
{
  int  len, n;
  const char *sptr;


  for(len=0, sptr=tmpl; *sptr; ) {

    if(*sptr == '%') {
      sptr++;
      if(!strncmp(sptr, TAGSYMBOL_VIEWNAME, 3)) {
        n=1;
      } else if(!strncmp(sptr, FILENAME_VIEWNAME, 3)) {
        n = max_filename_len;
      } else if(!strncmp(sptr, ATTRIBUTE_VIEWNAME, 3)) {
        n = 10;
      } else if(!strncmp(sptr, LINKCOUNT_VIEWNAME, 3)) {
        n = 3;
      } else if(!strncmp(sptr, FILESIZE_VIEWNAME, 3)) {
        n = 7;
      } else if(!strncmp(sptr, MODTIME_VIEWNAME, 3)) {
        n = 12;
      } else if(!strncmp(sptr, SYMLINK_VIEWNAME, 3)) {
        n = max_linkname_len;
      } else if(!strncmp(sptr, UID_VIEWNAME, 3)) {
        n = 8;
      } else if(!strncmp(sptr, GID_VIEWNAME, 3)) {
        n = 8;
      } else if(!strncmp(sptr, INODE_VIEWNAME, 3)) {
        n = 7;
      } else if(!strncmp(sptr, ACCTIME_VIEWNAME, 3)) {
        n = 12;
      } else if(!strncmp(sptr, CHGTIME_VIEWNAME, 3)) {
        n = 12;
      } else {
	n = -1;
      }
      if(n == -1) {
        len++;
	sptr++;
	} else {
        len += n;
        if(*sptr) sptr++;
        if(*sptr) sptr++;
        if(*sptr) sptr++;
      }
    } else {
      sptr++;
      len++;
    }
  }
  return(len);
}


long long AtoLL(const char *cptr)
{
  long long ll;

  sscanf(cptr, "%lld", &ll);

  return(ll);
}

std::optional<std::string> Getcwd()
{
  char buffer[PATH_MAX];

  if (!getcwd(buffer, PATH_MAX))
  {
    return std::nullopt;
  }

  return buffer;
}

std::string GetcwdOrDot()
{
  if (const auto cwd = Getcwd())
  {
    return *cwd;
  }
  Warning("Getcwd() failed*\".\" assumed");

  return ".";
}

static inline bool Stat(const std::string& path, struct stat& st)
{
#if defined(S_IFLNK) && !defined(isc386)
  return !lstat(path.c_str(), &st);
#else
  return !stat(path.c_str(), &st);
#endif
}

void StatOrAbort(const std::string& path, struct stat& st)
{
  if (!Stat(path, st))
  {
    ERROR_MSG("stat() failed*ABORT");
    std::exit(EXIT_FAILURE);
  }
}
