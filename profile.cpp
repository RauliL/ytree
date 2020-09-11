/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/profile.c,v 1.10 2014/12/26 09:53:11 werner Exp $
 *
 * Profile support
 *
 ***************************************************************************/
#include "ytree.h"

#include <vector>

enum class Section
{
  NO_SECTION = 0,
  GLOBAL_SECTION = 1,
  VIEWER_SECTION = 2,
  MENU_SECTION = 3,
  FILEMAP_SECTION = 4,
  FILECMD_SECTION = 5,
  DIRMAP_SECTION = 6,
  DIRCMD_SECTION = 7,
};

typedef struct
{
  const char* name;
  const char* def;
  const char* envvar;
  char *value;
} Profile;

struct Viewer
{
  const std::string ext;
  const std::string cmd;
};

typedef struct _dirmenu {
  int chkey;
  int chremap;
  char *cmd;
  struct _dirmenu *next;
} Dirmenu;

typedef struct _filemenu {
  int chkey;
  int chremap;
  char *cmd;
  struct _filemenu *next;
} Filemenu;

static std::vector<Viewer> viewer_array;
static Dirmenu dirmenu;
static Filemenu filemenu;

/* MUSS sortiert sein! */
static const Profile profile[] = {
  { "ARCEXPAND",    	DEFAULT_ARCEXPAND,     NULL,     NULL },
  { "ARCLIST",      	DEFAULT_ARCLIST,       NULL,     NULL },
  { "BUNZIP",       	DEFAULT_BUNZIP,        NULL,     NULL },
  { "CAT",          	DEFAULT_CAT,           NULL,     NULL },
  { "DIR1",         	DEFAULT_DIR1,          NULL,     NULL },
  { "DIR2",         	DEFAULT_DIR2,          NULL,     NULL },
  { "EDITOR",       	DEFAULT_EDITOR,        "EDITOR", NULL },
  { "FILE1",        	DEFAULT_FILE1,         NULL,     NULL },
  { "FILE2",        	DEFAULT_FILE2,         NULL,     NULL },
  { "FILEMODE",     	DEFAULT_FILEMODE,      NULL,     NULL },
  { "GNUUNZIP",     	DEFAULT_GNUUNZIP,      NULL,     NULL },
  { "HEXDUMP",      	DEFAULT_HEXDUMP,       NULL,     NULL },
  { "HEXEDITOFFSET",	DEFAULT_HEXEDITOFFSET, NULL,     NULL },
  { "INITIALDIR",   	DEFAULT_INITIALDIR,    NULL,     NULL },
  { "LHAEXPAND",    	DEFAULT_LHAEXPAND,     NULL,     NULL },
  { "LHALIST",      	DEFAULT_LHALIST,       NULL,     NULL },
  { "LISTJUMPSEARCH",   DEFAULT_LISTJUMPSEARCH,NULL,     NULL },
  { "MANROFF",      	DEFAULT_MANROFF,       NULL,     NULL },
  { "MELT",         	DEFAULT_MELT,          NULL,     NULL },
  { "NOSMALLWINDOW",	DEFAULT_NOSMALLWINDOW, NULL,     NULL },
  { "NUMBERSEP",    	DEFAULT_NUMBERSEP,     NULL,     NULL },
  { "PAGER",        	DEFAULT_PAGER,        "PAGER",  NULL },
  { "RAREXPAND",    	DEFAULT_RAREXPAND,     NULL,     NULL },
  { "RARLIST",      	DEFAULT_RARLIST,       NULL,     NULL },
  { "RPMEXPAND",    	DEFAULT_RPMEXPAND,     NULL,     NULL },
  { "RPMLIST",      	DEFAULT_RPMLIST,       NULL,     NULL },
  { "SEARCHCOMMAND",	DEFAULT_SEARCHCOMMAND, NULL,     NULL },
  { "TAPEDEV",      	DEFAULT_TAPEDEV,       "TAPE",   NULL },
  { "TAREXPAND",    	DEFAULT_TAREXPAND,     NULL,     NULL },
  { "TARLIST",      	DEFAULT_TARLIST,       NULL,     NULL },
  { "TREEDEPTH",    	DEFAULT_TREEDEPTH,     NULL,     NULL },
  { "UNCOMPRESS",   	DEFAULT_UNCOMPRESS,    NULL,     NULL },
  { "USERVIEW",     	"",                    NULL,     NULL },
  { "ZIPEXPAND",    	DEFAULT_ZIPEXPAND,     NULL,     NULL },
  { "ZIPLIST",      	DEFAULT_ZIPLIST,       NULL,     NULL },
  { "ZOOEXPAND",    	DEFAULT_ZOOEXPAND,     NULL,     NULL },
  { "ZOOLIST",      	DEFAULT_ZOOLIST,       NULL,     NULL }
};

#define PROFILE_ENTRIES (sizeof(profile) / sizeof(profile[0]))


static int Compare(const void *s1, const void *s2);

static int ChCode(const char *s);

int ReadProfile( char *filename )
{
  int  l, result = -1;
  char buffer[1024], *n, *old;
  unsigned char *name, *value, *cptr;
  Section section = Section::NO_SECTION;
  Profile *p, key;
  Filemenu *m, *new_m;
  Dirmenu *d, *new_d;
  FILE *f;

  m = &filemenu;
  d = &dirmenu;

  m->next = NULL;
  d->next = NULL;

  if( ( f = fopen( filename, "r" ) ) == NULL ) {
    ESCAPE;
  }

  while( fgets( buffer, sizeof( buffer ), f ) ) {
    if(*buffer == '#')
      continue;
    l = strlen( buffer );
    if( l > 2 ) {
      buffer[l-1] = '\0';
      /* trim whitspace */
      for( name = (unsigned char *) buffer; isspace(*name); name++ )
        ;
      for(cptr=name; !isspace(*cptr) && *cptr != '='; cptr++ )
        ;
      if(*cptr != '=')
        *cptr = '\0';
      if(*name == '[') {
        /* section */
	if( !strcmp((const char *) name, "[GLOBAL]") )
	  section = Section::GLOBAL_SECTION;
	else if( !strcmp((const char *) name, "[VIEWER]") )
	  section = Section::VIEWER_SECTION;
	else if( !strcmp((const char *) name, "[MENU]") )
	  section = Section::MENU_SECTION;
	else if( !strcmp((const char *) name, "[FILEMAP]") )
	  section = Section::FILEMAP_SECTION;
	else if( !strcmp((const char *) name, "[FILECMD]") )
	  section = Section::FILECMD_SECTION;
	else if( !strcmp((const char *) name, "[DIRMAP]") )
	  section = Section::DIRMAP_SECTION;
	else if( !strcmp((const char *) name, "[DIRCMD]") )
	  section = Section::DIRCMD_SECTION;
	else
	  section = Section::NO_SECTION;

	continue;
      }


      if( section == Section::GLOBAL_SECTION ) {
        value = (unsigned char *) strchr( buffer, '=' );
        if( *name && value ) {
          *value++ = '\0';
          key.name = (char *) name;
          if(( p = static_cast<Profile*>(std::bsearch(&key, profile, PROFILE_ENTRIES, sizeof(*p), Compare)))) {
	    p->value = Strdup( (const char *) value );
          }
        }
      } else if( section == Section::MENU_SECTION ) {
        value = (unsigned char *) strchr( buffer, '=' );
        if( *name && value ) {
          *value++ = '\0';
          if (!strcmp((const char *) name, "DIR1") ||
              !strcmp((const char *) name, "DIR2") ||
              !strcmp((const char *) name, "FILE1") ||
              !strcmp((const char *) name, "FILE2") ) {
            key.name = (char *) name;
            if(( p = static_cast<Profile*>(std::bsearch(&key, profile, PROFILE_ENTRIES, sizeof(*p), Compare)))) {
              /* Space pad menu strings to length COLS, ignoring '(' and ')' characters */
              l = 0;
              for (cptr = value; *cptr; ++cptr) {
                if (*cptr != '(' && *cptr != ')') {
                  ++l;
                }
              }
              while (l++ < COLS - 1)
                *cptr++ = ' ';
              *cptr = '\0';
	      p->value = Strdup( (const char *) value );
            }
          }
        }
      } else if(section == Section::FILEMAP_SECTION ) {
        value = (unsigned char *) std::strchr( buffer, '=' );
        if( *name && value ) {
	  *value++ = '\0';
          /* trim whitespace */
          while(*value && std::isspace(*value))
            value++;
	  n = Strtok_r((char *) name, ",", &old);
	  /* maybe comma-separated list, eg.: k,K=x */
	  while(n) {
            /* Check for existing entry from FILECMD_SECTION */
            for(new_m = filemenu.next; new_m != NULL; new_m = new_m->next) {
              if (new_m->chkey == ChCode( n )) {
                new_m->chremap = ChCode( (const char *) value );
                if (new_m->chremap == 0)
                  new_m->chremap = -1;   /* Don't beep if user cmd defined */
                break;
              }
            }
	    if( new_m == NULL && ( new_m = static_cast<Filemenu*>(std::malloc( sizeof(*new_m) ) ) )) {
	      new_m->chkey = ChCode( n );
              new_m->chremap = ChCode( (const char *) value );
	      new_m->cmd = NULL;
	      new_m->next = NULL;
	      m->next = new_m;
	      m = new_m;
	    }
	    n = Strtok_r(NULL, ",", &old);
	  }
        }
      } else if(section == Section::FILECMD_SECTION ) {
        value = (unsigned char *) strchr( buffer, '=' );
        if( *name && value ) {
	  *value++ = '\0';
          /* trim whitespace */
          while(*value && isspace(*value))
            value++;
	  /* may not be comma-separated list */
          /* Check for existing entry from FILEMAP_SECTION */
          for (new_m = filemenu.next; new_m != NULL; new_m = new_m->next) {
            if (new_m->chkey == ChCode( (const char *) name )) {
	      new_m->cmd = Strdup( (const char *) value );
              if (new_m->chremap == 0)
                new_m->chremap = -1;   /* Don't beep if user cmd defined */
              break;
            }
          }
	  if( new_m == NULL && ( new_m = static_cast<Filemenu*>(std::malloc( sizeof(*new_m) ) ) )) {
            new_m->chkey = ChCode( (const char *) name );
	    new_m->chremap = new_m->chkey;
	    new_m->cmd = Strdup( (const char *) value );
	    new_m->next = NULL;
	    m->next = new_m;
	    m = new_m;
	  }
        }
      } else if(section == Section::DIRMAP_SECTION ) {
        value = (unsigned char *) strchr( buffer, '=' );
        if( *name && value ) {
	  *value++ = '\0';
          /* trim whitespace */
          while(*value && isspace(*value))
            value++;
	  n = Strtok_r((char *) name, ",", &old);
	  /* maybe comma-separated list, eg.: k,K=x */
	  while(n) {
            /* Check for existing entry from DIRCMD_SECTION */
            for(new_d = dirmenu.next; new_d != NULL; new_d = new_d->next) {
              if (new_d->chkey == ChCode( n )) {
                new_d->chremap = ChCode( (const char *) value );
                if (new_d->chremap == 0)
                  new_d->chremap = -1;   /* Don't beep if user cmd defined */
                break;
              }
            }
	    if( new_d == NULL && ( new_d = static_cast<Dirmenu*>(std::malloc( sizeof(*new_d) ) ) )) {
	      new_d->chkey = ChCode( n );
              new_d->chremap = ChCode( (const char *) value );
	      new_d->cmd = NULL;
	      new_d->next = NULL;
	      d->next = new_d;
	      d = new_d;
	    }
	    n = Strtok_r(NULL, ",", &old);
	  }
        }
      } else if(section == Section::DIRCMD_SECTION ) {
        value = (unsigned char *) strchr( buffer, '=' );
        if( *name && value ) {
	  *value++ = '\0';
          /* trim whitespace */
          while(*value && isspace(*value))
            value++;
	  /* may not be comma-separated list */
          /* Check for existing entry from DIRMAP_SECTION */
          for(new_d = dirmenu.next; new_d != NULL; new_d = new_d->next) {
            if (new_d->chkey == ChCode( (const char *) name )) {
	      new_d->cmd = Strdup( (const char *) value );
              if (new_d->chremap == 0)
                new_d->chremap = -1;   /* Don't beep if user cmd defined */
              break;
            }
          }
	  if ( new_d == NULL && ( new_d = static_cast<Dirmenu*>(std::malloc( sizeof(*new_d) ) ) )) {
            new_d->chkey = ChCode( (const char *) name );
	    new_d->chremap = new_d->chkey;
	    new_d->cmd = Strdup( (const char *) value );
	    new_d->next = NULL;
	    d->next = new_d;
	    d = new_d;
	  }
        }
      } else if ( section == Section::VIEWER_SECTION ) {
        value = (unsigned char *) strchr( buffer, '=' );

        if( *name && value ) {

	  *value++ = '\0';
	  n = Strtok_r((char *) name, ",", &old);
	  /* maybe comma-separated list, eg.: .jpeg,.gif=xv */
	  while (n)
    {
      viewer_array.push_back({
        n,
        (const char*) value,
      });
	    n = Strtok_r(nullptr, ",", &old);
	  }
        }
      }
    }
  }
  result = 0;

FNC_XIT:

  if( f )
    fclose( f );

  return( result );
}

const char* GetProfileValue(const char* name)
{
  Profile* p;
  Profile key;
  const char* cptr;

  key.name = name;
  p = static_cast<Profile*>(std::bsearch(&key, profile, PROFILE_ENTRIES, sizeof(*p), Compare));

  if (!p)
  {
    return "";
  }
  else if (p->value)
  {
    return p->value;
  }
  else if (p->envvar && (cptr = std::getenv(p->envvar)))
  {
    return cptr;
  }

  return p->def;
}

static int ChCode(const char *s)
{
  if (*s == '^' && *(s+1) != '^')
    return((int)((*(s+1)) & 0x1F));
  else
    return((int)(*s));
}

static int Compare(const void *s1, const void *s2)
{
  return( strcmp( ((Profile *)s1)->name, ((Profile *)s2)->name ) );
}

char *GetUserFileAction(int chkey, int *pchremap)
{
  Filemenu *m;

  for(m=filemenu.next; m; m=m->next) {
    if(chkey == m->chkey) {
      if (pchremap)
        *pchremap = m->chremap;
      return(m->cmd);
    }
  }
  if (pchremap)
    *pchremap = chkey;
  return(NULL);
}

char *GetUserDirAction(int chkey, int *pchremap)
{
  Dirmenu *d;

  for(d=dirmenu.next; d; d=d->next) {
    if(chkey == d->chkey) {
      if (pchremap)
        *pchremap = d->chremap;
      return(d->cmd);
    }
  }
  if (pchremap)
    *pchremap = chkey;
  return(NULL);
}

bool IsUserActionDefined()
{
  return dirmenu.next && filemenu.next;
}

std::optional<std::string> GetExtViewer(const std::string& filename)
{
  const auto length = filename.length();

  for (const auto& viewer : viewer_array)
  {
    const auto x = viewer.ext.length();

    if (length > x && !filename.substr(length - x).compare(viewer.ext))
    {
      return viewer.ext;
    }
  }

  return std::nullopt;
}
