/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <stdio.h>
#include <stdlib.h>

#include  <workbench/icon.h>

#include <proto/exec.h>
#include <proto/icon.h>

int main(int argc, char **argv)
{
    WORD maxw, maxh;
    ULONG scalebox;

    if (argc != 3) {
        printf("Usage:\n%s <width> <height>\n", argv[0]);
        return EXIT_FAILURE;
    }

    maxw = strtol(argv[1], NULL, 0);
    maxh = strtol(argv[2], NULL, 0);

    scalebox = PACK_ICON_SCALEBOX(maxw, maxh);

    IconControl(NULL, ICONCTRLA_SetGlobalScaleBox, scalebox, TAG_END);

    return EXIT_SUCCESS;
}
