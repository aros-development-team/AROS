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
    struct config *cfg = initconfig(argc, argv);

    if (cfg->command == NORMAL)
	readref(cfg, functions);
    
    if (cfg->command == DUMMY
	||
	(
	    cfg->command == NORMAL
	    &&
	    (
	        cfg->modtype == LIBRARY
		|| cfg->modtype == DEVICE
		|| cfg->modtype == RESOURCE
		|| cfg->modtype == GADGET
		|| cfg->modtype == DATATYPE
	    )
	)
    )
    {
        writeincproto(cfg);
        writeincclib(cfg, functions);
        writeincdefines(cfg, functions);
    }

    if (cfg->command == NORMAL)
    {
	if (cfg->modtype == LIBRARY)
	    writeautoinit(cfg);
	writestart(cfg, functions);
	writeend(cfg);
	writestubs(cfg, functions);
    }

    if (cfg->command == LIBDEFS)
	writeinclibdefs(cfg);
    
    return 0;
}
