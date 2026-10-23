/*
    Copyright (C) 2005-2026, The AROS Development Team. All rights reserved.

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

    if (cl->classdatatype == NULL)
        fprintf(out, "#   define %s_DATA_SIZE (0)\n", cl->basename);
    else
        fprintf
        (
             out,
             "#   define %s_DATA_SIZE (sizeof(%s))\n",
             cl->basename, cl->classdatatype
        );

    /* Write defines of methods */
    writefuncdefs(out, NULL, cl->methlist);

    fprintf
    (
        out,
        "\n"
        "\n"
        "/*** Library startup and shutdown *******************************************/\n"
        "static int OOP_%s_Startup(LIBBASETYPEPTR LIBBASE)\n"
        "{\n"
        "#ifdef GM_OOPBASE_FIELD\n"
        "    struct Library *OOPBase = GM_OOPBASE_FIELD(LIBBASE);\n"
        "#endif\n"
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
            fprintf(out, "    static const struct OOP_MethodDescr %s_%s_descr[] =\n    {\n",
                    cl->basename, methlistit->interface->s
            );
            methods = 1;
            interface = methlistit->interface;
        }
        else
            methods++;

        fprintf(out, "        {(OOP_MethodFunc)%s, %s},\n",
                methlistit->internalname, methlistit->method
        );
    }
    if (methods) {
        /* Close the last declaration */
        fprintf(out,
                "        {NULL, 0}\n"
                "    };\n"
                "#define NUM_%s_%s_METHODS %d\n"
                "\n",
                cl->basename, interface->s, methods
        );
    }

    /* Write the interface description */
    fprintf(out, "    static const struct OOP_InterfaceDescr %s_ifdescr[] =\n    {\n", cl->basename);
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

    /* Write the class description; the creation itself is done by
       _set_oop_addclass() in libautoinit */
    fprintf(out,
            "    static const struct __aros_oopclass %s_class =\n"
            "    {\n"
            "        %s_ifdescr,\n",
            cl->basename,
            cl->basename
    );
    if (cl->superclass != NULL)
        fprintf(out, "        %s,\n", cl->superclass);
    else if (cl->superclass_field != NULL)
        fprintf(out, "        NULL,\n");
    else
    {
        fprintf(stderr, "Internal error: both superclass and superclass_field are NULL\n");
        exit(20);
    }
    fprintf(out,
            "        %s,\n"
            "        %s_DATA_SIZE\n"
            "    };\n"
            "\n",
            (cl->classid != NULL) ? cl->classid : "NULL",
            cl->basename
    );

    /* Write class constructor */
    fprintf(out,
            "    return _set_oop_addclass(&%s_class, ",
            cl->basename
    );
    if (cl->superclass != NULL)
        fprintf(out, "NULL");
    else
        fprintf(out, "(OOP_Class *)LIBBASE->%s", cl->superclass_field);
    fprintf(out,
            ",\n"
            "                             (APTR)LIBBASE, (OOP_Class **)&%s_CLASSPTR_FIELD(LIBBASE), OOPBase);\n"
            "}\n",
            cl->basename
    );

    /* Write class destructor */
    fprintf
    (
        out,
        "static void OOP_%s_Shutdown(LIBBASETYPEPTR LIBBASE)\n"
        "{\n"
        "#ifdef GM_OOPBASE_FIELD\n"
        "    struct Library *OOPBase = GM_OOPBASE_FIELD(LIBBASE);\n"
        "#endif\n"
        "    _set_oop_removeclass((OOP_Class *)%s_CLASSPTR_FIELD(LIBBASE), OOPBase);\n"
        "}\n",
        cl->basename,
        cl->basename
    );

    fprintf
    (
        out,
        "ADD2INITCLASSES(OOP_%s_Startup, %d)\n"
        "ADD2EXPUNGECLASSES(OOP_%s_Shutdown, %d)\n",
        cl->basename, -cl->initpri,
        cl->basename, -cl->initpri
    );
}
