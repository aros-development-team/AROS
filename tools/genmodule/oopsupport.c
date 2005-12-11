/*
    Copyright © 2005, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: Support functions for oop.library classes. Part of genmodule.
*/
#include "genmodule.h"
#include "oopsupport.h"

void writeoopincludes(FILE *out)
{
    fprintf
    (
        out,
        "#include <proto/oop.h>\n"
        "#include <oop/oop.h>\n"
        "#include <hidd/hidd.h>\n"
        "\n"
    );
}

void writeoopinit(FILE *out, struct classinfo *cl)
{
    struct functionhead *methlistit;
    struct functionarg *arglistit;
    struct stringlist *interface;
    int methods;
    fprintf
    (
        out,
        "/* Initialisation routines of a OOP class */\n"
        "/* =======================================*/\n"
        "\n"
    );

    fprintf(out,
	    "#ifdef GM_OOPBASE_FIELD\n"
	    "#   ifdef OOPBase\n"
	    "#       undef OOPBase\n"
	    "#   endif\n"
	    "#   define OOPBase GM_OOPBASE_FIELD(LIBBASE)\n"
	    "#endif\n"
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
    
    /* Write prototypes of methods */
    writefuncprotos(out, NULL, cl->methlist);

    fprintf
    (
        out,
        "\n"
        "\n"
        "/*** Library startup and shutdown *******************************************/\n"
        "AROS_SET_LIBFUNC(OOP_%s_Startup, LIBBASETYPE, LIBBASE)\n"
        "{\n"
        "    AROS_SET_LIBFUNC_INIT\n"
        "\n"
        "    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);\n"
        "    OOP_Class *cl = NULL;\n"
        "\n",
        cl->basename
    );
     
    /* Write variables initialization */
    for (methlistit = cl->methlist, interface = NULL, methods = 0;
	 methlistit != NULL;
	 methlistit = methlistit->next
    )
    {
	if (interface != methlistit->interface)
	{
	    if (interface != NULL)
	    {
		/* Close the previous declaration */
		fprintf(out,
			"        {NULL, 0}\n"
			"    };\n"
			"#define NUM_%s_%s_METHODS %d\n"
			"\n",
			cl->basename, interface->s, methods
		);
	    }

	    /* Start new MethodDescr declaration */
	    fprintf(out, "    struct OOP_MethodDescr %s_%s_descr[] =\n    {\n",
		    cl->basename, methlistit->interface->s
	    );
	    methods = 1;
	    interface = methlistit->interface;
	}
	else
	    methods++;

	fprintf(out, "        {(OOP_MethodFunc)%s, %s},\n",
		methlistit->name, methlistit->method
	);
    }
    /* Close the last declaration */
    fprintf(out,
	    "        {NULL, 0}\n"
	    "    };\n"
	    "#define NUM_%s_%s_METHODS %d\n"
	    "\n",
	    cl->basename, interface->s, methods
    );

    /* Write the interface description */
    fprintf(out, "    struct OOP_InterfaceDescr %s_ifdescr[] =\n    {\n", cl->basename);
    for (interface = cl->interfaces; interface != NULL; interface = interface->next)
    {
	fprintf(out,
		"        {%s_%s_descr, IID_%s, NUM_%s_%s_METHODS},\n",
		cl->basename, interface->s,
		interface->s,
		cl->basename, interface->s
	);
    }
    fprintf(out,
	    "        {NULL, NULL}\n"
	    "    };\n"
	    "\n"
    );
    
    /* Write the class creation TagList */
    fprintf(out,
	    "    struct TagItem %s_tags[] =\n"
	    "    {\n",
	    cl->basename
    );
    if (cl->superclass != NULL)
	fprintf(out,
		"        {aMeta_SuperID, (IPTR)%s},\n",
		cl->superclass
	);
    else if (cl->superclass_field != NULL)
	fprintf(out,
		"        {aMeta_SuperPtr, (IPTR)LIBBASE->%s},\n",
		cl->superclass_field
	);
    else
    {
	fprintf(stderr, "Internal error: both superclass and superclass_field are NULL\n");
	exit(20);
    }
	
    fprintf(out,
	    "        {aMeta_InterfaceDescr, (IPTR)%s_ifdescr},\n"
	    "        {aMeta_InstSize, (IPTR)%s_DATA_SIZE},\n",
	    cl->basename,
	    cl->basename
    );
    if (cl->classid != NULL)
	fprintf(out,
		"        {aMeta_ID, (IPTR)%s},\n",
		cl->classid
	);
    fprintf(out, "        {TAG_DONE, (IPTR)0}\n    };\n");

    /* Write class constructor */
    fprintf
    (
        out,
        "\n"
        "    if (MetaAttrBase == 0)\n"
        "        return FALSE;\n"
        "\n"
        "    cl = OOP_NewObject(NULL, CLID_HiddMeta, %s_tags);\n"
        "    if (cl != NULL)\n"
        "    {\n"
        "        cl->UserData = (APTR)LIBBASE;\n"
        "        %s_CLASSPTR_FIELD(LIBBASE) = cl;\n"
        "        OOP_AddClass(cl);\n"
        "    }\n"
        "\n"
        "    OOP_ReleaseAttrBase(IID_Meta);\n"
        "    return cl != NULL;\n"
        "\n"
        "    AROS_SET_LIBFUNC_EXIT\n"
        "}\n",
        cl->basename,
        cl->basename,
        cl->basename
    );

    /* Write class destructor */
    fprintf
    (
        out,
        "AROS_SET_LIBFUNC(OOP_%s_Shutdown, LIBBASETYPE, LIBBASE)\n"
        "{\n"
        "    AROS_SET_LIBFUNC_INIT\n"
        "\n"
        "    if (%s_CLASSPTR_FIELD(LIBBASE) != NULL)\n"
        "    {\n"
        "        OOP_RemoveClass(%s_CLASSPTR_FIELD(LIBBASE));\n"
        "        OOP_DisposeObject((OOP_Object *)%s_CLASSPTR_FIELD(LIBBASE));\n"
        "    }\n"
        "\n"
        "    return TRUE;\n"
        "\n"
        "    AROS_SET_LIBFUNC_EXIT\n"
        "}\n",
        cl->basename,
        cl->basename,
        cl->basename,
        cl->basename
    );
    
    fprintf
    (
        out,
        "ADD2INITCLASSES(OOP_%s_Startup, %d);\n"
        "ADD2EXPUNGECLASSES(OOP_%s_Shutdown, %d);\n",
        cl->basename, -cl->initpri,
        cl->basename, -cl->initpri
    );
}
