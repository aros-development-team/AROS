/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Main for mkkeymap. A tool to generate AmigaOS hunk format Keymap's.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mkkeymap.h"
#include "debug.h"

int main(int argc, char **argv)
{
    char *s;
    struct KeyMapNode *keyMap;
    struct config *cfg = initconfig(argc, argv);

    if (cfg->verbose)
    {
        fprintf(stdout, "mkamikeymap 0.4 © 2020, The AROS Development Team.\n");

        fprintf(stdout, "parsing descriptor: %s\n", cfg->descriptor);
    }

    if (parseKeyDescriptor(cfg))
    {
        if (!cfg->keymap)
        {
            char *ext;
            /* make the keymap name based on the descriptor name .. */
            cfg->keymap = malloc (strlen (cfg->descriptor) + 1);
            strcpy (cfg->keymap, cfg->descriptor);
            ext = strrchr (cfg->keymap, '.');
            if (ext)
            {
                *ext = '\0';
            }
        }
        if (writeKeyMap(cfg))
        {
            D(fprintf(stdout, "%s successfully generated\n", cfg->keymap);)
        }
    }

    return 0;
}
