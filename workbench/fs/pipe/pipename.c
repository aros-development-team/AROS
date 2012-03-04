/****************************************************************************
**  File:       pipename.c
**  Program:    pipe-handler - an AmigaDOS handler for named pipes
**  Version:    1.1
**  Author:     Ed Puckett      qix@mit-oz
**
**  Copyright 1987 by EpAc Software.  All Rights Reserved.
**
**  History:    05-Jan-87       Original Version (1.0)
**		07-Feb-87	Added conditional compilation for autoname.
*/
#include   <string.h>	/* For strcpy */

#include   <libraries/dos.h>
#include   <libraries/dosextens.h>
#include   <libraries/filehandler.h>
#include   <exec/exec.h>

#include   "pipelists.h"
#include   "pipename.h"
#include   "pipebuf.h"
#include   "pipecreate.h"
#include   "pipesched.h"
#include   "pipe-handler.h"



/*---------------------------------------------------------------------------
** pipename.c
** ----------
** This module contains functions related to the parsing of the pipe names.
**
** Visible Functions
** -----------------
**	int   ParsePipeName (Bname, nmp, sizep, tapnmp)
**	void  BSTRtoCstr    (BSTRp, str, maxsize)
**	void  CstrtoBSTR    (str, BSTRp, maxsize)
**	void  CstrtoFIB     (str, BSTRp, maxsize)
**	int   inrange       (x, lower, upper)
**	char  uppercase     (c)
**	char  *findchar     (str, ch)
**	void  l_strcpy      (to, from)
**	char  *strdiff      (str1, str2)
**	char  *get_autoname (newflag)     (if AUTONAME or AUTONAME_STAR is true)
**
** Macros (in pipename.h)
** ----------------------
**	isnumeral (c)
**
** Local Functions
** ---------------
**	int  ParseNum (str, nump)
*/

static int ParseNum ( char   *str, ULONG  *nump);



/*---------------------------------------------------------------------------
** ParsePipeName() parses the string "Bname" into three parts: a pipe name,
** a size specification and a tap name.  (Bname must be the byte address of a
** BSTR, i.e., a string whose first byte is its length.)  The three parts are
** separated by the character PIPE_SPEC_CHAR (defined in pipename.h).
** Assuming that PIPE_SPEC_CHAR is '/', and that '[]' are metacharacters
** which enclose optional parts, the syntax for Bname is [D:][p][/n][/[t]].
** Here, "D" represents a device name, "p" represents a pipe name,
** "n" represents a number and "t" represents a tap name.
**      ParsePipeName() returns nonzero iff "Bname" conforms to the syntax
** and the following restrictions.
**      "D:" represents a device name.  If it occurs, it is ignored.  Notice
** that tap names which contain a ":" force a device name to be specified for
** the pipe.  Otherwise, everything up to and including the ":" in the tap
** name will be ignored.
**      *nmp returns pointing to a copy of "p", even if it is empty.  Default
** pipe names are handled by calling get_autoname().  (This is done by
** OpenPipe() if *nmp returns empty.)
**      "n" must begin with a digit.  If "n" begins with "0x", it is parsed
** as a hexadecimal number.  If it begins with "0" but not "0x", it is parsed
** as an octal number.  Otherwise, it is parsed as a decimal number.  If the
** size specifier ("/t" above) is not given, *sizep is set to DEFAULT_PIPELEN.
**     If the compile-time flag CON_TAP_ONLY is set, "t" may only be a "CON:"
** file specifier, such as "CON:10/10/400/120/TapWindow".  If CON_TAP_ONLY is
** not set, string is accepted.  If "t" is empty (but the PIPE_SPEC_CHAR was
** given), then a defualt tap name is formed by appending "p" to
** DEFAULT_TAPNAME_PREFIX.  If the tap name specifier ("/[t]" above) is not
** given, *tapnmp is set to NULL.
*/

static char  default_tapname_prefix[]  =  DEFAULT_TAPNAME_PREFIX;
static char  namebuf[sizeof (default_tapname_prefix) + PIPENAMELEN];

int  ParsePipeName (Bname, nmp, sizep, tapnmp)

BYTE   *Bname;       /* reference to BSTR name sent to handler */
char   **nmp;        /* reference to pipe name pointer */
ULONG  *sizep;       /* size longword pointer */
char   **tapnmp;     /* reference to tap name pointer, returns NULL if none */

{ char  *cp;
  int   ParseNum();


  l_strcpy (namebuf, default_tapname_prefix);

  *nmp=    namebuf + (sizeof (default_tapname_prefix) - 1);
  *sizep=  DEFAULT_PIPELEN;
  *tapnmp= NULL;

  BSTRtoCstr (Bname, *nmp, PIPENAMELEN);

  if (*(cp= findchar (*nmp, ':')) == ':')
    l_strcpy (*nmp, ++cp);     /* get rid of "devname:" prefix */

  if ( *(cp= findchar (*nmp, PIPE_SPEC_CHAR)) )     /* true if not '\0' */
    { *(cp++)= '\0';     /* terminate pipe name */

      if (isnumeral (*cp))
        { if ( (! ParseNum (cp, sizep)) || (*sizep <= 0) )
            return FALSE;

          if ( *(cp= findchar (cp, PIPE_SPEC_CHAR)) == '\0' )
            return TRUE;     /* no tap name, but successful anyway */

          ++cp;     /* skip separator */
        }

      if ( *(*tapnmp= cp) == '\0' )     /* first character of tap name */
        *tapnmp= namebuf;     /* use default prefix prepended to pipe name */
#if CON_TAP_ONLY
      else
        { if ( *(strdiff ("CON:", *tapnmp)) )     /* true if not '\0' */
            return FALSE;     /* only CON: . . . allowed */
        }
#endif /* CON_TAP_ONLY */
    }

  return TRUE;
}



/*---------------------------------------------------------------------------
** BSTRtoCstr() converts the BSTR pointed to by "BSTRp" (a byte address) to
** a null-terminated string, storing the result in the locations pointed to
** by "str".  At most "maxsize" bytes will be stored.
*/

void  BSTRtoCstr (BSTRp, str, maxsize)

register BYTE  *BSTRp;
register char  *str;
unsigned       maxsize;

{
#ifdef AROS_FAST_BSTR
  strncpy(str, BSTRp, maxsize);
  str[maxsize-1] = 0;
#else
  register int   i;
  register int   limit;


  if ((limit= *(BSTRp++)) > ((int) maxsize - 1))     /* leave room for '\0' */
    limit= (int) maxsize - 1;

  for (i= 0; i < limit; ++i)
    *(str++)= *(BSTRp++);

  *str= '\0';
#endif
}



/*---------------------------------------------------------------------------
** CstrtoBSTR() converts the null-terminated string pointed to by "str" to
** a BSTR located at the byte address "BSTRp".  At most "maxsize" bytes will
** be stored.
*/

void  CstrtoBSTR (str, BSTRp, maxsize)

register char  *str;
BYTE           *BSTRp;
unsigned       maxsize;

{
#ifdef AROS_FAST_BSTR
  strncpy(BSTRp, str, maxsize);
  BSTRp[maxsize-1] = 0;
#else
  register char  *bp;
  register int   i, limit;

  bp= BSTRp + 1;

  limit= maxsize - 1;

  for (i= 0; i < limit; ++i)
    if ( (*(bp++)= *(str++)) == '\0' )
      break;

  BSTRp[0]= i;
#endif
}

/*---------------------------------------------------------------------------
** CstrtoFIB() converts the null-terminated string pointed to by "str" to
** a FIB fib_FileName or fib_Comment located at the byte address "BSTRp". 
** At most "maxsize" bytes will be stored.
*/

void  CstrtoFIB (str, BSTRp, maxsize)

register char  *str;
BYTE           *BSTRp;
unsigned       maxsize;

{
  register char  *bp;
  register int   i, limit;

  bp= BSTRp + 1;

  limit= maxsize - 1;

  for (i= 0; i < limit; ++i)
    if ( (*(bp++)= *(str++)) == '\0' )
      break;

  BSTRp[0]= i;
}




/*---------------------------------------------------------------------------
** inrange() returns nonzero iff x is in the range [lower, upper].
** uppercase() returns the uppercase version of the ASCII character sent.
** These are not implemented as macros to avoid hard-to-find bugs like
** uppercase(c++), where the side-effect occurs more than once.
*/

int  inrange (x, lower, upper)

register int  x;
register int  lower;
register int  upper;

{ return  ((x >= lower) && (x <= upper));
}


char  uppercase (c)

register char  c;

{ return  (char) (inrange (c, 'a', 'z') ? (c + ('A' - 'a')) : c);
}



/*---------------------------------------------------------------------------
** The null-terminated string "str" is scanned for the character "ch".  If
** found, a pointer to its first occurrence in "str" is returned.  Otherwise,
** a pointer to the terminating '\0' in "str" is returned.
*/

char  *findchar (str, ch)

register char  *str;
register char  ch;

{ while ((*str != '\0') && (*str != ch))
    ++str;

  return str;     /* return position of ch, or end if not found */
}



/*---------------------------------------------------------------------------
** This is just like strcpy().  Its is defined here to avoid including other
** libraries.
*/

void  l_strcpy (to, from)

register char  *to;
register char  *from;

{
#ifdef __AROS__
  strcpy(to, from);
#else
STRCPYLOOP:
  if (*(to++)= *(from++))
    goto STRCPYLOOP;
#endif
}



/*---------------------------------------------------------------------------
** strdiff() returns a pointer to the first difference in the two null-
** terminated strings "str1" and "str2".  If no differnce is found, or if
** "str1" is shorter than "str2", then a pointer to '\0' is returned.
** The returned pointer is to a character in "str1".
*/

char  *strdiff (str1, str2)

register char  *str1;
register char  *str2;

{ while ( *str1 && (uppercase (*str1) == uppercase (*str2)) )
    { ++str1;
      ++str2;
    }

  return str1;     /* return position of first difference, or end of str1 */
}



/*---------------------------------------------------------------------------
** get_autoname() returns a pointer to "autoname".  If "newflag" is nonzero,
** autoname is first updated so that it does not conflict with any existing
** pipe name.  This is done by looking for a block of ASCII digits in
** "autoname", and incrementing their effective value.  "autoname" MUST
** contain such a block of digits.
*/

#if AUTONAME || AUTONAME_STAR

static char  autoname[]  =  AUTONAME_INIT;

char  *get_autoname (newflag)

BYTE  newflag;

{ char      *cp, *cpc;
  PIPEDATA  *FindPipe();


  if (newflag)     /* then create a new unique pipe name */
    { cp= findchar (autoname, '\0');

      while (! isnumeral (*cp))     /* find last numeral */
        --cp;

      do
        { ++(*cp);     /* "increment" name */

          for (cpc= cp; (! isnumeral (*cpc)); )     /* ripple carry */
            { *(cpc--)= '0';

              if (! isnumeral (*cpc))
                break;     /* no more digits */

              ++(*cpc);
            }
        }
      while (FindPipe (autoname) != NULL);     /* repeat until name is unique */
    }


  return  autoname;
}

#endif /* AUTONAME || AUTONAME_STAR */



/*---------------------------------------------------------------------------
** ParseNum() parses the null-terminated string pointed to by "str" into a
** number, and stores its value in *nump.  ParseNum() returns nonzero iff
** successful.  Both '\0' and PIPE_SPEC_CHAR are acceptable terminators for
** the number.
**      If the number begins with "0x", it is interpreted as hexadecimal.
** If it begins with "0" but not "0x", it is interpreted as octal.
** Otherwise, it is interpreted as decimal.
*/

static int  ParseNum (str, nump)

char   *str;
ULONG  *nump;

{ int   radix    =  10;
  char  *digits  =  "0123456789ABCDEF";
  LONG  value;


  if ((*str == '0') && (uppercase (*(str + 1)) == 'X'))
    { radix= 16;
      str += 2;
    }
  else if (*str == '0')
    { radix= 8;
      ++str;
    }

  for (*nump= 0; TRUE; ++str)
    { value= findchar (digits, uppercase (*str)) - digits;

      if (! inrange (value, 0, (radix - 1)))
        break;

      if (*nump > ((MAX_PIPELEN - value) / radix))
        return FALSE;

      *nump *= radix;
      *nump += value;
    }


  return  ( (*str == PIPE_SPEC_CHAR) || (*str == '\0') );
}
