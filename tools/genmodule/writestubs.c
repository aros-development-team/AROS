/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: function to write module_autoinit.c. Part of genmodule.
*/
#include "genmodule.h"

void writestubs(void)
{
    FILE *out;
    struct functionlist *funclistit;
    struct arglist *arglistit;
    unsigned int start;

    snprintf(line, slen-1, "%s/%s_stubs.c", gendir, modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could not write %s\n", line);
	exit(20);
    }
    fprintf(out,
	    "/*\n"
	    "    *** Automatically generated file. Do not edit ***\n"
	    "    Copyright © 1995-2002, The AROS Development Team. All rights reserved.\n"
	    "*/\n"
	    "#define NOLIBDEFINES\n"
	    "#include <proto/%s.h>\n"
	    "#include <exec/types.h>\n"
	    "#include <aros/libcall.h>\n"
	    "\n",
	    modulename);
    for (funclistit = funclist, start=firstlvo;
	 funclistit!=NULL;
	 funclistit = funclistit->next, start++)
    {
	fprintf(out,
		"\n"
		"%s %s(",
		funclistit->type, funclistit->name);
	for (arglistit = funclistit->arguments;
	     arglistit!=NULL;
	     arglistit = arglistit->next)
	{
	    if (arglistit != funclistit->arguments)
		fprintf(out, ", ");
	    fprintf(out, "%s %s", arglistit->type, arglistit->name);
	}
	fprintf(out,
		")\n"
		"{\n"
		"    return AROS_LC%d(%s, %s,\n",
		funclistit->argcount, funclistit->type, funclistit->name);
	for (arglistit = funclistit->arguments;
	     arglistit!=NULL;
	     arglistit = arglistit->next)
	    fprintf(out, "                    AROS_LCA(%s,%s,%s),\n",
		    arglistit->type, arglistit->name, arglistit->reg);
	fprintf(out, "                    %s *, %s, %d, %s);\n}\n", libbasetypeextern, libbase, start, basename);
    }
    fclose(out);
}
