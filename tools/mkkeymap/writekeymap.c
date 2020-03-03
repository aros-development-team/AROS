/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <dos/doshunks.h>

#include "mkkeymap.h"
#include "debug.h"

BOOL writeKeyMap(struct config *cfg)
{
    BOOL doverbose = cfg->verbose;
    D(doverbose = TRUE;)

    if (doverbose)
        fprintf(stdout, "creating keymap '%s'\n", cfg->keymap);

}
