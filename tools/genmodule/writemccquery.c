/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write modulename_mcc_query.c. Part of genmodule.
*/
#include <stdio.h>

#include "genmodule.h"

void writemccquery(FILE *out, struct config *cfg)
{
    fprintf
    (
        out,
        "\n"
        "/* MCC_Query function */\n"
        "/* ================== */\n"
        "\n"
        "#include <libraries/mui.h>\n"
        "#include <aros/libcall.h>\n"
        "\n"
        "#define MCC_CLASS       0\n"
        "#define MCC_PREFS_CLASS 1\n"
        "\n"
        "extern struct MUI_CustomClass *MCC;\n"
        "\n"
        "AROS_LH1(IPTR, MCC_Query,\n"
        "         AROS_LHA(LONG, what, D0),\n"
        "         struct Library *, %s, 5, %s\n"
        ")\n"
        "{\n"
        "    AROS_LIBFUNC_INIT\n"
        "\n"
        "    switch( what )\n"
        "    {\n",
        cfg->libbase, cfg->basename
    );
    
    switch(cfg->modtype)
    {
    case MCC:
    case MUI:
        fprintf(out, "        case MCC_CLASS:          return (IPTR)MCC;\n");
        break;
    case MCP:
        fprintf(out, "        case MCC_PREFS_CLASS:    return (IPTR)MCC;\n");
        break;
    }
    
    /* FIXME: handle MCC_PREFS_IMAGE somehow */
    /* FIXME: handle "ONLY_GLOBAL" ?? */
    
    fprintf
    (
        out,
        "    }\n"
        "\n"
        "    return 0;\n"
        "\n"
        "    AROS_LIBFUNC_EXIT\n"
        "}\n"
    );
}
