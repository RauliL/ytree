/***************************************************************************
 *
 * $Header: /usr/local/cvsroot/utils/ytree/match.c,v 1.18 2007/04/01 13:33:36 werner Exp $
 *
 * Behandlung reg. Ausdruecke fuer Dateinamen
 *
 ***************************************************************************/


#include "ytree.h"

#if defined( sun ) || defined( linux ) || defined( OSF1 ) ||  defined( __OpenBSD__ ) || defined(__NetBSD__) || defined( __FreeBSD__ ) || defined( __GNU__ )
#define HAS_REGEX
#endif

#ifdef linux
/* To avoid warning about re_comp / re_exec */
/* Maybe this would work on other architectures too */
#define _REGEX_RE_COMP
#endif

#if defined( hpux ) || defined ( __APPLE__ )
#define HAS_REGCOMP
#endif


#ifdef HAS_REGEX
#ifdef sun
#define INIT   		register char *sp = "";
#define GETC() 		(*sp++)
#define PEEKC()     	(*sp)
#define UNGETC(c)   	(--sp)
#define RETURN(c)   	return;
#define ERROR(c)
#include <regexp.h>
#else
#include <regex.h>
#endif /* sun */
#else
extern char *regcmp();
extern char *regex();
static char *file_spec_cmp = NULL;

#endif /* HAS_REGEX */

#if defined( HAS_REGCOMP )
#include <regex.h>
static regex_t	re;
static bool	re_flag = false;
#endif





int SetMatchSpec(char *new_spec)
{
  auto buffer = MallocOrAbort<char>(std::strlen(new_spec) * 2 + 4);
  auto b_ptr = buffer;
  char *result;
  bool meta_flag = false;

  *b_ptr++ = '^';

  for(; *new_spec; new_spec++)
  {
    if( meta_flag )
    {
      *b_ptr++ = *new_spec;
      meta_flag = false;
    }
    else if( *new_spec == '\\' ) meta_flag = true;
    else if( *new_spec == '?' ) *b_ptr++ = '.';
    else if( *new_spec == '.' )
    {
      *b_ptr++ = '\\';
      *b_ptr++ = '.';
    }
    else if( *new_spec == '*' )
    {
      *b_ptr++ = '.';
      *b_ptr++ = '*';
    }
    else
      *b_ptr++ = *new_spec;
  }

  *b_ptr++ = '$';
  *b_ptr = '\0';

#if defined ( HAS_REGEX )

  if( ( result = (char *) re_comp( buffer ) ) )
  {
    free( buffer );
    return( 1 );
  }

#else

#if defined( HAS_REGCOMP )

  if(re_flag)
  {
    regfree(&re);
    re_flag = false;
  }

  if( regcomp(&re, buffer, REG_NOSUB) )
  {
    free( buffer );
    return( 1 );
  }
  free( buffer );
  re_flag = true;

#else

#if (!(defined(WIN32) || defined(__DJGPP__)))


  if( ( result = regcmp( buffer, (char *) 0 ) ) == NULL )
  {
    free( buffer );
    return( 1 );
  }

  if( file_spec_cmp ) free( file_spec_cmp );
  free( buffer );

  file_spec_cmp = result;

#else

  /* WIN32 */

  /* z.Z. nicht unterstuetzt */

  free( buffer );
  result = 0;

#endif /* WIN32 */
#endif /* HAS_REGCOMP */
#endif /* HAS_REGEX */

  return( 0 );
}



bool Match(char *file_name)
{
#if defined ( HAS_REGEX )

  if( re_exec( file_name ) ) return( true );
  else                       return( false );

#else
#if defined( HAS_REGCOMP )

  if(re_flag == false)
    return( true );

  if( ( regexec(&re, file_name, (size_t) 0, NULL, 0 ) ) == 0 )
  {
    return( true );
  }
  else
  {
    return( false );
  }

#else

#if (!(defined(WIN32) || defined(__DJGPP__)))


  char match_part[PATH_LENGTH + 1];

  if( !file_spec_cmp )
    return( true );

  if( regex( file_spec_cmp, file_name, match_part ) == NULL )
    return( false );

  return( true );

#else

  /* WIN32 */

  /* z.Z. nicht unterstuetzt */

  return( true );

#endif /* WIN32 */
#endif /* HAS_REGCOMP */
#endif /* HAS_REGEX */
}



