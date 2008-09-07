/*
    Copyright © 2005, The AROS Development Team. All rights reserved.
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
	    "%s_STARTFILES := %s_start\n"
	    "%s_ENDFILES := %s_end\n"
	    "%s_MODDIR := %s\n",
	    cfg->modulename, cfg->modulename,
	    cfg->modulename, cfg->modulename,
	    cfg->modulename, cfg->moddir
    );

    fprintf(out, "%s_LINKLIBFILES :=", cfg->modulename);
    if (cfg->options & OPTION_STUBS)
        fprintf(out, " %s_stubs", cfg->modulename);
    if (cfg->options & OPTION_AUTOINIT)
        fprintf(out, " %s_autoinit", cfg->modulename);
    fprintf(out, "\n");

    fprintf(out, "%s_LINKLIBAFILES :=", cfg->modulename);
    if ((cfg->options & OPTION_STUBS) && (cfg->intcfg & CFG_GENASTUBS))
        fprintf(out, "%s_astubs\n", cfg->modulename);
    fprintf(out, "\n");

    fprintf(out, "%s_INCLUDES := ", cfg->modulename);
    if (cfg->options & OPTION_INCLUDES)
    {
	fprintf(out,
		"clib/%s_protos.h defines/%s.h proto/%s.h",
		cfg->modulename, cfg->modulename, cfg->modulename
	);
    }
    fprintf(out, "\n");

    fprintf(out,
	    "%s_NEEDREF := %s\n",
	    cfg->modulename, (cfg->intcfg & CFG_NOREADREF) ? "no" : "yes"
    );
    
    if (ferror(out))
    {
	perror("Error writing Makefile");
	fclose(out);
	exit(20);
    }
    
    fclose(out);
}
