/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
    
    Main for genmodule. A tool to generate files for building modules.
*/
#include "genmodule.h"

int main(int argc, char **argv)
{
    char *s;
    struct functions *functions = functionsinit();
    struct config *cfg = initconfig(argc, argv, functions);

    switch (cfg->command)
    {
    case FILES:
	if (!(cfg->intcfg & CFG_NOREADREF))
	    readref(cfg, functions);
	writestart(cfg, functions);
	writeend(cfg);
	if (cfg->modtype == LIBRARY)
	    writeautoinit(cfg);
	writestubs(cfg, functions);
	break;
	
    case INCLUDES:
	if (!(cfg->intcfg & CFG_NOREADREF))
	    readref(cfg, functions);
	/* fall through */
    case DUMMY:
        writeincproto(cfg);
        writeincclib(cfg, functions);
        writeincdefines(cfg, functions);
	break;
	
    case LIBDEFS:
	writeinclibdefs(cfg);
	break;

    case MAKEFILE:
	writemakefile(cfg);
	break;

    case WRITEFUNCLIST:
	/* Ignore the functionlist and the methodlist that are available in the
	 * .conf file.
	 */
	functions->funclist = NULL;
	functions->methlist = NULL;

	readref(cfg, functions);
	writefunclist(cfg, functions);
	break;
	
    default:
	fprintf(stderr, "Internal error in main: Unhandled command type\n");
	exit(20);
    }
    
    return 0;
}
