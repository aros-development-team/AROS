/*
    Copyright © 2005-2011, The AROS Development Team. All rights reserved.
    $Id$
    
    Code to write a Makefile with variables that provides the files
    and configuration for building the module
*/
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "genmodule.h"
#include "config.h"

void writemakefile(struct config *cfg)
{
    FILE *out;
    char name[512];
    
    snprintf(name, sizeof(name), "%s/Makefile.%s", cfg->gendir, cfg->modulename);
    
    out = fopen(name, "w");
    
    if (out == NULL)
    {
	perror(name);
	exit(20);
    }

    fprintf(out,
	    "%s_STARTFILES += %s_start\n"
	    "%s_ENDFILES += %s_end\n"
	    "%s_MODDIR += %s\n",
	    cfg->modulename, cfg->modulename,
	    cfg->modulename, cfg->modulename,
	    cfg->modulename, cfg->moddir
    );

    fprintf(out, "%s_LINKLIBFILES +=", cfg->modulename);
    if (cfg->options & OPTION_STUBS)
        fprintf(out, " %s_stubs", cfg->modulename);
    if (cfg->options & OPTION_AUTOINIT)
        fprintf(out, " %s_autoinit", cfg->modulename);
    if (cfg->modtype == LIBRARY)
        fprintf(out, " %s_getlibbase", cfg->modulename);
    fprintf(out, "\n");
    fprintf(out, "%s_RELLINKLIBFILES +=", cfg->modulename);
    if (cfg->options & OPTION_STUBS)
        fprintf(out, " %s_relstubs", cfg->modulename);
    if (cfg->options & OPTION_AUTOINIT)
        fprintf(out, " %s_relautoinit", cfg->modulename);
    if (cfg->modtype == LIBRARY)
        fprintf(out, " %s_relgetlibbase", cfg->modulename);
    fprintf(out, "\n");

    /* Currently there are no asm files anymore */
    fprintf(out, "%s_LINKLIBAFILES +=\n", cfg->modulename);
    fprintf(out, "%s_RELLINKLIBAFILES +=\n", cfg->modulename);

    fprintf(out, "%s_INCLUDES += ", cfg->modulename);
    if (cfg->options & OPTION_INCLUDES)
    {
	fprintf(out,
		"clib/%s_protos.h inline/%s.h defines/%s.h proto/%s.h",
		cfg->modulename, cfg->modulename, cfg->modulename, cfg->modulename
	);
        if (cfg->modtype == LIBRARY)
            fprintf(out, " proto/%s_rel.h", cfg->modulename);
    }
    if (cfg->interfacelist)
    {
        struct interfaceinfo *in;
        for (in = cfg->interfacelist; in; in = in->next)
            fprintf(out,
                    " interface/%s.h"
                    , in->interfacename
            );
    }
    fprintf(out, "\n");

    fprintf(out, "%s_CFLAGS  +=", cfg->modulename);
    fprintf(out, "\n");

    fprintf(out, "%s_DFLAGS  +=", cfg->modulename);
    fprintf(out, "\n");

    fprintf(out, "%s_LDFLAGS +=", cfg->modulename);
    fprintf(out,"\n");

    if (ferror(out))
    {
	perror("Error writing Makefile");
	fclose(out);
	exit(20);
    }
    
    fclose(out);
}
