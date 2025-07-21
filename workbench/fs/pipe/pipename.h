/****************************************************************************
**  File:       pipename.h
**  Program:    pipe-handler - an AmigaDOS handler for named pipes
**  Version:    1.1
**  Author:     Ed Puckett      qix@mit-oz
**
**  Copyright 1987 by EpAc Software.  All Rights Reserved.
**
**  History:    05-Jan-87       Original Version (1.0)
**		07-Feb-87	Added conditional compilation for autoname.
*/



/*---------------------------------------------------------------------------
** PIPENAMELEN		: this is the maximum length of names ParsePipeName()
**			can handle.
**
** DEFAULT_PIPELEN	: the default pipe size returned by ParsePipeName()
**			if no size is specified.
**
** PIPE_SPEC_CHAR	: this is the character used by ParsePipeName() as an
**			identifier for specifiers.  See pipename.c
**
** DEFAULT_TAPNAME_PREFIX : the prefix for default tap names.  See pipename.c
**
** AUTONAME_INIT	: Initial value used by get_autoname() to form
**			default pipe names.  It MUST contain a block of
**			digits.  See pipename.c.
**			This is only used if AUTONAME or AUTONAME_STAR is true.
*/

#define   PIPENAMELEN        108

#define   DEFAULT_PIPELEN   4096

#define   PIPE_SPEC_CHAR           '/'
#define   DEFAULT_TAPNAME_PREFIX   "CON:10/15/300/70/"

# define   AUTONAME_INIT            "*00000000"

#define   isnumeral(c)   inrange ((c), '0', '9')

extern int   ParsePipeName ( BYTE *Bname, char **nmp, ULONG *sizep, char **tapnmp );
extern void  BSTRtoCstr    ( register BYTE *BSTRp, register char *str, unsigned maxsize );
extern void  CstrtoBSTR    ( register const char *str, register BYTE *BSTRp, unsigned maxsize );
extern void  CstrtoFIB     ( register char *str, register BYTE *BSTRp, unsigned maxsize );
extern int   inrange       ( register int x, register int  lower, register int  upper );
extern char  uppercase     ( register char c );
extern char  *findchar     ( register char *str, register char ch );
extern void  l_strcpy      ( register char *to, register char *from );
extern char  *strdiff      ( register char *str1, register char *str2 );

extern char  *get_autoname ( BYTE newflag );
