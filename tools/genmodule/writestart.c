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
    struct functionhead *funclistit;
    struct functionarg *arglistit;
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
    );
    fprintf(out,
	    "#include <exec/types.h>\n"
	    "#include <aros/libcall.h>\n"
	    "#include <aros/asmcall.h>\n"
    );
    if (!(options & OPTION_NORESIDENT))
	fprintf(out, "#include <libcore/libheader.c>\n");
    else
	fprintf(out, "#include LC_LIBDEFS_FILE\n");

    if (libcall == REGISTER)
    {
	for (linelistit = cdeflines; linelistit!=NULL; linelistit = linelistit->next)
	    fprintf(out, "%s\n", linelistit->line);
    }
    
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

    if (!(options & OPTION_NORESIDENT))
    {
	fprintf(out,
		"\n"
		"const APTR %s_functable[]=\n"
		"{\n"
		"    &AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader),\n"
		"    &AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader),\n"
		"    &AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader),\n"
		"    &AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader),\n",
		modulename
	);
	funclistit = funclist;
    }
    else /* NORESIDENT */
    {
	int neednull = 0;
	struct functionhead *funclistit2;
	
	funclistit = funclist;
	if (funclistit->lvo != 1)
	{
	    fprintf(stderr, "Module without a generated resident structure has to provide the Open function (LVO==1)\n");
	    exit(20);
	}
	else
	    funclistit = funclistit->next;
	
	if (funclistit->lvo != 2)
	{
	    fprintf(stderr, "Module without a generated resident structure has to provide the Close function (LVO==2)\n");
	    exit(20);
	}
	else
	    funclistit = funclistit->next;
	
	if (funclistit->lvo == 3)
	    funclistit = funclistit->next;
	else
	    neednull = 1;
	
	if (funclistit->lvo == 4)
	    funclistit = funclistit->next;
	else
	    neednull = 1;

	if (neednull)
	    fprintf(out,
		    "\n"
		    "AROS_UFH1(static int, %s_null,\n"
		    "          AROS_UFHA(struct Library *, libbase, A6)\n"
		    ")\n"
		    "{\n"
		    "    AROS_USERFUNC_INIT\n"
		    "    return 0;\n"
		    "    AROS_USERFUNC_EXIT\n"
		    "}\n",
		    modulename
	    );
	
	funclistit = funclist;
	funclistit2 = funclistit->next;
	fprintf(out,
		"\n"
		"const APTR %s_functable[]=\n"
		"{\n"
		"    &AROS_SLIB_ENTRY(%s,%s),\n"
		"    &AROS_SLIB_ENTRY(%s,%s),\n",
		modulename,
		funclistit->name, basename,
		funclistit2->name, basename
	);
	funclistit = funclistit2->next;

	if (funclistit->lvo == 3)
	{
	    fprintf(out, "    &AROS_SLIB_ENTRY(%s,%s),\n", funclistit->name, basename);
	    funclistit = funclistit->next;
	}
	else
	    fprintf(out, "    &%s_null,\n", modulename);
	
	if (funclistit->lvo == 4)
	{
	    fprintf(out, "    &AROS_SLIB_ENTRY(%s,%s),\n", funclistit->name, basename);
	    funclistit = funclistit->next;
	}
	else
	    fprintf(out, "    &%s_null,\n", modulename);
    }
    
    lvo = 4;
    while (funclistit != NULL)
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
	
	funclistit = funclistit->next;
    }

    fprintf(out, "    (void *)-1\n};\n");
    fclose(out);
}
