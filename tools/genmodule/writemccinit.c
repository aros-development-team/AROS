/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write modulename_mcc_init.c. Part of genmodule.
*/
#include "genmodule.h"
#include "boopsisupport.h"

void writemccinit(FILE *out, struct config *cfg, struct functions *functions)
{
    struct functionhead *methlistit;
    struct functionarg *arglistit;
    struct stringlist *linelistit;
    unsigned int lvo;
    
    fprintf
    (
        out,
        "/* Initialisation routines of a MUI class */\n"
        "/* =======================================*/\n"
        "\n"
        "#include <dos/dosextens.h>\n"
        "#include <aros/debug.h>\n"
        "\n"
        "#include <intuition/classes.h>\n"
        "#include <intuition/classusr.h>\n"
        "\n"
        "#include <proto/exec.h>\n"
        "#include <proto/utility.h>\n"
        "#include <proto/dos.h>\n"
        "#include <proto/graphics.h>\n"
        "#include <proto/intuition.h>\n"
        "#include <proto/muimaster.h>\n"
        "\n"
        "#include <aros/symbolsets.h>\n"
        "\n",
        cfg->modulename
    );
        
    for(linelistit = cfg->cdeflines; linelistit != NULL; linelistit = linelistit->next)
    {
        fprintf(out, "%s\n", linelistit->s);
    }

    if (cfg->classdatatype == NULL)
	fprintf(out, "#   define %s_DATA_SIZE (0)\n", cfg->basename);
    else
	fprintf
        (
	     out,
	     "#   define %s_DATA_SIZE (sizeof(%s))\n",
	     cfg->basename, cfg->classdatatype
	);
    
    writeboopsidispatcher(out, cfg, functions);
    
    fprintf
    (
        out,
        "\n"
        "\n"
        "/*** Library startup and shutdown *******************************************/\n"
        "AROS_SET_LIBFUNC(MCC_Startup, LIBBASETYPE, LIBBASE)\n"
        "{\n"
        "    AROS_SET_LIBFUNC_INIT\n"
        "    \n"
    );
    if (cfg->dispatcher == NULL)
	fprintf
	(
	    out,
	    "    GM_CLASSPTR_FIELD(LIBBASE) = MUI_CreateCustomClass((struct Library *) LIBBASE, %s, NULL, %s_DATA_SIZE, %s_Dispatcher);\n",
	    cfg->superclass, cfg->basename, cfg->basename
	);
    else
	fprintf
	(
	    out,
	    "    GM_CLASSPTR_FIELD(LIBBASE) = MUI_CreateCustomClass((struct Library *) LIBBASE, %s, NULL, %s_DATA_SIZE, %s);\n",
	    cfg->superclass, cfg->basename, cfg->dispatcher
	);
    fprintf
    (
        out,
        "    \n"
        "    return GM_CLASSPTR_FIELD(LIBBASE) != NULL;\n"
        "    \n"
        "    AROS_SET_LIBFUNC_EXIT\n"
        "}\n"
        "\n"
        "AROS_SET_LIBFUNC(MCC_Shutdown, LIBBASETYPE, LIBBASE)\n"
        "{\n"
        "    AROS_SET_LIBFUNC_INIT\n"
        "    \n"
        "    MUI_DeleteCustomClass(GM_CLASSPTR_FIELD(LIBBASE));\n"
        "    return TRUE;\n"
        "    \n"
        "    AROS_SET_LIBFUNC_EXIT\n"
        "}\n"
        "\n"
        "ADD2INITLIB(MCC_Startup, 0);\n"
        "ADD2EXPUNGELIB(MCC_Shutdown, 0);\n"
    );
}
