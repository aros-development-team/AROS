/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MAIN_H
#define _MAIN_H

/* RDArgs */
#ifdef DEBUG
  #define ARG_TEMPLATE    "SCRIPT/K,APPNAME,MINUSER,DEFUSER,LOGFILE,NOLOG/S,NOPRETEND/S,NOPRINT/S,LANGUAGE/K"
#else /* DEBUG */
  #define ARG_TEMPLATE    "SCRIPT/K,APPNAME/A,MINUSER,DEFUSER,LOGFILE,NOLOG/S,NOPRETEND/S,NOPRINT/S,LANGUAGE/K"
#endif /* DEBUG */

#define ARG_SCRIPT	0
#define ARG_APPNAME	1
#define ARG_MINUSER	2
#define ARG_DEFUSER	3
#define ARG_LOGFILE	4
#define ARG_NOLOG	5
#define ARG_NOPRETEND	6
#define ARG_NOPRINT	7
#define ARG_LANGUAGE	8
#define TOTAL_ARGS	9


#endif /* _MAIN_H */

