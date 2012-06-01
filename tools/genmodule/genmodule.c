/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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
	writestart(cfg);
	writeend(cfg);
	if (cfg->options & OPTION_AUTOINIT)
        {
	    writeautoinit(cfg, 0); /* normal */
	    writeautoinit(cfg, 1); /* relbase */
        }
        if (cfg->modtype == LIBRARY)
        {
            writegetlibbase(cfg, 0); /* normal */
            writegetlibbase(cfg, 1); /* relbase */
        }
        if (cfg->options & OPTION_STUBS)
        {
            writestubs(cfg, 0); /* normal */
            writestubs(cfg, 1); /* relbase */
        }
	break;
	
    case INCLUDES:
        if (!(cfg->options & OPTION_INCLUDES))
        {
            fprintf(stderr, "%s called with writeincludes when no includes are present\n", argv[0]);
            exit(20);
        }
        writeincproto(cfg, 0); /* normal */
        writeincproto(cfg, 1); /* relbase */
        writeincclib(cfg);
        writeincdefines(cfg);
        writeincinline(cfg);
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

	writefunclist(cfg);
	break;

    case WRITEFD:
	writefd(cfg);
	break;

    case WRITESKEL:
	writeskel(cfg);
	break;

    default:
	fprintf(stderr, "Internal error in main: Unhandled command type\n");
	exit(20);
    }
    
    return 0;
}
