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
        "    *** Automatically generated file. Do not edit! ***\n"
        "    Copyright © 1995-2003, The AROS Development Team. All rights reserved.\n"
        "*/\n"
        "\n"
        "#include <exec/types.h>\n"
        "#include <exec/libraries.h>\n"
        "#include <dos/dosextens.h>\n"
        "#include <aros/libcall.h>\n"
        "#include <aros/debug.h>\n"
        "#include <libcore/base.h>\n"
        "\n"
        "#include <intuition/classes.h>\n"
        "#include <intuition/classusr.h>\n"
        "\n"
        "#include <proto/exec.h>\n"
        "#include <proto/intuition.h>\n"
        "#include <proto/muimaster.h>\n"
        "\n"
        "#include <aros/symbolsets.h>\n"
        "#include LC_LIBDEFS_FILE\n"
        "\n"
    );
        
    for(linelistit = cdeflines; linelistit!=NULL; linelistit = linelistit->next)
    {
        fprintf(out, "%s\n", linelistit->line);
    }
      
    fprintf
    (
        out,
        "/*** Variables **************************************************************/\n"
        "struct ExecBase        *SysBase;\n"
        "struct Library         *MUIMasterBase;\n"
        "\n"
        "struct MUI_CustomClass *MCC;\n"
        "\n"
        "\n"
        "/*** Dispatcher *************************************************************/\n"
        "BOOPSI_DISPATCHER( IPTR, %s_Dispatcher, CLASS, self, message )\n"
        "{\n"
        "    switch( message->MethodID )\n"
        "    {\n",
        modulename
    );
    
    /* FIXME: Methods here... */
    
    /*case OM_NEW: return PreferencesWindow$OM_NEW( CLASS, self, (struct opSet *) message );*/
    
    fprintf
    (
        out,
        "        default:     return DoSuperMethodA( CLASS, self, message );\n"
        "    }\n"
        "    \n"
        "    return NULL;\n"
        "}\n"
        "\n"
        "\n"
        "/*** Library startup and shutdown *******************************************/\n"
        "AROS_SET_LIBFUNC( MCC_Startup, LIBBASETYPE, LIBBASE )\n"
        "{\n"
        "    SysBase = LIBBASE->lh_SysBase;\n"
        "    \n"
        "    MUIMasterBase = OpenLibrary( \"muimaster.library\", 0 );\n"
        "    if( MUIMasterBase == NULL ) return FALSE;\n"
        "    \n"
        "    MCC = MUI_CreateCustomClass( LIBBASE, \"%s\", NULL, 0, %s_Dispatcher );\n"
        "    if( MCC == NULL ) return FALSE;\n"
        "    \n"
        "    return TRUE;\n"
        "}\n"
        "\n"
        "AROS_SET_LIBFUNC( MCC_Shutdown, LIBBASETYPE, LIBBASE )\n"
        "{\n"
        "    CloseLibrary( MUIMasterBase );\n"
        "    \n"
        "    MUI_DeleteCustomClass( MCC );\n"
        "}\n"
        "\n"
        "ADD2INITLIB( MCC_Startup, 0 );\n"
        "ADD2EXPUNGELIB( MCC_Shutdown, 0 );\n",
        superclass, modulename            
    );
    
    fclose(out);
}
