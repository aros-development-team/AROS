/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: function to write modulename_start.c. Part of genmodule.
*/
#include "genmodule.h"

void writestart(void)
{
    FILE *out;
    struct functionlist *funclistit;
    
    snprintf(line, slen-1, "%s/%s_start.c", gendir, modulename);
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
	    "#include <libcore/libheader.c>\n\n");
    for (funclistit = funclist;
	 funclistit != NULL;
	 funclistit = funclistit->next)
	fprintf(out, "int %s();\n", funclistit->name);

    fprintf(out,
	    "\n"
	    "const APTR %s_functable[]=\n"
	    "{\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader),\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader),\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader),\n"
	    "    &AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader),\n",
	    modulename);

    for (funclistit = funclist;
	 funclistit != NULL;
	 funclistit = funclistit->next)
	fprintf(out, "    &%s,\n", funclistit->name);

    fprintf(out, "    (void *)-1\n};\n");
    fclose(out);
}
