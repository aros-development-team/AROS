/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: function to write defines/modulename.h. Part of genmodule.
*/
#include "genmodule.h"

void writeincdefines(void)
{
    FILE *out;
    char line[256];
    struct functionlist *funclistit;
    struct arglist *arglistit;

    snprintf(line, 255, "%s/defines/%s.h", genincdir, modulename);
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
    for (funclistit = funclist; funclistit!=NULL; funclistit = funclistit->next)
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
	    fprintf(out, "                  AROS_LCA(%s,%s,%s), \\\n",
		    arglistit->type, arglistit->name, arglistit->reg);
	fprintf(out, "        %s *, %s, %u, %s)\n", libbasetypeextern, libbase,
		funclistit->lvo, basename);
    }
    fprintf(out,
	    "\n"
	    "#endif /* DEFINES_%s_PROTOS_H*/\n",
	    modulenameupper);
    fclose(out);
}
