/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write modulename_mcc_init.c. Part of genmodule.
*/
#include "genmodule.h"

void writemccinit(void)
{
    FILE *out;
    char line[256];
    struct functionlist *funclistit;
    struct arglist *arglistit;
    struct linelist *linelistit;
    unsigned int lvo;
    int i;
    
    snprintf(line, 255, "%s/%s_mcc_init.c", gendir, modulename);
    out = fopen(line, "w");
    if(out==NULL)
    {
        fprintf(stderr, "Could not write %s\n", line);
        exit(20);
    }
    
    fprintf
    (
        out,
        "/*\n"
        "    *** Automatically generated file. Do not edit ***\n"
        "    Copyright © 1995-2003, The AROS Development Team. All rights reserved.\n"
        "*/\n"
        "#include <libcore/libheader.c>\n"
    );
    
    for(linelistit = cdeflines; linelistit!=NULL; linelistit = linelistit->next)
    {
        fprintf(out, "%s\n", linelistit->line);
    }
    
    fclose(out);
}
