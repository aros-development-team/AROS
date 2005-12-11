/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
    
    Support functions for MUI classes. Part of genmodule.
*/
#include "genmodule.h"
#include "boopsisupport.h"

void writemuiincludes(FILE *out)
{
    fprintf
    (
        out,
        "#include <proto/muimaster.h>\n"
        "\n"
    );
}

void writemccinit(FILE *out, int inclass, struct classinfo *cl)
{
    struct functionhead *methlistit;
    struct functionarg *arglistit;
    unsigned int lvo;
    
    fprintf
    (
        out,
        "/* Initialisation routines of a MUI class */\n"
        "/* =======================================*/\n"
        "\n"
    );
        
    if (cl->classdatatype == NULL)
	fprintf(out, "#   define %s_DATA_SIZE (0)\n", cl->basename);
    else
	fprintf
        (
	     out,
	     "#   define %s_DATA_SIZE (sizeof(%s))\n",
	     cl->basename, cl->classdatatype
	);
    
    writeboopsidispatcher(out, cl);
    
    fprintf
    (
        out,
        "\n"
        "\n"
        "/*** Library startup and shutdown *******************************************/\n"
        "AROS_SET_LIBFUNC(MCC_%s_Startup, LIBBASETYPE, LIBBASE)\n"
        "{\n"
        "    AROS_SET_LIBFUNC_INIT\n"
        "    \n",
        cl->basename
    );
    
    /* When classid is specified MakeClass will be used to make the class
     * otherwise MUI_CreateCustomClass. The former use is only needed for internal
     * muimaster use. Other use is deprecated.
     */
    if (cl->classid == NULL)
    {
	char *base, disp[256];
	
	/* Has the class a provided dispatcher function ? */
	if (cl->dispatcher == NULL)
	    snprintf(disp, 256, "%s_Dispatcher", cl->basename);
	else
	    strncpy(disp, cl->dispatcher, 256);

	/* Is this class the main class then pass the libbase to MUI_CreateCustomClass
	 * otherwise pass NULL
	 */
	if (!inclass)
	    base = "(struct Library *)LIBBASE";
	else
	    base = "NULL";

	if (cl->superclass != NULL)
	    fprintf
	    (
	        out,
	        "    %s_CLASSPTR_FIELD(LIBBASE) = MUI_CreateCustomClass(%s, %s, NULL, %s_DATA_SIZE, %s);\n",
	        cl->basename, base, cl->superclass, cl->basename, disp
	    );
	else if (cl->superclass_field != NULL)
	    fprintf
	    (
	        out,
	        "    %s_CLASSPTR_FIELD(LIBBASE) = MUI_CreateCustomClass(%s, NULL, LIBBASE->%s, %s_DATA_SIZE, %s);\n",
	        cl->basename, base, cl->superclass_field, cl->basename, disp
	    );
	else
	{
	    fprintf(out, "Internal error: both superclass and superclass_field are NULL\n");
	    exit(20);
	}
    }
    else
    {
	char disp[256];

	/* Has the class a provided dispatcher function ? */
	if (cl->dispatcher == NULL)
	    snprintf(disp, 256, "%s_Dispatcher", cl->basename);
	else
	    strncpy(disp, cl->dispatcher, 256);

	if (cl->superclass != NULL)
	    fprintf
	    (
	        out,
	        "    Class *superclass = MUI_GetClass(%s),\n",
	        cl->superclass
	    );
	else if (cl->superclass_field != NULL)
	    fprintf
	    (
	        out,
	        "    Class *superclass = LIBBASE->%s,\n",
	        cl->superclass_field
	    );
	else
	{
	    fprintf(stderr, "Internal error: both superclass and superclass_field are NULL\n");
	    exit(20);
	}
	
	fprintf
	(
	    out,
	    "          *cl = NULL;\n"
	    "\n"
	    "    if (superclass)\n"
	    "        cl = MakeClass(%s, NULL, superclass, %s_DATA_SIZE, 0);\n"   
	    "    if (cl)\n"
	    "        cl->cl_Dispatcher.h_Entry = %s;\n"
	    "    %s_CLASSPTR_FIELD(LIBBASE) = cl;\n",
	    cl->classid, cl->basename,
	    disp,
	    cl->basename
	);
    }

    fprintf
    (
        out,
        "    \n"
        "    return %s_CLASSPTR_FIELD(LIBBASE) != NULL;\n"
        "    \n"
        "    AROS_SET_LIBFUNC_EXIT\n"
        "}\n"
        "\n"
        "AROS_SET_LIBFUNC(MCC_%s_Shutdown, LIBBASETYPE, LIBBASE)\n"
        "{\n"
        "    AROS_SET_LIBFUNC_INIT\n"
        "    \n",
        cl->basename,
        cl->basename
    );
    if (cl->classid == NULL)
    {
	fprintf
	(
	     out,
	     "    MUI_DeleteCustomClass(%s_CLASSPTR_FIELD(LIBBASE));\n",
	     cl->basename
	);
    }
    else
    {
	fprintf
	(
	    out,
	    "    Class *cl = %s_CLASSPTR_FIELD(LIBBASE);\n"
	    "\n"
	    "    if (cl != NULL)\n"
	    "    {\n"
	    "        MUI_FreeClass(cl->cl_Super);\n"
	    "        FreeClass(cl);\n"
	    "    }\n",
	    cl->basename
	);
    }
    fprintf
    (
        out,
        "    return TRUE;\n"
        "    \n"
        "    AROS_SET_LIBFUNC_EXIT\n"
        "}\n"
        "\n"
        "ADD2INITCLASSES(MCC_%s_Startup, %d);\n"
        "ADD2EXPUNGECLASSES(MCC_%s_Shutdown, %d);\n",
        cl->basename, -cl->initpri,
        cl->basename, -cl->initpri
    );
}

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
        "AROS_LH1(IPTR, MCC_Query,\n"
        "         AROS_LHA(LONG, what, D0),\n"
        "         LIBBASETYPEPTR, LIBBASE, 5, %s\n"
        ")\n"
        "{\n"
        "    AROS_LIBFUNC_INIT\n"
        "\n"
        "    switch( what )\n"
        "    {\n",
        cfg->basename
    );
    
    switch(cfg->modtype)
    {
    case MCC:
    case MUI:
        fprintf(out, "        case MCC_CLASS:          return (IPTR)GM_CLASSPTR_FIELD(LIBBASE);\n");
        break;
    case MCP:
        fprintf(out, "        case MCC_PREFS_CLASS:    return (IPTR)GM_CLASSPTR_FIELD(LIBBASE);\n");
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
