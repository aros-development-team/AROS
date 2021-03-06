/*
    Copyright (C) 2015, The AROS Development Team. All rights reserved.

    The code for creating a thunk file for the functions present in the module
*/
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include "functionhead.h"
#include "config.h"

static void writethunkfunc(FILE *out, struct config *cfg, struct functionhead *funclist)
{
    struct functionarg *arglistit;
    char *type, *name;
    int is_void;
    
    is_void = (strcasecmp(funclist->type, "void") == 0);

    fprintf(out,
            "THUNK_PROTO%d(%s, %s, %s %s",
            funclist->argcount,
            funclist->type, funclist->internalname,
            cfg->libbasetypeptrextern, cfg->libbase);
    for (arglistit = funclist->arguments;
         arglistit!=NULL;
         arglistit = arglistit->next
    )
    {
        type = getargtype(arglistit);
        name = getargname(arglistit);
        assert(name != NULL && type != NULL);
        fprintf(out, ", %s %s", type, name);
        free(name);
        free(type);
    }
    fprintf(out, ");\n\n");

    fprintf(out,
            "AROS_LH%d(%s, %s,\n",
            funclist->argcount, funclist->type, funclist->internalname
    );

    for (arglistit = funclist->arguments;
         arglistit!=NULL;
         arglistit = arglistit->next
    )
    {
        type = getargtype(arglistit);
        name = getargname(arglistit);
        assert(name != NULL && type != NULL);

        fprintf(out,
                "        AROS_LHA(%s, %s, %s),\n",
                type, name, arglistit->reg
        );
        free(type);
        free(name);
    }

    fprintf(out,
            "        %s, %s, %u, %s)\n"
            "{\n"
            "    AROS_LIBFUNC_INIT\n\n",
            cfg->libbasetypeptrextern, cfg->libbase, funclist->lvo, cfg->basename
    );
    fprintf(out, "    ");
    fprintf(out, "THUNK_CALL%d(%s, %s, %s",
            funclist->argcount,
            is_void ? "" : "return ",
            funclist->internalname,
            cfg->libbase);

    for (arglistit = funclist->arguments;
         arglistit!=NULL;
         arglistit = arglistit->next
    )
    {
        name = getargname(arglistit);
        assert(name != NULL);
        fprintf(out, ", %s", name);
        free(name);
    }
    fprintf(out, ");\n\n");
    fprintf(out,
            "    AROS_LIBFUNC_EXIT\n"
            "}\n\n"
    );
}

void writethunk(struct config *cfg)
{
    struct functionhead *funclistit;
    FILE *out = NULL;
    char line[256];
    struct functionarg *arglistit;
    char *type, *name;
    uint64_t argmask = 0;
    int i;
    
    for(funclistit = cfg->funclist; funclistit != NULL; funclistit = funclistit->next)
    {
        argmask |= (1ULL << funclistit->argcount);
    }

    snprintf(line, 255, "%s/%s_thunk.c", cfg->gendir, cfg->modulename);
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
        exit(20);
    }

    for (i = 0; i < 64; i++) {
        if (argmask & (1ULL << i)) {
            int j;

            fprintf(out,
                    "#define THUNK_PROTO%d(type, name, arg_base", i);
            for (j = 0; j < i; j++) {
                fprintf(out, ", arg_%d", j);
            }
            fprintf(out, ")\\\n\ttype name(");
            for (j = 0; j < i; j++) {
                fprintf(out, "arg_%d, ", j);
            }
            fprintf(out, "arg_base)\n");

            fprintf(out,
                    "#define THUNK_CALL%d(retcode, name, arg_base", i);
            for (j = 0; j < i; j++) {
                fprintf(out, ", arg_%d", j);
            }
            fprintf(out, ")\\\n\tretcode name(");
            for (j = 0; j < i; j++) {
                fprintf(out, "arg_%d, ", j);
            }
            fprintf(out, "arg_base)\n");
        }
    }

    for(funclistit = cfg->funclist; funclistit != NULL; funclistit = funclistit->next)
    {
        writethunkfunc(out, cfg, funclistit);
    }

    fclose(out);
}
