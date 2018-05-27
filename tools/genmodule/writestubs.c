/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

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
    char *type, *name;

    if (funclistit->libcall != STACK)
    {
        int nargs = 0, nquad = 0;
        int isvoid = strcmp(funclistit->type, "void") == 0
            || strcmp(funclistit->type, "VOID") == 0;

        fprintf(out,
                "\n"
                "%s %s(",
                funclistit->type, funclistit->name
        );
        for (arglistit = funclistit->arguments;
             arglistit!=NULL;
             arglistit = arglistit->next
        )
        {
            if (arglistit != funclistit->arguments)
                fprintf(out, ", ");
            fprintf(out, "%s", arglistit->arg);
            if (strchr(arglistit->reg, '/')) {
                nquad++;
            } else {
                nargs++;
            }
        }

        fprintf(out,
                ")\n"
                "{\n"
        );
        if (nquad == 0) {
            fprintf(out,
                    "    %sAROS_LC%d%s(%s, %s,\n",
                    (isvoid) ? "" : "return ",
                    funclistit->argcount, (isvoid) ? "NR" : "",
                    funclistit->type, funclistit->name
            );

            for (arglistit = funclistit->arguments;
                 arglistit!=NULL;
                 arglistit = arglistit->next
            )
            {
                type = getargtype(arglistit);
                name = getargname(arglistit);
                assert(type != NULL && name != NULL);

                fprintf(out, "                    AROS_LCA(%s,%s,%s),\n",
                        type, name, arglistit->reg
                );
                free(type);
                free(name);
            }
        }
        else /* nquad != 0 */
        {
            if (nargs == 0) {
                fprintf(out,
                        "    %sAROS_LCQUAD%d%s(%s, %s, \\\n",
                        (isvoid) ? "" : "return ",
                        funclistit->argcount, (isvoid) ? "NR" : "",
                        funclistit->type, funclistit->name
                );
            }
            else
            {
                fprintf(out,
                        "    %sAROS_LC%dQUAD%d%s(%s, %s, \\\n",
                        (isvoid) ? "" : "return ",
                        nargs, nquad, (isvoid) ? "NR" : "",
                        funclistit->type, funclistit->name
                );
            }

            for (arglistit = funclistit->arguments;
                 arglistit != NULL;
                 arglistit = arglistit->next
            )
            {
                char *quad2 = strchr(arglistit->reg, '/');

                type = getargtype(arglistit);
                name = getargname(arglistit);
                assert(type != NULL && name != NULL);

                if (quad2) {
                    *quad2 = 0;
                    fprintf(out,
                            "         AROS_LCAQUAD(%s, %s, %s, %s), \\\n",
                            type, name, arglistit->reg, quad2+1
                    );
                    *quad2 = '/';
                }
                else
                {
                    fprintf(out,
                            "         AROS_LCA(%s, %s, %s), \\\n",
                            type, name, arglistit->reg
                    );
                }
                free(type);
                free(name);
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
