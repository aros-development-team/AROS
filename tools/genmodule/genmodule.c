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
    struct config *cfg = initconfig(argc, argv, NORMAL);

    readref(cfg, functions);
    if (cfg->modtype == LIBRARY || cfg->modtype == DEVICE || cfg->modtype == RESOURCE)
    {
        writeincproto(cfg);
        writeincclib(cfg, functions);
        writeincdefines(cfg, functions);
    }
    if (cfg->modtype == LIBRARY)
        writeautoinit(cfg);
    writestart(cfg, functions);
    writeend(cfg);
    writestubs(cfg, functions);
    
    return 0;
}
