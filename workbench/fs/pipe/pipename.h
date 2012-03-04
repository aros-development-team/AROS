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



extern int   ParsePipeName ( /* Bname, nmp, sizep, tapnmp */ );
extern void  BSTRtoCstr    ( /* BSTRp, str, maxsize */ );
extern void  CstrtoBSTR    ( /* str, BSTRp, maxsize */ );
extern void  CstrtoFIB     ( /* str, BSTRp, maxsize */ );
extern int   inrange       ( /* x, lower, upper */ );
extern char  uppercase     ( /* c */ );
extern char  *findchar     ( /* str, ch */ );
extern void  l_strcpy      ( /* to, from */ );
extern char  *strdiff      ( /* str1, str2 */ );

extern char  *get_autoname ( /* newflag */ );
