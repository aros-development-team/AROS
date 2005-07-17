/*
    Copyright � 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
    
    Support function for generating code for BOOPSI classes. Part of genmodule.
*/
#include "config.h"
#include "functionhead.h"

void writeboopsidispatcher(FILE *out, struct config *cfg, struct functions *functions)
{
    struct functionhead *methlistit;
    struct functionarg *arglistit;
    struct stringlist *aliasit;
    int i;

    if (!cfg->customdispatcher)
    {
        fprintf
        (
            out,
            "\n"
            "\n"
            "/*** Dispatcher *************************************************************/\n"
            "BOOPSI_DISPATCHER(IPTR, %s_Dispatcher, CLASS, self, message)\n"
            "{\n"
            "    switch (message->MethodID)\n"
            "    {\n",
            cfg->basename
        );
        
        for 
        (
            methlistit = functions->methlist; 
            methlistit != NULL; 
            methlistit = methlistit->next
	)
        {
	    char *type;
	    
            fprintf(out, "        case %s: ", methlistit->name);
	    for
	    (
	        aliasit = methlistit->aliases;
	        aliasit != NULL;
	        aliasit = aliasit->next
	    )
	    {
		fprintf(out, "case %s: ", aliasit->s);
	    }
	    if (strcmp(methlistit->type, "void") != 0)
		fprintf(out, "return (IPTR)");
	    fprintf(out,"%s__%s(", cfg->basename, methlistit->name);
            
            if (methlistit->argcount != 3)
            {
                fprintf(stderr, "Method \"%s\" has wrong number of arguments\n", methlistit->name);
                exit(20);
            }
            
            arglistit = methlistit->arguments;
            fprintf(out, "CLASS, ");

            arglistit = arglistit->next;
	    type = getargtype(arglistit);
	    if (type == NULL)
	    {
		fprintf(stderr,
			"Argument \"%s\" not understood for function %s\n",
			arglistit->arg, methlistit->name
		);
		exit(20);
	    }
            fprintf(out, "(%s)self, ", type);
	    free(type);
	    
            arglistit = arglistit->next;
	    type = getargtype(arglistit);
	    if (type == NULL)
	    {
		fprintf(stderr,
			"Argument \"%s\" not understood for function %s\n",
			arglistit->arg, methlistit->name
		);
		exit(20);
	    }
	    fprintf(out, "(%s) message);", type);
	    free(type);
		
	    if (strcmp(methlistit->type, "void") == 0)
		fprintf(out, " break;");

	    fprintf(out, "\n");
        }
        
        fprintf
        (
            out,
            "        default: return DoSuperMethodA(CLASS, self, message);\n"
            "    }\n"
            "    \n"
            "    return (IPTR) NULL;\n"
            "}\n"
            "BOOPSI_DISPATCHER_END\n"
        );
    }
    else
    {
        fprintf
        (
            out,
            "\n"
            "\n"
            "/*** Custom dispatcher prototype ********************************************/\n"
            "BOOPSI_DISPATCHER_PROTO(IPTR, %s_Dispatcher, CLASS, object, message);\n",
            cfg->basename
        );
    }
}

void writeclassinit(FILE *out, struct config *cfg, struct functions *functions)
{
    struct functionhead *methlistit;
    struct functionarg *arglistit;
    struct stringlist *linelistit;
    unsigned int lvo;
    
    fprintf
    (
        out,
        "/* Initialisation routines for a BOOPSI class */\n"
        "/* ===========================================*/\n"
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
        "/*** Prototypes *************************************************************/\n"
    );
    
    for 
    (
        methlistit = functions->methlist; 
        methlistit != NULL; 
        methlistit = methlistit->next
    )
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
            
            fprintf(out, "%s", arglistit->arg);
        }
        
        fprintf(out, ");\n");
    }

    writeboopsidispatcher(out, cfg, functions);

    if (cfg->classdatatype == NULL)
	fprintf(out, "#define %s_DATA_SIZE (0)\n", cfg->basename);
    else
	fprintf(out,
		"#define %s_DATA_SIZE (sizeof(%s))\n",
		cfg->basename, cfg->classdatatype
	);
    
    fprintf
    (
        out,
        "\n"
        "\n"
        "/*** Library startup and shutdown *******************************************/\n"
        "AROS_SET_LIBFUNC(BOOPSI_Startup, LIBBASETYPE, LIBBASE)\n"
        "{\n"
        "    AROS_SET_LIBFUNC_INIT\n"
        "\n"
        "    struct IClass *cl = NULL;\n"
        "    \n"
        "    cl = MakeClass(\"%s\", %s, NULL, %s_DATA_SIZE, 0);\n"
        "    if (cl != NULL)\n"
        "    {\n"
        "        GM_CLASSPTR_FIELD(LIBBASE) = cl;\n"
        "        cl->cl_Dispatcher.h_Entry = (APTR)%s_Dispatcher;\n"
	"        cl->cl_Dispatcher.h_SubEntry = NULL;\n"
        "        cl->cl_UserData = (IPTR)LIBBASE\n;"
        "\n"
        "        AddClass(cl);\n"
        "\n"
        "        return TRUE;\n"
        "    }\n"
        "    else\n"
        "        return FALSE;\n"
        "    \n"
        "    AROS_SET_LIBFUNC_EXIT\n"
        "}\n"
        "\n"
        "AROS_SET_LIBFUNC(BOOPSI_Shutdown, LIBBASETYPE, LIBBASE)\n"
        "{\n"
        "    AROS_SET_LIBFUNC_INIT\n"
        "    \n"
        "    if (GM_CLASSPTR_FIELD(LIBBASE) != NULL)\n"
        "    {\n"
        "        RemoveClass(GM_CLASSPTR_FIELD(LIBBASE));\n"
        "        FreeClass(GM_CLASSPTR_FIELD(LIBBASE));\n"
        "        GM_CLASSPTR_FIELD(LIBBASE) = NULL;\n"
        "    }\n"
        "\n"
        "    return TRUE;\n"
        "\n"
        "    AROS_SET_LIBFUNC_EXIT\n"
        "}\n"
        "\n"
        "ADD2INITLIB(BOOPSI_Startup, 1);\n"
        "ADD2EXPUNGELIB(BOOPSI_Shutdown, 1);\n",
        cfg->classname, cfg->superclass, cfg->basename, cfg->basename
    );
}

