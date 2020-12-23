/*
    Copyright © 2020, The AROS Development Team. All rights reserved.

    Desc: Define the C structure for storing the command line options
*/

#ifndef _CONFIG_H
#define _CONFIG_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif

#include <devices/keymap.h>

struct config
{
    char        *descriptor;
    char        *keymap;
    int         verbose;
    int         bitorder; 
    UBYTE       LoKeyMapTypes[0x40];
    IPTR        LoKeyMap[0x40];
    UBYTE       LoCapsable[0x08];
    UBYTE       LoRepeatable[0x08];
    UBYTE       HiKeyMapTypes[0x38];
    IPTR        HiKeyMap[0x38];
    UBYTE       HiCapsable[0x07];
    UBYTE       HiRepeatable[0x07];
    struct List KeyDesc;
};

/* Function prototypes */

struct config *initconfig(int, char **);

#endif //_CONFIG_H
