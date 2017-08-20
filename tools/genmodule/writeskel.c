/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    The code for creating skeleton files for the functions present in the module
*/
#include <string.h>
#include <assert.h>
#include "functionhead.h"
#include "config.h"

static void writeskelfunc(struct config *cfg, struct functionhead *funclist)
{
    FILE *out;
    char line[256], *banner;
    struct functionarg *arglistit;
    char *type, *name;
    int first;

    snprintf(line, 255, "%s/%s.c", cfg->gendir, funclist->internalname);
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
        exit(20);
    }

    fprintf(out,
        "/*\n"
        "    Copyright \xA9 <year>, The AROS Development Team. All rights reserved.\n"
        "    $I" "d$\n"
        "*/\n\n"
    );

    if (funclist->libcall == REGISTERMACRO)
    {
        fprintf(out,
                "/*****************************************************************************\n\n"
                "    NAME */\n"
                "        AROS_LH%d(%s, %s,\n\n"
                "/*  SYNOPSIS */\n",
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
                "\n/*  LOCATION */\n"
                "        %s, %s, %u, %s)\n\n"
                "/*  FUNCTION\n\n"
                "    INPUTS\n\n"
                "    RESULT\n\n"
                "    NOTES\n\n"
                "    EXAMPLE\n\n"
                "    BUGS\n\n"
                "    SEE ALSO\n\n"
                "    INTERNALS\n\n"
                "    HISTORY\n\n"
                "*****************************************************************************/\n"
                "{\n"
                "    AROS_LIBFUNC_INIT\n\n"
                "    AROS_LIBFUNC_EXIT\n"
                "}\n\n",
                cfg->libbasetypeptrextern, cfg->libbase, funclist->lvo, cfg->basename
        );
    }
}

void writeskel(struct config *cfg)
{
    struct functionhead *funclistit;

    for(funclistit = cfg->funclist; funclistit != NULL; funclistit = funclistit->next)
    {
        writeskelfunc(cfg, funclistit);
    }
}
