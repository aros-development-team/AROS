/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write modulename_mcc_query.c. Part of genmodule.
*/
#include "genmodule.h"

void writemccquery(struct config *cfg)
{
    FILE *out;
    char line[256];
    
    if(cfg->modtype != MCC && cfg->modtype != MUI && cfg->modtype != MCP)
    {
        fprintf(stderr, "Unsupported modtype %d\n", cfg->modtype);
        exit(20);
    }
    
    snprintf(line, 255, "%s/%s_mcc_query.c", cfg->gendir, cfg->modulename);
    out = fopen(line, "w");
    if(out == NULL)
    {
        fprintf(stderr, "Could not write %s\n", line);
        exit(20);
    }
    
    fprintf
    (
        out,
        "/*\n"
        "    *** Automatically generated file. Do not edit ***\n"
        "    Copyright © 1995-2004, The AROS Development Team. All rights reserved.\n"
        "*/\n"
        "\n"
        "#include <exec/types.h>\n"
        "#include <libraries/mui.h>\n"
        "\n"
        "#define MCC_CLASS       0\n"
        "#define MCC_PREFS_CLASS 1\n"
        "\n"
        "extern struct MUI_CustomClass *MCC;\n"
        "\n"
        "IPTR MCC_Query( LONG what )\n"
        "{\n"
        "    switch( what )\n"
        "    {\n"
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
        "}\n"
    );
    
    fclose(out);
}
