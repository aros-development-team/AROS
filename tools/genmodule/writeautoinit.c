/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write module_autoinit.c. Part of genmodule.
*/
#include "genmodule.h"

void writeautoinit(void)
{
    FILE *out;
    char line[256];
    
    snprintf(line, 255, "%s/%s_autoinit.c", gendir, modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could not write %s\n", line);
	exit(20);
    }
    fprintf(out,
	    "/*\n"
	    "    *** Automatically generated file. Do not edit ***\n"
	    "    Copyright © 1995-2003, The AROS Development Team. All rights reserved.\n"
	    "*/\n"
	    "#include <proto/%s.h>\n"
	    "#include <aros/symbolsets.h>\n"
	    "\n"
	    "ADD2LIBS(\"%s.library\",%u, %s*, %s);\n",
	    modulename, modulename, majorversion, libbasetypeextern, libbase);
    if (forcelist!=NULL)
    {
	struct forcelist * forcelistit;
	
	fprintf(out, "\n");
	for (forcelistit = forcelist;
	     forcelistit!=NULL;
	     forcelistit = forcelistit->next
	    )
	{
	    fprintf(out, "extern struct Library *%s;\n", forcelistit->basename);
	}
	fprintf(out, "\nvoid __%s_forcelibs(void)\n{\n", modulename);
	for (forcelistit = forcelist;
	     forcelistit!=NULL;
	     forcelistit = forcelistit->next
	    )
	{
	    fprintf(out, "    %s = NULL;\n", forcelistit->basename);
	}
	fprintf(out, "}\n");
    }
    fclose(out);
}
