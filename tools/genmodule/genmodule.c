/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
    
    Main for genmodule. A tool to generate files for building modules.
*/
#include "genmodule.h"

int main(int argc, char **argv)
{
    char *s;
    struct config *cfg = initconfig(argc, argv);

    switch (cfg->command)
    {
    case FILES:
	if (!(cfg->intcfg & CFG_NOREADREF))
	    readref(cfg);
	writestart(cfg);
	writeend(cfg);
	if (cfg->modtype == LIBRARY)
	    writeautoinit(cfg);
	writestubs(cfg);
	break;
	
    case INCLUDES:
	if (!(cfg->intcfg & CFG_NOREADREF))
	    readref(cfg);
	/* fall through */
    case DUMMY:
        writeincproto(cfg);
        writeincclib(cfg);
        writeincdefines(cfg);
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
	cfg->funclist = NULL;
	if (cfg->classlist != NULL)
	    cfg->classlist->methlist = NULL;

	readref(cfg);
	writefunclist(cfg);
	break;
	
    default:
	fprintf(stderr, "Internal error in main: Unhandled command type\n");
	exit(20);
    }
    
    return 0;
}
