/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write clib/modulename_protos.h. Part of genmodule.
*/
#include "genmodule.h"

void writeincclib(struct config *cfg)
{
    FILE *out;
    char line[256];
    struct functionhead *funclistit;
    struct functionarg *arglistit;
    struct stringlist *linelistit;
    
    snprintf(line, 255, "%s/clib/%s_protos.h", cfg->gendir, cfg->modulename);

    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
        exit(20);
    }

    fprintf(out,
	    "#ifndef CLIB_%s_PROTOS_H\n"
	    "#define CLIB_%s_PROTOS_H\n"
	    "\n"
        "%s"
	    "\n"
	    "#include <aros/libcall.h>\n",
	    cfg->modulenameupper, cfg->modulenameupper, getBanner(cfg)
    );
    for (linelistit = cfg->cdeflines; linelistit!=NULL; linelistit = linelistit->next)
	fprintf(out, "%s\n", linelistit->s);

    if (cfg->command!=DUMMY)
	writefuncprotos(out, cfg, cfg->funclist);

    fprintf(out, "\n#endif /* CLIB_%s_PROTOS_H */\n", cfg->modulenameupper);
}
