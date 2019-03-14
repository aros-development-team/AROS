/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
*/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "parsendkoffsets.h"

#ifndef __DATE__
#   define __DATE__ "No __DATE__"
#endif
#ifndef PACKAGE_VERSION
#   define PACKAGE_VERSION "No PACKAGE_VERSION"
#endif

char *bindir = NULL;
char *ndkofffile;
char *gendir;
char *sdkdir;

int verbose;

void printBanner(FILE *structfile, char *comment)
{
	fprintf(structfile, "%s Copyright (c) 2019, The AROS Dev team.\n%s NB: THIS IS AN AUTO GENERATED FILE!\n\n", comment, comment);
}

int
main (int argc, char ** argv)
{
    char * currdir;
    int t, retval = 0;
    currdir = getcwd (NULL, 1024);

    ndkofffile = currdir;
    gendir = currdir;
    sdkdir = currdir;

    for (t=1; t<argc; t++)
    {
        if (argv[t][0] == '-')
        {
            if (!strcmp (argv[t], "--version"))
            {
                printf ("parseoffsets %s (%s)\n", PACKAGE_VERSION, __DATE__);
                if (argc == 2)
                    exit (0);
            }
            else if (!strncmp (argv[t], "--offsetsfile", 13) || !strcmp (argv[t], "-o"))
            {
                ndkofffile = (char *)&argv[t][14];
            }
            else if (!strncmp (argv[t], "--gendir", 8) || !strcmp (argv[t], "-g"))
            {
                gendir = (char *)&argv[t][9];
            }
            else if (!strncmp (argv[t], "--sdkdir", 8) || !strcmp (argv[t], "-s"))
            {
                sdkdir = (char *)&argv[t][9];
            }
            else if (!strncmp (argv[t], "--bindir", 8) || !strcmp (argv[t], "-b"))
            {
                bindir = (char *)&argv[t][9];
            }
            else if (!strcmp (argv[t], "--verbose") || !strcmp (argv[t], "-v"))
            {
                verbose = 1;
            }
            else if (!strcmp (argv[t], "--help"))
            {
                printf ("%s [--offsetsfile=<file>] [--sdkdir=<directory>] [--gendir=<directory>] [--bindir=<directory>] [--version] [-v,--verbose] [--help]\nfor correct results this programs output should be used on a big-endian 32bit platform.\n", argv[0]);
                return retval;
            }
        }
    }

    if (bindir == NULL)
        bindir = gendir;

    if (t >= 3)
    {
        if (verbose)
        {
            printf ("Offsets File   '%s'\n", ndkofffile);
            printf ("SDKDIR         '%s'\n", sdkdir);
            printf ("GENDIR         '%s'\n", gendir);
            printf ("Target Bins    '%s'\n", bindir);
        }

        retval = parsendkoffsets (ndkofffile, sdkdir, gendir, bindir);
    }
    return retval;
}
