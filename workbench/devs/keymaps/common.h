/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Lang: English
*/

#include <devices/keymap.h>

#define DEFINE_KEYMAP(kmname)                   \
                                                 \
STATIC char  keymapname[] = kmname;             \
                                                \
STATIC CONST UBYTE lokeymaptypes[];             \
STATIC CONST IPTR  lokeymap[];                  \
STATIC CONST UBYTE locapsable[];                \
STATIC CONST UBYTE lorepeatable[];              \
                                                \
STATIC CONST UBYTE hikeymaptypes[];             \
STATIC CONST IPTR  hikeymap[];                  \
STATIC CONST UBYTE hicapsable[];                \
STATIC CONST UBYTE hirepeatable[];              \
                                                \
CONST struct KeyMapNode km =                    \
{                                               \
    {                                           \
        NULL, NULL, 0, 0, keymapname            \
    },                                          \
    {                                           \
        (UBYTE *)lokeymaptypes,                 \
        (IPTR  *)lokeymap,                      \
        (UBYTE *)locapsable,                    \
        (UBYTE *)lorepeatable,                  \
        (UBYTE *)hikeymaptypes,                 \
        (IPTR  *)hikeymap,                      \
        (UBYTE *)hicapsable,                    \
        (UBYTE *)hirepeatable                   \
    }                                           \
};
