/*
    Copyright � 1995-2002, The AROS Development Team. All rights reserved.

    Desc: function to write clib/modulename_protos.h. Part of genmodule.
*/
#include "genmodule.h"

void writeincclib(void)
{
    FILE *out;
    struct functionlist *funclistit;
    struct arglist *arglistit;
    struct linelist *linelistit;
    unsigned int start;
    
    snprintf(line, slen-1, "%s/clib/%s_protos.h", genincdir, modulename);
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
	    "    Copyright � 1995-2002, The AROS Development Team. All rights reserved.\n"
	    "*/\n"
	    "\n"
	    "#include <aros/libcall.h>\n",
	    modulenameupper, modulenameupper);
    for (linelistit = cliblines; linelistit!=NULL; linelistit = linelistit->next)
	fprintf(out, "%s\n", linelistit->line);
    for (funclistit = funclist, start = 5;
	 funclistit!=NULL;
	 funclistit = funclistit->next, start++)
    {
	fprintf(out, "\nAROS_LP%d(%s, %s,\n", funclistit->argcount, funclistit->type, funclistit->name);

	for (arglistit = funclistit->arguments;
	     arglistit!=NULL;
	     arglistit = arglistit->next)
	    fprintf(out, "        AROS_LPA(%s, %s,),\n", arglistit->type, arglistit->name);

	fprintf(out, "        struct Library *, %sBase, %d, %s)\n",
		basename, start, basename);
    }
    fprintf(out, "\n#endif /* CLIB_%s_PROTOS_H */\n", modulenameupper);
}
