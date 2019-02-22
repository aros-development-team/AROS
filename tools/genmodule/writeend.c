/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.

    Desc: function to write modulename_end.c. Part of genmodule.
*/
#include "genmodule.h"

void writeend(struct config *cfg)
{
    FILE *out;
    char line[256];

    snprintf(line, 255, "%s/%s_end.c", cfg->gendir, cfg->modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
        fprintf(stderr, "Could not write %s\n", line);
        exit(20);
    }

    if (!cfg->flavour)
    {
        fprintf(out,
                "#include \"%s_libdefs.h\"\n",
                cfg->modulename
        );
    }
    else
    {
        fprintf(out,
                "#include \"%s_%s_libdefs.h\"\n",
                cfg->modulename, cfg->flavour
        );
    }

    fprintf(out,
            "int GM_UNIQUENAME(End)(void) {return 0;}\n"
    );
    fclose(out);
}
