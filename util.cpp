/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/util.c,v 1.38 2019/09/29 10:37:49 werner Exp $
 *
 * Diverse Hilfsfunktionen
 *
 ***************************************************************************/
#include "ytree.h"

typedef struct
{
  const char* extension;
  int  method;
} Extension2Method;

static Extension2Method file_extensions[] = FILE_EXTENSIONS;

static char* GNU_getcwd();

char *GetPath(DirEntry *dir_entry, char *buffer)
{
  DirEntry *de_ptr;
  char     help_buffer[PATH_LENGTH + 1];

  *buffer = '\0';

  for( de_ptr = dir_entry; de_ptr; de_ptr = de_ptr->up_tree )
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
  char current_path[PATH_LENGTH+1];
  char help_path[PATH_LENGTH+1];
  char *token, *old;
  DirEntry *de_ptr, *sde_ptr;
  int n;

  *dir_entry = NULL;
  *to_path   = '\0';

   strcpy(to_path, dir_path);
  if( Getcwd( current_path, sizeof( current_path ) - 2 ) == NULL )
  {
    (void) sprintf( message, "Getcwd failed*%s", strerror(errno) );
    ERROR_MSG( message );
    return( -1 );
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

  if( *dir_path != FILE_SEPARATOR_CHAR ) {
    (void) Getcwd( dest_path, sizeof( dest_path ) - 2 );
    (void) strcpy( to_path, dest_path );
  } else {
    strcpy(dest_path, dir_path);
  }


  if( chdir( current_path ) )
  {
    ERROR_MSG( "Chdir failed; Can't resume" );
    return( -1 );
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

void PrintSpecialString(
  WINDOW* win,
  int y,
  int x,
  const std::string& str,
  int color
)
{
  if (x < 0 || y < 0)
  {
    return; // Screen too small.
  }

  wmove(win, y, x);

  for (auto ch : str)
  {
    if ((!std::iscntrl(ch)) || (!std::isspace(ch)) || (ch == ' '))
    {
      switch (ch)
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
        default:  ch = PRINT(ch);
      }
    } else {
      ch = ACS_BLOCK;
    }
#ifdef COLOR_SUPPORT
    wattrset(win, COLOR_PAIR(color) | A_BOLD);
#endif /* COLOR_SUPPORT */
    waddch(win, ch);
#ifdef COLOR_SUPPORT
    wattrset(win, 0);
#endif /* COLOR_SUPPORT */
  }
}

void Print(WINDOW* win, int y, int x, const std::string& str, int color)
{
  if (x < 0 || y < 0)
  {
    return; // Screen too small.
  }
  wmove(win, y, x);
  for (auto ch : str)
  {
    ch = PRINT(ch);
#ifdef COLOR_SUPPORT
    wattrset(win, COLOR_PAIR(color) | A_BOLD);
#endif /* COLOR_SUPPORT */
    waddch(win, ch );
#ifdef COLOR_SUPPORT
    wattrset(win, 0);
#endif /* COLOR_SUPPORT */
  }
}

void PrintOptions(WINDOW* win, int y, int x, const std::string& str)
{
  int color;
  int hi_color;
  int lo_color;

  if (x < 0 || y < 0)
  {
    return; // Screen too small.
  }

#ifdef COLOR_SUPPORT
  lo_color = MENU_COLOR;
  hi_color = HIMENUS_COLOR;
#else
  lo_color = A_NORMAL;
  hi_color = A_BOLD;
#endif

  color = lo_color;

  for (auto ch : str)
  {
    switch (ch)
    {
      case '(': color = hi_color; continue;
      case ')': color = lo_color;  continue;

#ifdef COLOR_SUPPORT
	  case ']': color = lo_color;  continue;
	  case '[': color = hi_color;  continue;
#else
	  case ']':
	  case '[': continue; // Ignore.
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
      default:  ch = PRINT(ch);
    }

#ifdef COLOR_SUPPORT
    wattrset(win, COLOR_PAIR(color) | A_BOLD);
#else
    wattrset(win, color);
#endif
    mvwaddch(win, y, x++, ch);
    wattrset(win, 0);
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

char *CutPathname(char *dest, char *src, unsigned int max_len)
{
  unsigned int l;

  l = strlen(src);

  if( l <= max_len )
    return( strcpy( dest, src ) );
  else
  {
    (void) strcpy( dest, "..." );
    (void) strncat( dest, &src[l - max_len + 3], max_len - 3 );
    return( dest );
  }
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

  if( i == PATH_LENGTH && *path )
  {
    (void) sprintf( message, "filename too long:*%s*truncating to*%s",
		    name_begin, trunc_name
		  );
    WARNING( message );
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



int GetFileMethod( char *filename )
{
  int i, k, l;

  l = strlen( filename );

  for( i=0;
       i < (int)(sizeof( file_extensions ) / sizeof( file_extensions[0] ));
       i++
     )
  {
    k = strlen( file_extensions[i].extension );
    if( l >= k && !strcmp( &filename[l-k], file_extensions[i].extension ) )
      return( file_extensions[i].method );
  }

  return( NO_COMPRESS );
}



#ifdef __NeXT__

char *getcwd(char *dest, int len)
{
  static char  buffer[MAXNAMLEN];
  DIR    *dirp;
  struct direct  *dp;
  long   inode;
  char   *cp = buffer + MAXNAMLEN - 1;

  *cp = 0;

  do {
    dirp = opendir(".");
    for (dp=readdir(dirp); dp; dp=readdir(dirp)) {
      if(dp->d_namlen && !strcmp(dp->d_name,".")) {
        break;
      }
    }
    if (inode == dp->d_ino) break;
    inode = dp->d_ino;
    (void) closedir(dirp);
    dirp = opendir("..");
    for (dp=readdir(dirp); dp && (dp->d_ino != inode); dp=readdir(dirp));
    cp -= dp->d_namlen;
    (void) strncpy(cp,dp->d_name,dp->d_namlen);
    *--cp = FILE_SEPARATOR_CHAR;
    (void) closedir(dirp);
  } while(!chdir(".."));

  (void) chdir(cp+2);
  (void) strncpy(dest,cp+2,len);
}


#endif

void NormPath(const char* in_path, char* out_path)
{
  const char* s;
  char *d;
  char *old, *opath;
  int  level;
  char *in_path_dup;

  level = 0;
  opath = out_path;

  if( ( in_path_dup = static_cast<char*>(std::malloc( strlen( in_path ) + 1 ) )) == NULL ) {
    ERROR_MSG( "Malloc Failed*ABORT" );
    exit( 1 );
  }

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
char* Strtok_r(char* str, const char* delim, char** old)
{
  std::size_t length;
  char* result;

  if (!str && !(str = *old))
  {
    return nullptr;
  }
  length = std::strlen(str);
  if (!(result = std::strtok(str, delim)))
  {
    const auto m = std::strlen(result);

    if (m + 1 >= length)
    {
      *old = nullptr;
    } else {
      *old = result + m + 1;
    }
  } else {
    *old = nullptr;
  }

  return result;
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

/* NeXT does not define strdup */
char* Strdup(const char* s)
{
  char* cp = nullptr;

  if (s)
  {
    cp = static_cast<char*>(std::malloc(std::strlen(s) + 1));
    if (cp)
    {
      std::strcpy(cp, s);
    }
  }

  return cp;
}

/* Solaris does not define this */
char* Strndup(const char *s, std::size_t len)
{
  char* cp = nullptr;

  if (s)
  {
    const auto l = std::min(std::strlen(s), len);

    cp = static_cast<char*>(std::malloc(l));
    if (cp)
    {
      std::memcpy(static_cast<void*>(cp), static_cast<const void*>(s), l);
      cp[l] = 0;
    }
  }

  return cp;
}

const char* GetExtension(const char* filename)
{
  auto cptr = std::strrchr(filename, '.');

  // Filenames beginning with a dot are not an extension.
  if (!cptr || cptr == filename)
  {
    return "";
  }

  return cptr + 1;
}

void StrCp(char *dest, const char *src)
{
   static char esc_chars[] ="#*|&;()<> \t\n\r\"!$?'`~";

   while(*src)
   {
     if(strchr(esc_chars, *src))
       *dest++ = '\\';
     *dest++ = *src++;
   }
   *dest = '\0';
}




int BuildUserFileEntry(FileEntry *fe_ptr,
			int max_filename_len, int max_linkname_len,
			const char *templatez, int linelen, char *line)
{
  char attributes[11];
  char modify_time[13];
  char change_time[13];
  char access_time[13];
  char format1[60];
  char format2[60];
  int  n;
  std::string owner_name;
  std::string group_name;
  const char* sym_link_name = nullptr;
  const char* sptr;
  char *dptr;
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

  if (const auto result = GetPasswdName(fe_ptr->stat_struct.st_uid))
  {
    owner_name = *result;
  } else {
    owner_name = std::to_string(fe_ptr->stat_struct.st_uid);
  }
  if (const auto result = GetGroupName(fe_ptr->stat_struct.st_gid))
  {
    group_name = *result;
  } else {
    group_name = std::to_string(fe_ptr->stat_struct.st_gid);
  }

  sprintf(format1, "%%-%ds", max_filename_len);
  sprintf(format2, "%%-%ds", max_linkname_len);

  for(sptr=templatez, dptr=buffer; *sptr; ) {

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
#ifdef HAS_LONGLONG
        n = sprintf(dptr, "%7lld", (LONGLONG) fe_ptr->stat_struct.st_size);
#else
        n = sprintf(dptr, "%7d", fe_ptr->stat_struct.st_size);
#endif
      } else if(!strncmp(sptr, MODTIME_VIEWNAME, 3)) {
        n = sprintf(dptr, "%12s", modify_time);
      } else if(!strncmp(sptr, SYMLINK_VIEWNAME, 3)) {
        n = sprintf(dptr, format2, sym_link_name);
      } else if(!strncmp(sptr, UID_VIEWNAME, 3)) {
        n = sprintf(dptr, "%-8s", owner_name.c_str());
      } else if(!strncmp(sptr, GID_VIEWNAME, 3)) {
        n = sprintf(dptr, "%-8s", group_name.c_str());
      } else if(!strncmp(sptr, INODE_VIEWNAME, 3)) {
#ifdef HAS_LONGLONG
        n = sprintf(dptr, "%7lld", (LONGLONG)fe_ptr->stat_struct.st_ino);
#else
        n = sprintf(dptr, "%7ld", (int)fe_ptr->stat_struct.st_ino);
#endif
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
  std::strncpy(line, buffer, linelen);
  line[linelen - 1] = '\0';
  return(0);
}



int GetUserFileEntryLength( int max_filename_len, int max_linkname_len, const char *templatez)
{
  int  len, n;
  const char *sptr;


  for(len=0, sptr=templatez; *sptr; ) {

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

std::int64_t AtoLL(const char* cptr)
{
  long long int ll;

  std::sscanf(cptr, "%lld", &ll);

  return ll;
}

#ifndef HAVE_STRERROR
const char *StrError(int errnum)
{

#if !defined(__FreeBSD__) && !(defined(__GLIBC__) && __GLIBC__ >= 2)
  extern char *sys_errlist[];
  extern int sys_nerr;
#endif

  if (errnum > 0 && errnum <= sys_nerr)
    return(sys_errlist[errnum]);
  return ("Unknown system error");
}
#endif /* HAVE_STRERROR */


/*****************************************************************************
 *                              Getcwd                                       *
 *****************************************************************************/


char *Getcwd(char *buffer, unsigned int size)
{
  if(size == 0)
    return(GNU_getcwd());

  return(getcwd(buffer, size));
}

static char* GNU_getcwd()
{
  std::size_t size = 100;

  for (;;)
  {
    auto buffer = static_cast<char*>(std::malloc(size));

    if (getcwd(buffer, size) == buffer)
    {
      return buffer;
    }
    std::free(buffer);
    if (errno != ERANGE)
    {
      return nullptr;
    }
    size *= 2;
  }
}

/*****************************************************************************
 *                              CutName                                      *
 *****************************************************************************/

char *CutName(char *dest, char *src, unsigned int max_len)
{
  unsigned int l;

  l = strlen(src);

  if( l <= max_len )
    return( strcpy( dest, src ) );
  else
  {
    (void) strncpy( dest, src, max_len - 3 );
    return strcpy( &dest[max_len - 3], "..." );
  }
}



