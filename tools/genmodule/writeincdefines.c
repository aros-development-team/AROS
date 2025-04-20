/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Function to write defines/modulename.h. Part of genmodule.
*/
#include "genmodule.h"

static void writedefineregister(FILE *, struct functionhead *, struct config *);
static void writedefinevararg(FILE *, struct functionhead *, struct config *, char *);
static void writealiases(FILE *, struct functionhead *, struct config *);

void writeincdefines(struct config *cfg)
{
    FILE *out;
    char line[256], *banner;
    struct functionhead *funclistit;

#if (1)
    snprintf(line, 255, "%s/defines/%s_LVO.h", cfg->gendir, cfg->includename);
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
        exit(20);
    }

    banner = getBanner(cfg);
    fprintf(out,
            "#ifndef DEFINES_LVO_%s_H\n"
            "#define DEFINES_LVO_%s_H\n"
            "\n"
            "%s"
            "\n"
            "/*\n"
            "    Desc: Function LVO's for %s\n"
            "*/\n"
            "\n",
            cfg->includenameupper, cfg->includenameupper, banner, cfg->modulename
    );

    for (funclistit = cfg->funclist; funclistit!=NULL; funclistit = funclistit->next)
    {
        if (!funclistit->priv)
        {
            fprintf(out, "#define LVO%s          %u\n", funclistit->name, funclistit->lvo);
        }
    }

    fprintf(out,
            "\n"
            "#endif /* DEFINES_LVO_%s_H*/\n",
            cfg->includenameupper
    );
    fclose(out);
#endif

    snprintf(line, 255, "%s/defines/%s.h", cfg->gendir, cfg->includename);
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
        exit(20);
    }

    fprintf(out,
            "#ifndef DEFINES_%s_H\n"
            "#define DEFINES_%s_H\n"
            "\n"
            "%s"
            "\n"
            "/*\n"
            "    Desc: Defines for %s\n"
            "*/\n"
            "\n"
            "#include <aros/libcall.h>\n"
            "#include <exec/types.h>\n"
            "#include <aros/symbolsets.h>\n"
            "#include <aros/preprocessor/variadic/cast2iptr.hpp>\n"
            "\n",
            cfg->includenameupper, cfg->includenameupper, banner, cfg->modulename
    );
    if ((cfg->options & OPTION_RELLINKLIB) || (cfg->options & OPTION_DUPBASE))
    {
        fprintf(out,
                "#if !defined(__%s_LIBBASE)\n"
                "#  if !defined(__NOLIBBASE__) && !defined(__%s_NOLIBBASE__)\n"
                "#    define __%s_LIBBASE __aros_getbase_%s()\n"
                "#  else\n"
                "#    define __%s_LIBBASE %s\n"
                "#  endif\n"
                "#endif\n"
                "\n",
                cfg->includenameupper, cfg->includenameupper,
                cfg->includenameupper, cfg->libbase,
                cfg->includenameupper, cfg->libbase
        );
    }
    else
        fprintf(out,
                "#if !defined(__%s_LIBBASE)\n"
                "#    define __%s_LIBBASE %s\n"
                "#endif\n"
                "\n",
                cfg->includenameupper,
                cfg->includenameupper, cfg->libbase
        );
    fprintf(out,
            "__BEGIN_DECLS\n"
            "\n"
    );
    freeBanner(banner);

    for (funclistit = cfg->funclist; funclistit!=NULL; funclistit = funclistit->next)
    {
        if (!funclistit->priv && (funclistit->lvo >= cfg->firstlvo) && funclistit->libcall != STACK)
        {
            char *varargname = NULL, *lastname;

            fprintf(out,
                    "\n"
                    "#if !defined(__%s_LIBAPI__) || (%d <= __%s_LIBAPI__)"
                    "\n",
                    cfg->includenameupper,
                    funclistit->version,
                    cfg->includenameupper
            );

            if ((!funclistit->novararg) && (funclistit->arguments))
            {
                struct functionarg *arglistit = funclistit->arguments;

                while (arglistit->next != NULL) arglistit = arglistit->next;

                lastname = arglistit->name;
                assert(lastname != NULL);

                if (*(funclistit->name + strlen(funclistit->name) - 1) == 'A')
                {
                    funclistit->varargtype = 1;
                    varargname = strdup(funclistit->name);
                    varargname[strlen(funclistit->name)-1] = '\0';
                    if (arglistit && strncmp(arglistit->arg, "RAWARG",6) == 0)
                        funclistit->varargtype = 3;
                }
                else if (strcmp(funclistit->name + strlen(funclistit->name) - 7, "TagList") == 0)
                {
                    funclistit->varargtype = 1;
                    /* TagList has to be changed to Tags at the end of the functionname */
                    varargname = strdup(funclistit->name);
                    varargname[strlen(funclistit->name)-4] = 's';
                    varargname[strlen(funclistit->name)-3] = '\0';
                }
                else if (strcmp(funclistit->name + strlen(funclistit->name) - 4, "Args") == 0
                         && (strcasecmp(lastname, "args") == 0 || strcasecmp(lastname, "arglist") == 0)
                )
                {
                    funclistit->varargtype = 1;
                    varargname = strdup(funclistit->name);
                    varargname[strlen(funclistit->name)-4] = '\0';
                }
                else if ((funclistit->name[0] == 'V') &&  (strncmp(arglistit->arg, "va_list", 7) == 0))
                {
                    funclistit->varargtype = 2;
                    varargname = malloc(strlen(funclistit->name));
                    strcpy(varargname, &funclistit->name[1]);
                }
                else if ((funclistit->name[0] == 'V') &&  (strncmp(arglistit->arg, "RAWARG", 6) == 0))
                {
                    funclistit->varargtype = 3;
                    varargname = malloc(strlen(funclistit->name));
                    strcpy(varargname, &funclistit->name[1]);
                }
                else
                {
                    char *p;

                    if (strncmp(arglistit->arg, "const", 5) == 0) {
                        p = arglistit->arg + 5;
                        while (isspace(*p)) p++;
                    } else
                        p = arglistit->arg;
                    if (strncmp(p, "struct", 6)==0)
                    {
                        p += 6;
                        while (isspace(*p)) p++;
                        if (strncmp(p, "TagItem", 7) == 0)
                        {
                            p += 7;
                            while (isspace(*p)) p++;

                            if (*p == '*')
                            {
                                funclistit->varargtype = 1;
                                varargname = malloc(strlen(funclistit->name) + 5);
                                strcpy(varargname, funclistit->name);
                                strcat(varargname, "Tags");
                            }
                        }
                    }
                }
            }

            writedefineregister(out, funclistit, cfg);
            if (!funclistit->novararg && funclistit->varargtype)
            {
                writedefinevararg(out, funclistit, cfg, varargname);
                free(varargname);
            }
            writealiases(out, funclistit, cfg);

            fprintf(out,
                    "\n"
                    "#endif /* !defined(__%s_LIBAPI__) || (%d <= __%s_LIBAPI__) */"
                    "\n",
                    cfg->includenameupper,
                    funclistit->version,
                    cfg->includenameupper
            );

        }
    }
    fprintf(out,
            "\n"
            "__END_DECLS\n"
            "\n"
            "#endif /* DEFINES_%s_H*/\n",
            cfg->includenameupper
    );
    fclose(out);
}

void
writedefineregister(FILE *out, struct functionhead *funclistit, struct config *cfg)
{
    struct functionarg *arglistit;
    int count, isvoid;
    int argtypes[funclistit->argcount];

    isvoid = strcmp(funclistit->type, "void") == 0
        || strcmp(funclistit->type, "VOID") == 0;

    fprintf(out,
            "\n"
            "#define __%s_WB(__%s",
            funclistit->name, cfg->libbase
    );
    for (arglistit = funclistit->arguments, count = 1;
         arglistit!=NULL;
         arglistit = arglistit->next, count++
    )
    {
        fprintf(out, ", __arg%d", count);
        if (strchr(arglistit->reg, '/') != NULL) {
            if (strcmp(arglistit->type, "double") == 0) {
                argtypes[count-1] = TYPE_DOUBLE;
            } else {
                argtypes[count-1] = TYPE_QUAD;
            }
        } else {
            argtypes[count-1]  = TYPE_NORMAL;
        }
    }
    fprintf(out,
                    ") ({\\\n"
    );
    fprintf(out,
                "        AROS_LIBREQ(%s,%d)\\\n",
                cfg->libbase, funclistit->version
    );
    fprintf(out,"        AROS_LC");
    if (funclistit->argcount == 0) {
        fprintf(out, "0");
    } else {
        int consecutive_args = 0;
        int current_argtype = -1;
        for (int i = 0; i < funclistit->argcount; ++i) {
            if (current_argtype == -1) {
                current_argtype = argtypes[i];
                consecutive_args = 1;
            } else if (argtypes[i] == current_argtype) {
                ++consecutive_args;
            } else {
                generate_argtype_name_part(out, current_argtype, consecutive_args);
                // Restart argument count for the new basic type
                current_argtype = argtypes[i];
                consecutive_args = 1;
            }
        }
        generate_argtype_name_part(out, current_argtype, consecutive_args);
    }
    fprintf(out,
            "%s%s(%s, %s,\\\n",
            funclistit->unusedlibbase ? "I" : "",
            (isvoid) ? "NR" : "",
            funclistit->type, funclistit->name
            );

    for (arglistit = funclistit->arguments, count = 1;
         arglistit != NULL;
         arglistit = arglistit->next, count++
         ) {
        char *quad2 = strchr(arglistit->reg, '/');

        arglistit->reg[2] = 0;
        assert(arglistit->type != NULL);

        if (quad2 != NULL) {
            *quad2 = '\0';
            fprintf(out,
                    "         AROS_LCA2(%s, (__arg%d), %s, %s), \\\n",
                    arglistit->type, count, arglistit->reg, quad2+1
                    );
            *quad2 = '/';
        } else {
            fprintf(out,
                    "         AROS_LCA(%s, (__arg%d), %s), \\\n",
                    arglistit->type, count, arglistit->reg
                    );
        }
    }

    fprintf(out,
            "        %s, (__%s), %u, %s);\\\n})\n\n",
            cfg->libbasetypeptrextern, cfg->libbase,    funclistit->lvo, cfg->basename
    );

    fprintf(out, "#define %s(", funclistit->name);
    for (arglistit = funclistit->arguments, count = 1;
         arglistit != NULL;
         arglistit = arglistit->next, count++
    )
    {
        if (arglistit != funclistit->arguments)
            fprintf(out, ", ");
        fprintf(out, "arg%d", count);
    }
    fprintf(out, ") \\\n    __%s_WB(__%s_LIBBASE",
            funclistit->name, cfg->includenameupper
    );
    for (arglistit = funclistit->arguments, count = 1;
         arglistit != NULL;
         arglistit = arglistit->next, count++
    )
        fprintf(out, ", (arg%d)", count);
    fprintf(out, ")\n");
}

void
writedefinevararg(FILE *out, struct functionhead *funclistit, struct config *cfg, char *varargname)
{
    struct functionarg *arglistit = funclistit->arguments;
    int isvoid;

    isvoid = strcmp(funclistit->type, "void") == 0
        || strcmp(funclistit->type, "VOID") == 0;

    if (funclistit->varargtype == 1)
    {
        int count;
        char *type;

        fprintf(out,
                "\n#if !defined(NO_INLINE_STDARG) && !defined(%s_NO_INLINE_STDARG)\n"
                "#define %s(",
                cfg->includenameupper, varargname
        );
        for (arglistit = funclistit->arguments, count = 1;
             arglistit != NULL && arglistit->next != NULL;
             arglistit = arglistit->next, count++
        )
        {
            fprintf(out, "arg%d, ", count);
        }
        fprintf(out,
                "...) \\\n"
                "({ \\\n"
        );
        for (arglistit = funclistit->arguments, count = 1;
             arglistit != NULL;
             arglistit = arglistit->next, count++
        )
        {
            if (arglistit->next == NULL)
            {
                fprintf(out, "    const IPTR %s_args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) };\\\n", funclistit->name);
            }
        }
        fprintf(out,
                "    %s(",
                funclistit->name
        );
        for (arglistit = funclistit->arguments, count = 1;
             arglistit != NULL;
             arglistit = arglistit->next, count++
        )
        {
            if (arglistit != funclistit->arguments)
                fprintf(out, ", ");

            if (arglistit->next == NULL)
            {
                assert(arglistit->type != NULL);
                fprintf(out, "(%s)(%s_args)", arglistit->type, funclistit->name);
            }
            else
                fprintf(out, "(arg%d)", count);
        }
        fprintf(out,
                "); \\\n"
                "})\n"
                "#endif /* !NO_INLINE_STDARG */\n"
        );
    }
    else if (funclistit->varargtype == 2)
    {
        int count;
        struct functionarg *lastarg;

        fprintf(out,
                "\n#if !defined(NO_INLINE_STDARG) && !defined(%s_NO_INLINE_STDARG)\n"
                "static inline %s __%s_WB(%s __%s",
                cfg->includenameupper,
                funclistit->type, varargname, cfg->libbasetypeptrextern, cfg->libbase
        );
        for (arglistit = funclistit->arguments;
             arglistit != NULL && arglistit->next != NULL;
             arglistit = arglistit->next
        )
        {
            fprintf(out, ", %s", arglistit->arg);
            lastarg = arglistit;
        }
        fprintf(out, ", ...)\n");

        fprintf(out,
                "{\n"
                "    %s retval;\n"
                "    va_list args;\n"
                "\n"
                "    va_start(args, %s);\n"
                "    retval = __%s_WB(__%s, ",
                funclistit->type,
                lastarg->name,
                funclistit->name, cfg->libbase
        );
        for (arglistit = funclistit->arguments;
             arglistit != NULL && arglistit->next != NULL;
             arglistit = arglistit->next
        )
        {
            fprintf(out, "%s, ",arglistit->name);
        }
        fprintf(out,
                "args);\n"
                "    va_end(args);\n"
                "    return retval;\n"
                "}\n"
                "#define %s(",
                varargname
        );
        for (arglistit = funclistit->arguments, count = 1;
             arglistit != NULL && arglistit->next != NULL && arglistit->next->next != NULL;
             arglistit = arglistit->next, count++
        )
        {
            fprintf(out, "arg%d, ", count);
        }
        fprintf(out,
                "...) __%s_WB(",
                varargname
        );
        fprintf(out, "(%s)__%s_LIBBASE, ",
                cfg->libbasetypeptrextern,
                cfg->includenameupper);
        for (arglistit = funclistit->arguments, count = 1;
             arglistit != NULL && arglistit->next != NULL && arglistit->next->next != NULL;
             arglistit = arglistit->next, count++
        )
        {
            fprintf(out, "(arg%d), ", count);
        }
        fprintf(out,
                "__VA_ARGS__)\n"
                "#endif /* !NO_INLINE_STDARG */\n"
        );
    }
    else if (funclistit->varargtype == 3)
    {
        int count;

        fprintf(out,
                "\n#if !defined(NO_INLINE_STDARG) && !defined(%s_NO_INLINE_STDARG)\n"
                "static inline %s __inline_%s_%s(%s __%s",
                cfg->includenameupper,
                funclistit->type, cfg->basename, varargname, cfg->libbasetypeptrextern, cfg->libbase
        );
        for (arglistit = funclistit->arguments, count = 0;
             arglistit != NULL && arglistit->next != NULL;
             arglistit = arglistit->next
        )
        {
            fprintf(out, ", %s __arg%d", arglistit->type, ++count);
        }
        fprintf(out, ", ...)\n");

        fprintf(out,"{\n");
        if (!isvoid)
            fprintf(out, "    %s retval;\n\n", funclistit->type);

        fprintf(out,
                "    AROS_SLOWSTACKFORMAT_PRE(__arg%d);\n"
                "    %s__%s_WB(__%s",
                count,
                isvoid ? "" : "retval = ",
                funclistit->name,
                cfg->libbase
        );
        for (arglistit = funclistit->arguments, count = 1;
             arglistit != NULL && arglistit->next != NULL;
             arglistit = arglistit->next, count++
        )
        {
            fprintf(out, ", __arg%d", count);
        }
        count--;
        fprintf(out,
                ", AROS_SLOWSTACKFORMAT_ARG(__arg%d));\n"
                "    AROS_SLOWSTACKFORMAT_POST(__arg%d);\n"
                "    return%s;\n"
                "}\n"
                "\n"
                "#define %s(",
                count,
                count,
                isvoid ? "" : " retval",
                varargname
        );
        for (arglistit = funclistit->arguments, count = 1;
             arglistit != NULL && arglistit->next != NULL && arglistit->next->next != NULL;
             arglistit = arglistit->next, count++
        )
        {
            fprintf(out, "arg%d, ", count);
        }
        fprintf(out,
                "...) \\\n"
                "    __inline_%s_%s(",
                cfg->basename, varargname
        );
        fprintf(out, "(%s)__%s_LIBBASE, ",
                cfg->libbasetypeptrextern,
                cfg->includenameupper);
        for (arglistit = funclistit->arguments, count = 1;
             arglistit != NULL && arglistit->next != NULL && arglistit->next->next != NULL;
             arglistit = arglistit->next, count++
        )
        {
            fprintf(out, "(arg%d), ", count);
        }
        fprintf(out,
                "__VA_ARGS__)\n"
                "#endif /* !NO_INLINE_STDARG */\n"
        );
    }
}

void
writealiases(FILE *out, struct functionhead *funclistit, struct config *cfg)
{
    struct stringlist *aliasesit;

    for (aliasesit = funclistit->aliases;
         aliasesit != NULL;
         aliasesit = aliasesit->next
    )
    {
        fprintf(out, "#define %s %s\n", aliasesit->s, funclistit->name);
    }
}
