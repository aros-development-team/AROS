/*
 * Copyright (C) 2012, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * Generate include/interface/XXXX.h files
 */
#include "genmodule.h"

static void writeincinterface(struct config *cfg, struct interfaceinfo *in)
{
    FILE *out;
    char line[256], *banner;
    struct functionhead *funclistit;
    int maxlvo = -1;

    snprintf(line, 255, "%s/interface/%s.h", cfg->gendir, in->interfacename);
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
        exit(20);
    }

    banner = getBanner(cfg);
    fprintf(out,
            "#ifndef INTERFACE_%s_H\n"
            "#define INTERFACE_%s_H\n"
            "\n"
        "%s"
            "\n"
        "/*\n"
            "    Desc: interface inlines for %s\n"
            "*/\n"
            "\n"
            "#include <exec/types.h>\n"
            "#include <proto/oop.h>\n"
            "\n"
            "#define IID_%-32s \"%s\"\n"
            "\n"
            , in->interfacename
            , in->interfacename
            , banner
            , in->interfacename
            , in->interfacename
            , in->interfaceid
    );
    freeBanner(banner);
    
    /* Emit the get-the-methodbase stub */
    fprintf(out,
            "#if !defined(%s) && !defined(__OOP_NOMETHODBASES__) && !defined(__%s_NOMETHODBASE__)\n"
            "#define %s %s_GetMethodBase(__obj)\n"
            "\n"
            "static inline OOP_MethodID %s_GetMethodBase(OOP_Object *obj)\n"
            "{\n"
            "    static OOP_MethodID %s_mid;\n"
            "    if (!%s_mid) {\n"
            "        struct Library *OOPBase = (struct Library *)OOP_OCLASS(obj)->OOPBasePtr;\n"
            "        %s_mid = OOP_GetMethodID(IID_%s, 0);\n"
            "    }\n"
            "    return %s_mid;\n"
            "}\n"
            "#endif\n"
            "\n"
            ,in->methodbase
            ,in->interfacename
            ,in->methodbase
            ,in->interfacename
            ,in->interfacename
            ,in->interfacename
            ,in->interfacename
            ,in->interfacename
            ,in->interfacename
            ,in->interfacename
    );

    if (in->attributelist) {
        fprintf(out,
                "#define %-32s __I%s\n"
                "\n"
                "#if !defined(__OOP_NOATTRBASES__) && !defined(__%s_NOATTRBASE__)\n"
                "extern OOP_AttrBase %s;\n"
                "#endif\n"
                "\n"
                "enum\n"
                "{\n"
                , in->attributebase
                , in->interfacename
                , in->interfacename
                , in->attributebase
        );
    }

    /* Define all the attribute ids */
    for (funclistit = in->attributelist; funclistit!=NULL; funclistit = funclistit->next)
    {
        fprintf(out,
                "    ao%s_%s = %d,"
                , in->interfacename
                , funclistit->name
                , funclistit->lvo
        );
        if (funclistit->comment)
        {
            fprintf(out,
                    "  /* %s */"
                    , funclistit->comment
            );
        }
        fprintf(out, "\n");
        if ((int)funclistit->lvo > maxlvo)
            maxlvo = funclistit->lvo;
    }

    if (maxlvo >= 0) {
        fprintf(out,
                "    num_%s_Attrs = %d,\n"
                "};\n"
                "\n"
                , in->interfacename
                , maxlvo+1
        );
    }

    for (funclistit = in->attributelist; funclistit!=NULL; funclistit = funclistit->next)
    {
        fprintf(out,
                "#define a%s_%-32s (%s + ao%s_%s)\n"
                , in->interfacename
                , funclistit->name
                , in->attributebase
                , in->interfacename
                , funclistit->name
        );
    }

    fprintf(out,
            "\n"
            "#define %s_Switch(attr, idx) \\\n"
            "if (((idx) = (attr) - %s) < num_%s_Attrs) \\\n"
            "switch (idx)\n"
            "\n"
            , in->interfacename
            , in->attributebase
            , in->interfacename
    );
    
    if (!in->methodlist)
        goto done;

    /* Define method ids */
    fprintf(out,
            "\n"
            "enum {\n"
    );
    maxlvo = -1;
    for (funclistit = in->methodlist; funclistit!=NULL; funclistit = funclistit->next)
    {
        fprintf(out,
                "    mo%s_%s = %d,\n"
                , in->interfacename
                , funclistit->name
                , funclistit->lvo
        );
        if ((int)funclistit->lvo > maxlvo)
            maxlvo = funclistit->lvo;
    }

    fprintf(out,
            "    num_%s_Methods = %d\n"
            "};\n"
            "\n"
            , in->interfacename
            , maxlvo+1
    );

    /* Define method stubs */
    for (funclistit = in->methodlist; funclistit!=NULL; funclistit = funclistit->next)
    {
        struct functionarg *arg;

        /* Define message types */
        fprintf(out,
                "struct p%s_%s\n"
                "{\n"
                "    OOP_MethodID mID;\n"
                , in->interfacename
                , funclistit->name
        );

        for (arg = funclistit->arguments; arg; arg = arg->next)
        {
            fprintf(out,
                    "    %s;\n"
                    , arg->arg
            );
        }

        fprintf(out,
                "};\n"
                "\n"
        );

        if (funclistit->comment)
        {
            fprintf(out,
                    "/* %s */\n"
                    , funclistit->comment
            );
        }

        fprintf(out,
                "#define %s_%s(obj, args...) \\\n"
                "    ({OOP_Object *__obj = obj;\\\n"
                "      %s_%s_(%s, __obj ,##args); })\n"
                "\n"
                ,in->methodstub
                ,funclistit->name
                ,in->methodstub
                ,funclistit->name
                ,in->methodbase
        );

        fprintf(out,
                "static inline %s %s_%s_(OOP_MethodID __%s, OOP_Object *__obj"
                , funclistit->type
                , in->methodstub
                , funclistit->name
                , in->methodbase
        );

        for (arg = funclistit->arguments; arg; arg = arg->next)
        {
            fprintf(out,
                    ", %s"
                    , arg->arg
            );
        }

        fprintf(out,
                ")\n"
                "{\n"
                "    struct p%s_%s p;\n"
                "    p.mID = __%s + mo%s_%s;\n"
                , in->interfacename
                , funclistit->name
                , in->methodbase
                , in->interfacename
                , funclistit->name
        );

        for (arg = funclistit->arguments; arg; arg = arg->next)
        {
            const char *ptr;
            char *name;

            ptr = arg->arg + strlen(arg->arg) - 1;
            while (isspace(*ptr))
                ptr--;

            while (isalnum(*ptr) || (*ptr == '_'))
                ptr--;

            if (*ptr == ')') {
                /* Function prototype */
                const char *cp;
                ptr = strchr(arg->arg, '(');
                while (!(isalnum(*ptr) || (*ptr == '_'))) ptr++;
                cp = ptr;
                while (isalnum(*ptr) || (*ptr == '_')) ptr++;
                name = malloc(ptr - cp + 1);
                memcpy(name, cp, ptr - cp);
                name[ptr-cp] = 0;
            } else {
                name = strdup(ptr + 1);
            }

            fprintf(out,
                    "    p.%s = %s;\n"
                    , name
                    , name
            );

            free(name);
        }

        fprintf(out,
                "    %s(%s)OOP_DoMethod(__obj, &p.mID);\n"
                "}\n"
                "\n"
                , (strcasecmp(funclistit->type, "void") == 0) ? "" : "return "
                , funclistit->type
        );
    }

done:
    fprintf(out,
            "#endif /* INTERFACE_%s_H */\n"
            , in->interfacename
    );

    fclose(out);
}
 
void writeincinterfaces(struct config *cfg)
{
    struct interfaceinfo *in;

    for (in = cfg->interfacelist; in ; in = in->next) {
        writeincinterface(cfg, in);
    }
}
