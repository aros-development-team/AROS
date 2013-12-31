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
                "extern %s%s;\n"
                "\n"
                "%s__aros_getbase_%s(void);\n"
                "\n"
                "%s__aros_getbase_%s(void)\n"
                "{\n"
                "    return %s;\n"
                "}\n",
                cfg->libbasetypeptrextern, cfg->libbase,
                cfg->libbasetypeptrextern, cfg->libbase,
                cfg->libbasetypeptrextern, cfg->libbase,
                cfg->libbase
        );
    }
    else /* is_rel */
    {
        fprintf(out,
                "#include <exec/types.h>\n"
                "char *__aros_getoffsettable(void);\n"
                "extern IPTR __aros_rellib_offset_%s;\n"
                "\n"
                "%s__aros_getbase_%s(void);\n"
                "\n"
                "%s__aros_getbase_%s(void)\n"
                "{\n"
                "    return *((%s*)(__aros_getoffsettable()+__aros_rellib_offset_%s));\n"
                "}\n",
                cfg->libbase,
                cfg->libbasetypeptrextern, cfg->libbase,
                cfg->libbasetypeptrextern, cfg->libbase,
                cfg->libbasetypeptrextern, cfg->libbase
         );
    }
    fclose(out);
}
