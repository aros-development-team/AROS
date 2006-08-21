/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
    
    Support function for generating code for BOOPSI classes. Part of genmodule.
*/
#include "config.h"
#include "functionhead.h"
#include "boopsisupport.h"

void writeboopsiincludes(FILE *out)
{
    fprintf
    (
        out,
        "#include <intuition/classes.h>\n"
        "#include <intuition/classusr.h>\n"
        "\n"
        "#include <proto/utility.h>\n"
        "#include <proto/intuition.h>\n"
        "\n"
        "#include <aros/symbolsets.h>\n"
        "\n"
    );
}

void writeboopsidispatcher(FILE *out, struct classinfo *cl)
{
    struct functionhead *methlistit;
    struct functionarg *arglistit;
    struct stringlist *aliasit;
    int i;

    if (cl->dispatcher == NULL)
    {
	fprintf
	(
	    out,
	    "\n"
            "\n"
            "/*** Prototypes *************************************************************/\n"
	);

	writefuncdefs(out, NULL, cl->methlist);

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
            cl->basename
        );
        
        for 
        (
            methlistit = cl->methlist; 
            methlistit != NULL; 
            methlistit = methlistit->next
	)
        {
	    char *type;
	    
            fprintf(out, "        ");
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
	    fprintf(out,"%s(", methlistit->name);
            
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
            "BOOPSI_DISPATCHER_PROTO(IPTR, %s, CLASS, object, message);\n",
            cl->dispatcher
        );
    }
}

void writeclassinit(FILE *out, struct classinfo *cl)
{
    struct functionhead *methlistit;
    struct functionarg *arglistit;
    unsigned int lvo;
    
    fprintf
    (
        out,
        "/* Initialisation routines for a BOOPSI class */\n"
        "/* ===========================================*/\n"
        "\n"
    );
        
    writeboopsidispatcher(out, cl);

    if (cl->classdatatype == NULL)
	fprintf(out, "#define %s_DATA_SIZE (0)\n", cl->basename);
    else
	fprintf(out,
		"#define %s_DATA_SIZE (sizeof(%s))\n",
		cl->basename, cl->classdatatype
	);
    
    fprintf
    (
        out,
        "\n"
        "\n"
        "/*** Library startup and shutdown *******************************************/\n"
        "static int BOOPSI_%s_Startup(LIBBASETYPEPTR LIBBASE)\n"
        "{\n"
        "    struct IClass *cl = NULL;\n"
        "    \n",
        cl->basename
    );
    if (cl->superclass != NULL)
	fprintf(out,
		"    cl = MakeClass(%s, %s, NULL, %s_DATA_SIZE, 0);\n",
		cl->classid, cl->superclass, cl->basename
	);
    else if (cl->superclass_field != NULL)
	fprintf(out,
		"    cl = MakeClass(%s, NULL, LIBBASE->%s, %s_DATA_SIZE, 0);\n",
		cl->classid, cl->superclass_field, cl->basename
	);
    else
    {
	fprintf(stderr, "Internal error: both superclass and superclass_field are NULL\n");
	exit(20);
    }
    fprintf
    (
        out,
        "    if (cl != NULL)\n"
        "    {\n"
        "#if %s_STORE_CLASSPTR\n"
        "        %s_CLASSPTR_FIELD(LIBBASE) = cl;\n"
        "#endif\n",
        cl->basename,
        cl->basename
    );

    if (cl->dispatcher == NULL)
	fprintf(out,
		"        cl->cl_Dispatcher.h_Entry = (APTR)%s_Dispatcher;\n",
		cl->basename
	);
    else
	fprintf(out,
		"        cl->cl_Dispatcher.h_Entry = (APTR)%s;\n",
		cl->dispatcher
	);

    fprintf
    (
        out,
	"        cl->cl_Dispatcher.h_SubEntry = NULL;\n"
        "        cl->cl_UserData = (IPTR)LIBBASE\n;"
    );
    
    if (!(cl->options & COPTION_PRIVATE))
	fprintf
	(
	    out,
	    "\n"
	    "        AddClass(cl);\n"
	);
    
    fprintf
    (
        out,
        "\n"
        "        return TRUE;\n"
        "    }\n"
        "    else\n"
        "        return FALSE;\n"
        "}\n"
        "\n"
        "static void BOOPSI_%s_Shutdown(LIBBASETYPEPTR LIBBASE)\n"
        "{\n"
        "    struct IClass *cl = %s_CLASSPTR_FIELD(LIBBASE);\n"
        "    \n"
        "    if (cl != NULL)\n"
        "    {\n",
        cl->basename, cl->basename
    );
    if (!(cl->options & COPTION_PRIVATE))
	fprintf(out, "        RemoveClass(cl);\n");
    fprintf
    (
        out,
        "        FreeClass(cl);\n"
        "#if %s_STORE_CLASSPTR\n"
        "        %s_CLASSPTR_FIELD(LIBBASE) = NULL;\n"
        "#endif\n"
        "    }\n"
//        "\n"
//        "    return TRUE;\n"
        "}\n"
        "\n"
        "ADD2INITCLASSES(BOOPSI_%s_Startup, %d);\n"
        "ADD2EXPUNGECLASSES(BOOPSI_%s_Shutdown, %d);\n",
        cl->basename,
        cl->basename,
        cl->basename, -cl->initpri,
        cl->basename, -cl->initpri
    );
}

