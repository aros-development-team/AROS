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

    fprintf
    (
        out,
        "\n"
        "/*** Instance data structure size ***************************************/\n"
        "#ifndef NO_CLASS_DATA\n"
        "#   define %s_DATA_SIZE (sizeof(struct %s_DATA))\n"
        "#else\n"
        "#   define %s_DATA_SIZE (0)\n"
        "#endif\n",
        cfg->basename, cfg->basename, cfg->basename
    );
    
    fprintf
    (
        out,
        "\n"
        "\n"
        "/*** Variables **************************************************************/\n"
        "struct MUI_CustomClass *MCC;\n"
        "\n"
        "\n"
        "/*** Prototypes *************************************************************/\n"
    );
    
    for 
    (
        methlistit = functions->methlist; 
        methlistit != NULL; 
        methlistit = methlistit->next)
    {
        int first = 1;
        
        fprintf(out, "%s %s__%s(", methlistit->type, cfg->basename, methlistit->name);
        
        for 
        (
            arglistit = methlistit->arguments; 
            arglistit != NULL; 
            arglistit = arglistit->next
	)
        {
            if (!first)
                fprintf(out, ", ");
            else
                first = 0;
            
            fprintf(out, "%s", arglistit->type);
        }
        
        fprintf(out, ");\n");
    }

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
        "    MCC = MUI_CreateCustomClass((struct Library *) LIBBASE, \"%s\", NULL, %s_DATA_SIZE, %s_Dispatcher);\n"
        "    \n"
        "    return MCC != NULL;\n"
        "    \n"
        "    AROS_SET_LIBFUNC_EXIT\n"
        "}\n"
        "\n"
        "AROS_SET_LIBFUNC(MCC_Shutdown, LIBBASETYPE, LIBBASE)\n"
        "{\n"
        "    AROS_SET_LIBFUNC_INIT\n"
        "    \n"
        "    MUI_DeleteCustomClass(MCC);\n"
        "    return TRUE;\n"
        "    \n"
        "    AROS_SET_LIBFUNC_EXIT\n"
        "}\n"
        "\n"
        "ADD2INITLIB(MCC_Startup, 0);\n"
        "ADD2EXPUNGELIB(MCC_Shutdown, 0);\n",
        cfg->superclass, cfg->basename, cfg->basename
    );
}
