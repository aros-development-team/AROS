/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write module_getlibbase.c. Part of genmodule.
*/

#include "genmodule.h"

void writegetlibbase(struct config *cfg, int is_rel)
{
    FILE *out;
    char line[256], *banner;

    snprintf(line, 255, "%s/%s_%sgetlibbase.c",
             cfg->gendir, cfg->modulename, is_rel ? "rel" : ""
    );
    out = fopen(line, "w");

    if (out==NULL)
    {
        perror(line);
        exit(20);
    }

    banner = getBanner(cfg);
    fprintf(out, "%s\n", banner);
    freeBanner(banner);

    if (!is_rel)
    {
        fprintf(out,
                "extern void *%s;\n"
                "\n"
                "void *%s_GetLibbase(void)\n"
                "{\n"
                "    return %s;\n"
                "}\n",
                cfg->libbase,
                cfg->basename,
                cfg->libbase
        );
    }
    else /* is_rel */
    {
        fprintf(out,
                "#include <exec/types.h>\n"
                "void *__GM_GetBase(void);\n"
                "extern IPTR %s_offset;\n"
                "\n"
                "void *%s_GetLibbase(void)\n"
                "{\n"
                "    return ((void *)((char *)__GM_GetBase()+%s_offset));\n"
                "}\n",
                cfg->libbase,
                cfg->basename,
                cfg->libbase
         );
    }
    fclose(out);
}
