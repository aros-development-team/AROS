/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.

    Desc: Function to write proto/modulename(_rel).h. Part of genmodule.
*/
#include "genmodule.h"

void writeincproto(struct config *cfg, int is_rel)
{
    FILE *out;
    char line[256], define[256], *banner;
    struct linelist *linelistit;
    
    snprintf(line, 255, "%s/proto/%s%s.h",
             cfg->gendir, cfg->modulename, is_rel ? "_rel" : ""
    );
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
    	exit(20);
    }

    banner = getBanner(cfg);
    fprintf(out,
	    "#ifndef PROTO_%s%s_H\n"
	    "#define PROTO_%s%s_H\n"
	    "\n"
            "%s"
	    "\n"
	    , cfg->modulenameupper, is_rel ? "_REL" : ""
            , cfg->modulenameupper, is_rel ? "_REL" : ""
            , banner
    );
    if (!is_rel)
        fprintf(out,
	    "#ifdef __%s_RELLIBBASE__\n"
	    "#include <proto/%s_rel.h>\n"
	    "#else /* !__%s_RELLIBBASE__ */\n"
	    , cfg->modulenameupper
	    , cfg->modulename
	    , cfg->modulenameupper
        );
    fprintf(out,
	    "#include <exec/types.h>\n"
	    "#include <aros/system.h>\n"
	    "\n"
	    "#include <clib/%s_protos.h>\n"
	    "\n",
	    cfg->modulename
    );
    freeBanner(banner);
    if (!is_rel)
    {
        fprintf(out,
                "#if !defined(%s) && !defined(__NOLIBBASE__) && !defined(__%s_NOLIBBASE__)\n"
                " #ifdef __%s_STDLIBBASE__\n"
                "  extern struct Library *%s;\n"
                " #else\n"
                "  extern %s%s;\n"
                " #endif\n"
                "#endif\n"
                "\n",
                cfg->libbase, cfg->modulenameupper,
                cfg->modulenameupper,
                cfg->libbase,
                cfg->libbasetypeptrextern, cfg->libbase
        );
    }
    else /* is_rel */
    {
        fprintf(out,
                "#if !defined(%s) && !defined(__NOLIBBASE__) && !defined(__%s_NOLIBBASE__)\n"
                "#ifndef __aros_getbase\n"
                " void *__aros_getbase(void);\n"
                "#endif\n"
                " extern const IPTR __aros_rellib_offset_%s;\n"
                "#define AROS_RELLIB_BASE_%s __aros_rellib_base_%s\n"
                "#define AROS_RELLIB_OFFSET_%s __aros_rellib_offset_%s\n"
                "#endif\n"
                "\n",
                cfg->libbase, cfg->modulenameupper,
                cfg->libbase,
                cfg->modulenameupper, cfg->libbase,
                cfg->modulenameupper, cfg->libbase
        );
    }

    // define name must not start with a digit
    // this solves a problem with proto/8svx.h
    if (isdigit(cfg->modulenameupper[0]))
    {
	snprintf(define, sizeof define, "X%s", cfg->modulenameupper);
    }
    else
    {
	strncpy(define, cfg->modulenameupper, sizeof define);
    }

    if (!is_rel)
    {
        fprintf(out,
                "#if !defined(NOLIBINLINE) && !defined(%s_NOLIBINLINE)\n"
                "#   include <inline/%s%s.h>\n",
                define, cfg->modulename
	    , (is_rel) ? "_rel" : ""
        );
    }

    fprintf(out,
	    "#%sif !defined(NOLIBDEFINES) && !defined(%s_NOLIBDEFINES)\n"
	    "#   include <defines/%s%s.h>\n"
	    "#endif\n"
	    "\n",
            (is_rel) ? "" : "el",
	    define, cfg->modulename
	    , (is_rel) ? "_rel" : ""
    );
    if (!is_rel)
        fprintf(out,
            "#endif /* !__%s_RELLIBBASE__ */\n"
	    , cfg->modulenameupper
	);
    fprintf(out,
	    "#endif /* PROTO_%s_H */\n",
            cfg->modulenameupper
    );
    fclose(out);
}
