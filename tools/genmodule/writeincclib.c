/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Function to write clib/modulename_protos.h. Part of genmodule.
*/
#include "genmodule.h"

void writeincclib(struct config *cfg)
{
    FILE *out;
    char line[256], *banner;
    struct functionhead *funclistit;
    struct functionarg *arglistit;
    struct stringlist *linelistit;

    snprintf(line, 255, "%s/clib/%s_protos.h", cfg->gendir, cfg->includename);

    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
        exit(20);
    }

    banner = getBanner(cfg);
    fprintf(out,
            "#ifndef CLIB_%s_PROTOS_H\n"
            "#define CLIB_%s_PROTOS_H\n"
            "\n"
            "%s"
            "\n"
            "#include <aros/libcall.h>\n"
            "\n",
            cfg->includenameupper, cfg->includenameupper, banner
    );
    freeBanner(banner);

    for (linelistit = cfg->cdeflines; linelistit!=NULL; linelistit = linelistit->next)
        fprintf(out, "%s\n", linelistit->s);

    fprintf(out,
            "\n"
            "__BEGIN_DECLS\n"
            "\n"
    );

    writefuncprotos(out, cfg, cfg->funclist);

    fprintf(out,
            "\n"
            "__END_DECLS\n"
            "\n"
            "#endif /* CLIB_%s_PROTOS_H */\n",
            cfg->includenameupper);
}
