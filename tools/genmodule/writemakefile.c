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

    if (!(cfg->intcfg & CFG_GENLINKLIB))
	fprintf(out,
		"%s_LINKLIBFILES :=\n"
		"%s_LINKLIBAFILES :=\n",
		cfg->modulename,
		cfg->modulename
	);
    else
    {
	switch (cfg->modtype)
	{
	case LIBRARY:
	    fprintf(out,
		    "%s_LINKLIBFILES := %s_stubs %s_autoinit\n",
		    cfg->modulename, cfg->modulename, cfg->modulename
	    );
	    break;
	
	case DEVICE:
	case RESOURCE:
	    fprintf(out,
		    "%s_LINKLIBFILES := %s_stubs\n",
		    cfg->modulename, cfg->modulename
	    );
	    break;
	
	default:
	    fprintf(stderr, "Internal error in writemakefile: unsupported modtype for genlinklib\n");
	    exit(20);
	}
	
	fprintf(out, "%s_LINKLIBAFILES :=", cfg->modulename);
	if (cfg->intcfg & CFG_GENASTUBS)
	    fprintf(out, "%s_astubs\n", cfg->modulename);
	else
	    fprintf(out, "\n");
    }

    fprintf(out, "%s_INCLUDES := ", cfg->modulename);
    switch (cfg->modtype)
    {
    case LIBRARY:
    case DEVICE:
    case RESOURCE:
    case GADGET:
	fprintf(out,
		"clib/%s_protos.h defines/%s.h proto/%s.h\n",
		cfg->modulename, cfg->modulename, cfg->modulename
	);
	break;
	
    case DATATYPE:
    case MCC:
    case MUI:
    case MCP:
	fprintf(out, "\n");
	break;
	
    default:
	fprintf(out, "Internal error writemakefile: unhandled modtype for includes\n");
	break;
    }
    
    if (ferror(out))
    {
	perror("Error writing Makefile");
	fclose(out);
	exit(20);
    }
    
    fclose(out);
}
