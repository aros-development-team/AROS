/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
    
    Main for genmodule. A tool to generate files for building modules.
*/
#include "genmodule.h"

int main(int argc, char **argv)
{
    char *s;

    struct config *cfg = initconfig(argc, argv, NORMAL);

    readconfig(cfg);
    readref(cfg);
    if (cfg->modtype == LIBRARY || cfg->modtype == DEVICE)
    {
        writeincproto(cfg);
        writeincclib(cfg);
        writeincdefines(cfg);
    }
    if (cfg->modtype == LIBRARY)
        writeautoinit(cfg);
    if (cfg->modtype == MCC || cfg->modtype == MUI || cfg->modtype == MCP)
    {
        writemccinit(cfg);
        writemccquery(cfg);
    }
    writestart(cfg);
    writeend(cfg);
    writestubs(cfg);
    
    return 0;
}
