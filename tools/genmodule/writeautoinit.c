/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write module_autoinit.c. Part of genmodule.
*/
#include "genmodule.h"

void writeautoinit(struct config *cfg)
{
    FILE *out;
    char line[256];
    
    snprintf(line, 255, "%s/%s_autoinit.c", cfg->gendir, cfg->modulename);
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
	    "#include <proto/%s.h>\n"
	    "#include <aros/symbolsets.h>\n"
	    "\n"
	    "ADD2LIBS(\"%s.library\",%u, %s, %s);\n",
	    cfg->modulename,
	    cfg->modulename, cfg->majorversion, cfg->libbasetypeptrextern, cfg->libbase
    );
    if (cfg->forcelist!=NULL)
    {
	struct stringlist * forcelistit;
	
	fprintf(out, "\n");
	for (forcelistit = cfg->forcelist;
	     forcelistit!=NULL;
	     forcelistit = forcelistit->next
	    )
	{
	    fprintf(out, "extern struct Library *%s;\n", forcelistit->s);
	}
	fprintf(out, "\nvoid __%s_forcelibs(void)\n{\n", cfg->modulename);
	for (forcelistit = cfg->forcelist;
	     forcelistit!=NULL;
	     forcelistit = forcelistit->next
	    )
	{
	    fprintf(out, "    %s = NULL;\n", forcelistit->s);
	}
	fprintf(out, "}\n");
    }
    fclose(out);
}
