/*
    Copyright (C) 1995-2019, The AROS Development Team. All rights reserved.

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

    /*
     * The module's Resident rt_EndSkip points at this End marker so the romtag
     * scanner can leap from a module's tag to its end in one step. For that the
     * marker must be the LAST thing in the module. It is emitted into its own
     * ".text.moduleend" section; the m68k ROM link (see arch/m68k-amiga/boot,
     * config/make.tmpl KERNEL_KOBJ_LDSCRIPT) orders sections so .text.moduleend
     * comes last, making &End the true module end.
     */
    fprintf(out,
            "__section(\".text.moduleend\") const char GM_UNIQUENAME(End)[] = { 0 };\n"
    );
    fclose(out);
}
