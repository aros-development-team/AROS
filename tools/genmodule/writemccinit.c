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
    struct functionlist *methlistit;
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
        
    for(linelistit = cdeflines; linelistit != NULL; linelistit = linelistit->next)
    {
        fprintf(out, "%s\n", linelistit->line);
    }
    
    if (datastruct != NULL)
    {
        struct linelist *line = NULL;
        
        fprintf
        (
            out,
            "\n"
            "/*** Instance data structure ********************************************/\n"
            "struct %s_DATA\n"
            "{\n",
            modulename
        );
        
        for (line = datastruct; line != NULL; line = line->next)
        {
            fprintf(out, "    %s\n", line->line);
        }
        
        fprintf(out, "};\n");
    }
    
    fprintf
    (
        out,
        "\n"
        "\n"
        "/*** Variables **************************************************************/\n"
        "struct ExecBase        *SysBase;\n"
        "struct Library         *MUIMasterBase;\n"
        "\n"
        "struct MUI_CustomClass *MCC;\n"
        "\n"
        "\n"
        "/*** Prototypes *************************************************************/\n"
    );
    
    for 
    (
        methlistit = methlist; 
        methlistit != NULL; 
        methlistit = methlistit->next)
    {
        int first = 1;
        
        fprintf(out, "%s %s$%s(", methlistit->type, modulename, methlistit->name);
        
        for 
        (
            arglistit = methlistit->arguments; 
            arglistit != NULL; 
            arglistit = arglistit->next)
        {
            if (!first)
                fprintf(out, ", ");
            else
                first = 0;
            
            fprintf(out, "%s %s", arglistit->type, arglistit->name );
        }
        
        fprintf(out, ");\n");
    }
    
    fprintf
    (
        out,
        "\n"
        "\n"
        "/*** Dispatcher *************************************************************/\n"
        "BOOPSI_DISPATCHER( IPTR, %s_Dispatcher, CLASS, self, message )\n"
        "{\n"
        "    switch( message->MethodID )\n"
        "    {\n",
        modulename
    );
    
    for 
    (
        methlistit = methlist; 
        methlistit != NULL; 
        methlistit = methlistit->next)
    {
        fprintf
        (
            out, 
            "        case %s: return %s$%s( ", 
            methlistit->name, modulename, methlistit->name
        );
        
        if (methlistit->argcount != 3)
        {
            fprintf(stderr, "Method \"%s\" has wrong number of arguments\n", methlistit->name);
            exit(20);
        }
        
        arglistit = methlistit->arguments;
        fprintf(out, "CLASS, ");
        arglistit = arglistit->next;
        fprintf(out, "self, ");
        arglistit = arglistit->next;
        fprintf(out, "(%s) message);\n", arglistit->type);
    }
    
    fprintf
    (
        out,
        "        default: return DoSuperMethodA( CLASS, self, message );\n"
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
        "    MCC = MUI_CreateCustomClass( LIBBASE, \"%s\", NULL, ",
        superclass
    );
    
    if (datastruct == NULL)
        fprintf(out, "0");
    else
        fprintf(out, "sizeof(struct %s_DATA)", modulename);
    
    fprintf
    (
        out,
        ", %s_Dispatcher );\n"
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
        modulename            
    );
    
    fclose(out);
}
