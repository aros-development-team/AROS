/*
    Copyright (C) 1995-2018, The AROS Development Team. All rights reserved.

    Function to write module stubs. Part of genmodule.

    Stubs are agregated differently into objects files based on STACK vs REG
    convention.

    Linker will always put a whole object file from archive into
    executable. If all stubs are in one object file, all stubs are present
    in executable, inflating its size, even if they are not used.

    In majority of cases REG call function stubs will be handled by inlines
    or defines while STACK call function stubs will be linked from linklib.

    Based on above paragraphs, putting stubs of STACK functions into separate
    object files will allow for linker to only include required function stubs.
*/

#include "genmodule.h"

static void writeheader(struct config *cfg, int is_rel, FILE *out);
static void writefuncstub(struct config *cfg, int is_rel, FILE *out, struct functionhead *funclistit);

void writestubs(struct config *cfg, int is_rel)
{
    FILE *out;
    char line[256];
    struct functionhead *funclistit;

    /* Build STACKCALL - each stub in separate object file */
    for (funclistit = cfg->funclist;
         funclistit!=NULL;
         funclistit = funclistit->next
    )
    {
        if (funclistit->lvo >= cfg->firstlvo && funclistit->libcall == STACK)
        {

            snprintf(line, 255, "%s/%s_%s_%sstub.c", cfg->libgendir, cfg->modulename, funclistit->name, is_rel ? "rel" : "");
            out = fopen(line, "w");

            if (out == NULL)
            {
                perror(line);
                exit(20);
            }

            writeheader(cfg, is_rel, out);
            writefuncstub(cfg, is_rel, out, funclistit);

            fclose(out);
        }
    }

    /* Build REGCALL - all stubs in one object file */
    snprintf(line, 255, "%s/%s_regcall_%sstubs.c", cfg->libgendir, cfg->modulename, is_rel ? "rel" : "");
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
        exit(20);
    }

    writeheader(cfg, is_rel, out);

    for (funclistit = cfg->funclist;
         funclistit!=NULL;
         funclistit = funclistit->next
    )
    {
        if (funclistit->lvo >= cfg->firstlvo && funclistit->libcall != STACK)
        {
            writefuncstub(cfg, is_rel, out, funclistit);
        }
    }
    fclose(out);

}

static void writeheader(struct config *cfg, int is_rel, FILE *out)
{
    struct stringlist *linelistit;
    char *banner;

    banner = getBanner(cfg);
    if (is_rel) {
        fprintf
        (
            out,
            "%s"
            "#ifdef  NOLIBINLINE\n"
            "#undef  NOLIBINLINE\n"
            "#endif  /* NOLIBINLINE */\n"
            "#ifdef  NOLIBDEFINES\n"
            "#undef  NOLIBDEFINES\n"
            "#endif  /* NOLIBDEFINES */\n"
            "#define NOLIBINLINE\n"
            "#define NOLIBDEFINES\n"
            "char *__aros_getoffsettable(void);\n"
            "#ifndef __%s_NOLIBBASE__\n"
            "/* Do not include the libbase */\n"
            "#define __%s_NOLIBBASE__\n"
            "#endif\n",
            banner, cfg->includenameupper, cfg->includenameupper
        );
    } else {
        fprintf
        (
            out,
            "%s"
            "#ifdef  NOLIBINLINE\n"
            "#undef  NOLIBINLINE\n"
            "#endif  /* NOLIBINLINE */\n"
            "#ifdef  NOLIBDEFINES\n"
            "#undef  NOLIBDEFINES\n"
            "#endif  /* NOLIBDEFINES */\n"
            "#define NOLIBINLINE\n"
            "#define NOLIBDEFINES\n"
            "/* Be sure that the libbases are included in the stubs file */\n"
            "#ifdef  __NOLIBBASE__\n"
            "#undef  __NOLIBBASE__\n"
            "#endif  /* __NOLIBBASE__ */\n"
            "#ifdef  __%s_NOLIBBASE__\n"
            "#undef  __%s_NOLIBBASE__\n"
            "#endif  /* __%s_NOLIBBASE__ */\n",
            banner, cfg->includenameupper, cfg->includenameupper, cfg->includenameupper
        );
    }
    freeBanner(banner);

    for (linelistit = cfg->stubprivatelines; linelistit!=NULL; linelistit = linelistit->next)
        fprintf(out, "%s\n", linelistit->s);

    if (!(cfg->options & OPTION_NOINCLUDES))
    {
        if (is_rel)
            fprintf(out, "#define __%s_RELLIBBASE__\n", cfg->includenameupper);
        fprintf(out, "#include <proto/%s.h>\n", cfg->includename);
    }
    else
    {
        fprintf(out,
            "#include <exec/types.h>\n"
            "#include <aros/system.h>\n\n"
        );

        for (linelistit = cfg->cdefprivatelines; linelistit!=NULL; linelistit = linelistit->next)
            fprintf(out, "%s\n", linelistit->s);

        fprintf(out,
            "\n"
            "%s__aros_getbase_%s(void);\n"
            "\n",
            cfg->libbasetypeptrextern, cfg->libbase
        );
    }

    fprintf
    (
        out,
        "#include <stddef.h>\n"
        "\n"
        "#include <aros/cpu.h>\n"
        "#include <aros/genmodule.h>\n"
        "#include <aros/libcall.h>\n"
        "#include <aros/symbolsets.h>\n"
        "\n"
    );
}

static void writefuncstub(struct config *cfg, int is_rel, FILE *out, struct functionhead *funclistit)
{
    struct stringlist *aliasesit;
    struct functionarg *arglistit;
    int argtypes[funclistit->argcount];

    if (funclistit->libcall != STACK)
    {
        int count;
        int isvoid = strcmp(funclistit->type, "void") == 0
            || strcmp(funclistit->type, "VOID") == 0;

        fprintf(out,
                "\n"
                "%s %s(",
                funclistit->type, funclistit->name
        );
        for (arglistit = funclistit->arguments, count = 1;
             arglistit!=NULL;
             arglistit = arglistit->next, count++
        )
        {
            if (arglistit != funclistit->arguments)
                fprintf(out, ", ");
            fprintf(out, "%s", arglistit->arg);
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
                ")\n"
                "{\n"
        );

        fprintf(out,
                "    %sAROS_LC",
                (isvoid) ? "" : "return ");

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
                "%s(%s, %s,\\\n",
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
                        "         AROS_LCA2(%s, %s, %s, %s), \\\n",
                        arglistit->type, arglistit->name, arglistit->reg, quad2+1
                        );
                *quad2 = '/';
            } else {
                fprintf(out,
                        "         AROS_LCA(%s, %s, %s), \\\n",
                        arglistit->type, arglistit->name, arglistit->reg
                        );
            }
        }

        fprintf(out, "                    %s, __aros_getbase_%s(), %u, %s);\n}\n",
                cfg->libbasetypeptrextern, cfg->libbase, funclistit->lvo, cfg->basename
        );
    }
    else /* libcall==STACK */
    {
        /* Library version requirement added only for stack call functions
         * since each function is in separate compilation unit. Reg call
         * functions are all in one compilation unit. In such case highest
         * version would always be required even if function for that version
         * is not used.
         */
        fprintf(out, "void __%s_%s_libreq(){ AROS_LIBREQ(%s,%d); }\n",
                funclistit->name, cfg->libbase, cfg->libbase, funclistit->version
        );

        fprintf(out, "AROS_GM_%sLIBFUNCSTUB(%s, %s, %d)\n",
                is_rel ? "REL" : "",
                funclistit->name, cfg->libbase, funclistit->lvo
        );
    }

    for (aliasesit = funclistit->aliases;
         aliasesit != NULL;
         aliasesit = aliasesit->next
    )
    {
        fprintf(out, "AROS_GM_LIBFUNCALIAS(%s, %s)\n",
                funclistit->name, aliasesit->s
        );
    }
}
