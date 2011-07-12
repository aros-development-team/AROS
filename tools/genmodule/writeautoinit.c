/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write module_autoinit.c. Part of genmodule.
*/
#include "genmodule.h"

void writeautostub(struct config *cfg, int version)
{
    FILE *out;
    char line[256], *banner;

    snprintf(line, 255, "%s/%s_autoinit_%d.c", cfg->gendir, cfg->modulename, version);
    out = fopen(line, "w");

    if (out==NULL)
    {
        perror(line);
    	exit(20);
    }

    banner = getBanner(cfg);
    fprintf(out,
        "%s"
	    "\n"
	    "#include <proto/%s.h>\n"
	    "#include <aros/symbolsets.h>\n"
	    "\n"
	    "AROS_LIBREQUEST(\"%s.%s\", %d, %s, %s)\n",
	    banner, cfg->modulename,
	    cfg->modulename, cfg->suffix, version,
	    cfg->libbasetypeptrextern, cfg->libbase
    );
    freeBanner(banner);

    fclose(out);
}

void writeautoinit(struct config *cfg)
{
    FILE *out;
    char line[256], *banner;
    struct stringlist *linelistit;
    int minversion = 0;
    struct functionhead *funclistit;

    writeautostub(cfg, 0);
    for (funclistit = cfg->funclist; funclistit; funclistit = funclistit->next) {
        if (funclistit->version > minversion) {
            minversion = funclistit->version;
            writeautostub(cfg, minversion);
        }
    }
    
    snprintf(line, 255, "%s/%s_autoinit.c", cfg->gendir, cfg->modulename);
    out = fopen(line, "w");

    if (out==NULL)
    {
        perror(line);
    	exit(20);
    }

    /* Write the code to be added to startup provided in the config file */
    for (linelistit = cfg->startuplines; linelistit != NULL; linelistit = linelistit->next)
    {
        fprintf(out, "%s\n", linelistit->s);
    }

    banner = getBanner(cfg);
    fprintf(out,
        "%s"
	    "\n"
	    "#include <proto/%s.h>\n"
	    "#include <aros/symbolsets.h>\n"
	    "\n"
 	    "ADD2LIBS(\"%s.%s\", %d, %s, %s)\n",
	    banner, cfg->modulename,
	    cfg->modulename, cfg->suffix, 0,
	    cfg->libbasetypeptrextern, cfg->libbase
    );
    freeBanner(banner);

    fprintf(out,
	    "AROS_IMPORT_ASM_SYM(int, dummy, __includelibrarieshandling);\n"
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
