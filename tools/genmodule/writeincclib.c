/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
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
    struct linelist *linelistit;
    
    snprintf(line, 255, "%s/clib/%s_protos.h", cfg->genincdir, cfg->modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could write file %s\n", line);
	exit(20);
    }
    fprintf(out,
	    "#ifndef CLIB_%s_PROTOS_H\n"
	    "#define CLIB_%s_PROTOS_H\n"
	    "\n"
	    "/*\n"
	    "    *** Automatically generated file. Please do not edit ***\n"
	    "    Copyright © 1995-2004, The AROS Development Team. All rights reserved.\n"
	    "*/\n"
	    "\n"
	    "#include <aros/libcall.h>\n",
	    cfg->modulenameupper, cfg->modulenameupper);
    for (linelistit = cfg->cdeflines; linelistit!=NULL; linelistit = linelistit->next)
	fprintf(out, "%s\n", linelistit->line);
    if (cfg->command!=DUMMY && cfg->libcall!=STACK)
    {
	for (funclistit = funclist; funclistit!=NULL; funclistit = funclistit->next)
	{
	    if (!funclistit->priv && (funclistit->lvo >= cfg->firstlvo))
	    {
		fprintf(out, "\nAROS_LP%d(%s, %s,\n", funclistit->argcount, funclistit->type, funclistit->name);
		
		for (arglistit = funclistit->arguments; arglistit!=NULL; arglistit = arglistit->next)
		    fprintf(out, "        AROS_LPA(%s, %s, %s),\n",
			    arglistit->type, arglistit->name, arglistit->reg);

		fprintf(out, "        %s, %s, %u, %s)\n",
			cfg->libbasetypeptrextern, cfg->libbase, funclistit->lvo, cfg->basename);
	    }
	}
    }
    fprintf(out, "\n#endif /* CLIB_%s_PROTOS_H */\n", cfg->modulenameupper);
}
