/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: function to write defines/modulename.h. Part of genmodule.
*/
#include "genmodule.h"

void writeincdefines(void)
{
    FILE *out;
    struct functionlist *funclistit;
    struct arglist *arglistit;
    unsigned int start;
    
    snprintf(line, slen-1, "%s/defines/%s.h", genincdir, modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could not write %s\n", line);
	exit(20);
    }
    fprintf(out,
	    "#ifndef DEFINES_%s_PROTOS_H\n"
	    "#define DEFINES_%s_PROTOS_H\n"
	    "\n"
	    "/*\n"
	    "    *** Automatically generated file. Do not edit ***\n"
	    "    Copyright © 1995-2002, The AROS Development Team. All rights reserved.\n"
	    "\n"
	    "    Desc: Prototype for %s\n"
	    "*/\n"
	    "\n"
	    "#include <aros/libcall.h>\n"
	    "#include <exec/types.h>\n"
	    "\n",
	    modulenameupper, modulenameupper, modulename);
    for (funclistit = funclist, start=5;
	 funclistit!=NULL;
	 funclistit = funclistit->next, start++)
    {
	fprintf(out,
		"\n"
		"#define %s(",
		funclistit->name);
	for (arglistit = funclistit->arguments;
	     arglistit!=NULL;
	     arglistit = arglistit->next)
	{
	    if (arglistit != funclistit->arguments)
		fprintf(out, ", ");
	    fprintf(out, "%s", arglistit->name);
	}
	fprintf(out,
		") \\\n"
		"        AROS_LC%d(%s, %s, \\\n",
		funclistit->argcount, funclistit->type, funclistit->name);
	for (arglistit = funclistit->arguments;
	     arglistit!=NULL;
	     arglistit = arglistit->next)
	    fprintf(out, "        AROS_LCA(%s,%s,), \\\n", arglistit->type, arglistit->name);
	fprintf(out, "        struct Library *, %sBase, %d, %s)\n", basename, start, basename);
    }
    fprintf(out,
	    "\n"
	    "#endif /* DEFINES_%s_PROTOS_H*/\n",
	    modulenameupper);
    fclose(out);
}
