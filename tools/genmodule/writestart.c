/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write modulename_start.c. Part of genmodule.
*/
#include "genmodule.h"

void writestart(void)
{
    FILE *out;
    char line[256];
    struct functionlist *funclistit;
    struct arglist *arglistit;
    struct linelist *linelistit;
    unsigned int lvo;
    int i;
    
    snprintf(line, 255, "%s/%s_start.c", gendir, modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could not write %s\n", line);
	exit(20);
    }
    fprintf(out,
	    "/*\n"
	    "    *** Automatically generated file. Do not edit ***\n"
	    "    Copyright © 1995-2004, The AROS Development Team. All rights reserved.\n"
	    "*/\n"
	    "#include <libcore/libheader.c>\n");
    
    for (linelistit = cdeflines; linelistit!=NULL; linelistit = linelistit->next)
	fprintf(out, "%s\n", linelistit->line);
    
    for (funclistit = funclist; funclistit != NULL; funclistit = funclistit->next)
    {
	switch (libcall)
	{
	case STACK:
	    fprintf(out, "int %s();\n", funclistit->name);
	    break;
	    
	case REGISTER:
	    fprintf(out, "%s %s(", funclistit->type, funclistit->name);
	    for (arglistit = funclistit->arguments;
		 arglistit!=NULL;
		 arglistit = arglistit->next)
	    {
		if (arglistit!=funclistit->arguments)
		    fprintf(out, ", ");
		fprintf(out, "%s %s", arglistit->type, arglistit->name);
	    }
	    fprintf(out, ");\nAROS_LH%d(%s, %s,\n",
		    funclistit->argcount, funclistit->type, funclistit->name);
	    for (arglistit = funclistit->arguments;
		 arglistit!=NULL;
		 arglistit = arglistit->next)
	    {
		fprintf(out, "         AROS_LHA(%s, %s, %s),\n",
			arglistit->type, arglistit->name, arglistit->reg);
	    }
	    fprintf(out,
		    "         %s *, %s, %u, %s)\n"
		    "{\n"
		    "    AROS_LIBFUNC_INIT\n\n"
		    "    return %s(",
		    libbasetypeextern, libbase, funclistit->lvo, basename, funclistit->name);
	    for (arglistit = funclistit->arguments;
		 arglistit!=NULL;
		 arglistit = arglistit->next)
	    {
		if (arglistit!=funclistit->arguments)
		    fprintf(out, ", ");
		fprintf(out, "%s", arglistit->name);
	    }
	    fprintf(out,
		    ");\n\n"
		    "    AROS_LIBFUNC_EXIT\n"
		    "}\n\n");
	    break;
	    
	case REGISTERMACRO:
	    fprintf(out, "int LC_BUILDNAME(%s)();\n", funclistit->name);
	    break;

	default:
	    fprintf(stderr, "Internal error: unhandled libcall in writestart\n");
	    exit(20);
	    break;
	}
    }
    
    fprintf(out,
	    "\n"
	    "const APTR %s_functable[]=\n"
	    "{\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader),\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader),\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader),\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader),\n",
	    modulename);

    lvo = 4;
    for (funclistit = funclist; funclistit != NULL; funclistit = funclistit->next)
    {
	for (i = lvo+1; i<funclistit->lvo; i++)
	    fprintf(out, "    NULL,\n");
	lvo = funclistit->lvo;
	
	switch (libcall)
	{
	case STACK:
	    fprintf(out, "    &%s,\n", funclistit->name);
	    break;
	    
	case REGISTER:
	case REGISTERMACRO:
	    fprintf(out, "    &AROS_SLIB_ENTRY(%s,%s),\n", funclistit->name, basename);
	    break;
	    
	default:
	    fprintf(stderr, "Internal error: unhandled libcall type in writestart\n");
	    exit(20);
	    break;
	}
    }

    fprintf(out, "    (void *)-1\n};\n");
    fclose(out);
}
