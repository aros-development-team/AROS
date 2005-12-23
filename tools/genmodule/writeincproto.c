/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.

    Desc: Function to write proto/modulename.h. Part of genmodule.
*/
#include "genmodule.h"

void writeincproto(struct config *cfg)
{
    FILE *out;
    char line[256];
    struct linelist *linelistit;
    
    snprintf(line, 255, "%s/proto/%s.h", cfg->gendir, cfg->modulename);
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
    	exit(20);
    }

    fprintf(out,
	    "#ifndef PROTO_%s_H\n"
	    "#define PROTO_%s_H\n"
	    "\n"
            "%s"
	    "\n"
	    "#include <exec/types.h>\n"
	    "#include <aros/system.h>\n"
	    "\n"
	    "#include <clib/%s_protos.h>\n"
	    "\n"
	    "#if !defined(%s) && !defined(__NOLIBBASE__) && !defined(__%s_NOLIBBASE__)\n"
	    "extern %s%s;\n"
	    "#endif\n"
	    "\n",
	    cfg->modulenameupper, cfg->modulenameupper, getBanner(cfg),
	    cfg->modulename,
	    cfg->libbase, cfg->modulenameupper,
	    cfg->libbasetypeptrextern, cfg->libbase
    );
    
    fprintf(out,
	    "#if !defined(NOLIBDEFINES) && !defined(%s_NOLIBDEFINES)\n"
	    "#   include <defines/%s.h>\n"
	    "#endif\n"
	    "\n"
	    "#endif /* PROTO_%s_H */\n",
	    cfg->modulenameupper, cfg->modulename, cfg->modulenameupper
    );
    fclose(out);
}
