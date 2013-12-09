/* MetaMake - A Make extension
   Copyright © 1995-2011, The AROS Development Team. All rights reserved.

This file is part of MetaMake.

MetaMake is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

MetaMake is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */
/* Includes */

//#define DEBUG_MMAKE

#include "config.h"

#ifdef PROTOTYPES
#   define PARAMS(x) x
#else
#   define PARAMS(x) ()
#endif /* PROTOTYPES */

#if defined(HAVE_STDARG_H) && defined(__STDC__) && __STDC__
#   include <stdarg.h>
#   define VA_START(args, lastarg) va_start(args, lastarg)
#else
#   include <varargs.h>
#   define VA_START(args, lastarg) va_start(args)
#endif

#ifndef __DATE__
#   define __DATE__ "No __DATE__"
#endif

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#ifdef HAVE_STRING_H
#   include <string.h>
#else
#   include <strings.h>
#endif
#ifdef HAVE_SYS_STAT_H
#   include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif

#include "list.h"
#include "mem.h"
#include "var.h"
#include "dep.h"
#include "project.h"

#if defined(DEBUG_MMAKE)
#define debug(a) a
#else
#define debug(v)
#endif

/* globals */
char * mflags[64];
int mflagc;
int verbose = 0;
int quiet = 0;
int debug = 0;

char *mm_srcdir;    /* Location to scan for cfg files */
char *mm_builddir;  /* Location to generate files/build in */

/* Functions */
void
error (char * fmt, ...)
{
    va_list args;
    VA_START (args, fmt);
    fprintf (stderr, "[MMAKE] Error: ");
    vfprintf (stderr, fmt, args);
    if (errno != 0)
	fprintf (stderr, ": %s", strerror (errno));
    fprintf (stderr, "\n");
    va_end (args);
}


int
main (int argc, char ** argv)
{
    char * currdir;
    int t;
    char * targets[64];
    int targetc;

    currdir = getcwd (NULL, 1024);

    mm_srcdir = currdir;
    mm_builddir = currdir;

    mflagc = targetc = 0;

    for (t=1; t<argc; t++)
    {
	if (argv[t][0] == '-')
	{
	    if (!strcmp (argv[t], "--version"))
	    {
		printf ("MetaMake %s (%s)\n", PACKAGE_VERSION, __DATE__);
		if (argc == 2)
		    exit (0);
	    }
	    else if (!strncmp (argv[t], "--srcdir", 8) || !strcmp (argv[t], "-s"))
	    {
		mm_srcdir = (char *)&argv[t][9];
	    }
	    else if (!strncmp (argv[t], "--builddir", 10) || !strcmp (argv[t], "-b"))
	    {
		mm_builddir = (char *)&argv[t][11];
	    }
	    else if (!strcmp (argv[t], "--verbose") || !strcmp (argv[t], "-v"))
	    {
		verbose = 1;
	    }
	    else if (!strcmp (argv[t], "--quiet") || !strcmp (argv[t], "-q"))
	    {
		quiet = 1;
	    }
	    else if (!strcmp (argv[t], "--debug"))
	    {
		debug = 1;
	    }
	    else if (!strcmp (argv[t], "--help"))
	    {
		printf ("%s [--srcdir=<directory>] [--builddir=<directory>] [--version] [-v,--verbose] [-q,--quiet] [--debug] [--help]\n", argv[0]);
		return 0;
	    }
	    else
	    {
		mflags[mflagc++] = argv[t];
	    }
	}
	else
	{
	    targets[targetc++] = argv[t];
	}
    }

    if (verbose)
    {
	quiet = 0;
	printf ("SRCDIR   '%s'\n", mm_srcdir);
	printf ("BUILDDIR '%s'\n", mm_builddir);
    }

    if (debug)
    {
	quiet = 0;
    }

    debug(printf("MMAKE:mmake.c->main: parsed command line options\n"));

    initprojects ();

    debug(printf("MMAKE:mmake.c->main: projects initialised\n"));

    if (!targetc)
    {
	struct Project * firstprj = getfirstproject ();

	assert (firstprj);

	targets[targetc++] = firstprj->node.name;
	debug(printf("MMAKE:mmake.c->main: targetc not set, using default'%s'\n", firstprj->node.name));
    }

    for (t=0; t<targetc; t++)
    {
	char * pname, * tname, * ptr;
	struct Project * prj;

	pname = ptr = targets[t];
	while (*ptr && *ptr != '.')
	    ptr ++;
	if (*ptr)
	    *ptr ++ = 0;
	tname = ptr;

	prj = findproject (pname);

	if (!prj)
	{
	    printf ("[MMAKE] Nothing known about project %s\n", pname);
	    return 20;
	}

	debug(printf("MMAKE:mmake.c->main: calling maketarget '%s'\n", tname));
	maketarget (prj, tname);
    }

    expungeprojects ();

    ASSERT(chdir (currdir) == 0);

    free (currdir);

    return 0;
}

