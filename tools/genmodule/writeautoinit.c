/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: function to write module_autoinit.c. Part of genmodule.
*/
#include "genmodule.h"

void writeautoinit(void)
{
    FILE *out;
    
    snprintf(line, slen-1, "%s/%s_autoinit.c", gendir, modulename);
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
	    "#include <proto/%s.h>\n"
	    "#include <aros/symbolsets.h>\n"
	    "\n"
	    "ADD2LIBS(\"%s.library\",%u, LIBSET_USER_PRI, %s*, %s, NULL, NULL);\n",
	    modulename, modulename, majorversion, libbasetypeextern, libbase);
    fclose(out);
}
