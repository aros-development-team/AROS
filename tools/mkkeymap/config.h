/*
    Copyright © 2020, The AROS Development Team. All rights reserved.

    Desc: Define the C structure for storing the command line options
*/

#ifndef _CONFIG_H
#define _CONFIG_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

struct config
{
    char *descriptor;
    char *keymap;
    int verbose;
};

/* Function prototypes */

struct config *initconfig(int, char **);

#endif //_CONFIG_H
