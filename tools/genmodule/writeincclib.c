/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write clib/modulename_protos.h. Part of genmodule.
*/
#include "genmodule.h"

void writeincclib(int dummy)
{
    FILE *out;
    char line[256];
    struct functionhead *funclistit;
    struct functionarg *arglistit;
    struct linelist *linelistit;
    
    snprintf(line, 255, "%s/clib/%s_protos.h", genincdir, modulename);
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
	    "    Copyright © 1995-2003, The AROS Development Team. All rights reserved.\n"
	    "*/\n"
	    "\n"
	    "#include <aros/libcall.h>\n",
	    modulenameupper, modulenameupper);
    for (linelistit = cdeflines; linelistit!=NULL; linelistit = linelistit->next)
	fprintf(out, "%s\n", linelistit->line);
    if (!dummy && libcall!=STACK)
    {
	for (funclistit = funclist; funclistit!=NULL; funclistit = funclistit->next)
	{
	    if (funclistit->lvo >= firstlvo)
	    {
		fprintf(out, "\nAROS_LP%d(%s, %s,\n", funclistit->argcount, funclistit->type, funclistit->name);
		
		for (arglistit = funclistit->arguments; arglistit!=NULL; arglistit = arglistit->next)
		    fprintf(out, "        AROS_LPA(%s, %s, %s),\n",
			    arglistit->type, arglistit->name, arglistit->reg);

		fprintf(out, "        struct Library *, %sBase, %u, %s)\n",
			basename, funclistit->lvo, basename);
	    }
	}
    }
    fprintf(out, "\n#endif /* CLIB_%s_PROTOS_H */\n", modulenameupper);
}
