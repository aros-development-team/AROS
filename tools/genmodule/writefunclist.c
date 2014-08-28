/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Write the functionlist to a file that can be included in the .conf file.
*/
#include "genmodule.h"

void writefunclist(struct config *cfg)
{
    FILE *out;
    char line[256];
    struct functionhead *funclistit;
    struct functionarg *arglistit;
    struct stringlist *aliaslistit;
    unsigned int lvo;

    snprintf(line, 255, "%s/%s.funclist", cfg->gendir, cfg->modulename);
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
        exit(20);
    }

    /* When not a BOOPSI class write out the functionlist even if it is empty
     * when it is a BOOPSI write only the list when it is not empty
     * When cfg->basename != cfg->classlist->basename this means we are not in a BOOPSI class
     * but there are extra classes defined
     */
    if (cfg->classlist == NULL
        || strcmp(cfg->basename, cfg->classlist->basename) != 0
        || cfg->funclist != NULL)
    {
        fprintf(out, "##begin functionlist\n");

        for (funclistit = cfg->funclist, lvo = cfg->firstlvo - 1;
             funclistit != NULL;
             funclistit = funclistit->next
        )
        {
            if (funclistit->libcall == REGISTERMACRO)
            {
                if (funclistit->lvo > lvo+1)
                {
                    if (funclistit->lvo == lvo+2)
                        fprintf(out, "\n");
                    else
                        fprintf(out, ".skip %u\n", funclistit->lvo - lvo - 1);
                }

                fprintf(out,
                        "%s %s(",
                        funclistit->type, funclistit->name
                );

                for (arglistit = funclistit->arguments;
                     arglistit!=NULL;
                     arglistit = arglistit->next
                )
                {
                    /* Print a , separator when not the first function argument */
                    if (arglistit != funclistit->arguments)
                        fprintf(out, ", ");

                    fprintf(out, "%s", arglistit->arg);
                }
                fprintf(out, ") (");

                for (arglistit = funclistit->arguments;
                     arglistit != NULL;
                     arglistit = arglistit->next
                )
                {
                    /* Print a , separator when not the first function argument */
                    if (arglistit != funclistit->arguments)
                        fprintf(out, ", ");

                    fprintf(out, "%s", arglistit->reg);
                }
                fprintf(out, ")\n");

                for (aliaslistit = funclistit->aliases;
                     aliaslistit != NULL;
                     aliaslistit = aliaslistit->next
                )
                {
                    fprintf(out, ".alias %s\n", aliaslistit->s);
                }

                if (funclistit->novararg)
                    fprintf(out, ".novararg\n");

                if (funclistit->priv)
                    fprintf(out, ".private\n");

                lvo = funclistit->lvo;
            }
        }

        fprintf(out, "##end functionlist\n");
    }

    if (cfg->classlist != NULL && strcmp(cfg->basename, cfg->classlist->basename) == 0)
    {
        fprintf(out, "##begin methodlist\n");

        for(funclistit = cfg->classlist->methlist;
            funclistit != NULL;
            funclistit = funclistit->next
        )
        {
            fprintf(out, "%s\n", funclistit->name);
        }

        fprintf(out, "##end methodlist\n");
    }

    if (ferror(out))
    {
        perror(line);
        exit(20);
    }

    fclose(out);
}
