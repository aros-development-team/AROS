/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Code to parse the command line options for the mkkeymap program
*/

#define __USE_XOPEN

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>

#include "config.h"

const static char usage[] =
    "\n"
    "Usage: mkkeymap [-d descriptor] [-k keymap] [-v]\n"
;

/* Create a config struct. Initialize with the values from the programs command
 * line arguments
 */
struct config *initconfig(int argc, char **argv)
{
    struct config *cfg;
    char *s, **argvit = argv + 1;
    int hassuffix = 0, c;

    cfg = malloc(sizeof(struct config));
    if (cfg == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        exit(20);
    }

    memset(cfg, 0, sizeof(struct config));

    while ((c = getopt(argc, argv, ":d:k:v")) != -1)
    {
        switch (c)
        {
        case 'd':
            cfg->descriptor = optarg;
            break;

        case 'k':
            cfg->keymap = optarg;
            break;

        case 'v':
            cfg->verbose = TRUE;
            break;

        case '?':
            if ((optopt == 'd') || (optopt == 'k'))
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
            exit(20);

        default:
            exit(20);
        }
    }

    if (((!cfg->descriptor) && (!cfg->keymap)) || (optind < 3)  || (optind != argc))
    {
        fprintf(stderr, "Wrong number of arguments.\n%s", usage);
        exit(20);
    }

    if (!cfg->descriptor)
    {
        char *ext;
        /* make the descriptor name based on the keymap name .. */
        cfg->descriptor = malloc (strlen (cfg->keymap) + 5 + 1);
        strcpy (cfg->descriptor, cfg->keymap);
        strcat (cfg->descriptor, ".akmd");
    }

    return cfg;
}
