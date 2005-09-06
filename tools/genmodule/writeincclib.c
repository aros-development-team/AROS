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
    {
	char *type, *name;
	
	for (funclistit = cfg->funclist;
	     funclistit!=NULL;
	     funclistit = funclistit->next
	)
	{
	    if (!funclistit->priv
		&& (funclistit->libcall != STACK)
		&& (funclistit->lvo >= cfg->firstlvo)
	    )
	    {
		fprintf(out,
			"\nAROS_LP%d(%s, %s,\n",
			funclistit->argcount, funclistit->type, funclistit->name
		);
		
		for (arglistit = funclistit->arguments;
		     arglistit!=NULL;
		     arglistit = arglistit->next
		)
		{
		    type = getargtype(arglistit);
		    name = getargname(arglistit);
		    assert(type != NULL && name != NULL);
		    fprintf(out,
			    "        AROS_LPA(%s, %s, %s),\n",
			    type, name, arglistit->reg
		    );
		    free(type);
		    free(name);
		}

		fprintf(out,
			"        %s, %s, %u, %s)\n",
			cfg->libbasetypeptrextern, cfg->libbase, funclistit->lvo,
			cfg->basename
		);
	    }
	}
    }
    fprintf(out, "\n#endif /* CLIB_%s_PROTOS_H */\n", cfg->modulenameupper);
}
