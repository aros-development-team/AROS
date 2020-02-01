/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Export the module entry points e.g to use when linking with --gc-sections
*/
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "genmodule.h"
#include "config.h"

void writelinkentries(struct config *cfg)
{
    FILE *out;
    char moduleversname[512];
    char name[512];
    struct functionhead *funclistit;

    if (!cfg->flavour)
    {
        snprintf(moduleversname, sizeof(moduleversname), "%s", cfg->modulename);
    }
    else
    {
        snprintf(moduleversname, sizeof(moduleversname), "%s_%s", cfg->modulename, cfg->flavour);
    }

    snprintf(name, sizeof(name), "%s/%s%s.entrypoints", cfg->gendir, moduleversname, cfg->modtypestr);
    out = fopen(name, "w");

    if (out == NULL)
    {
        perror(name);
        exit(20);
    }

    if (cfg->classlist == NULL
        || strcmp(cfg->basename, cfg->classlist->basename) != 0
        || cfg->funclist != NULL)
    {
        fprintf(out,"# LVO entry points...\n");
        for (funclistit = cfg->funclist;
             funclistit != NULL;
             funclistit = funclistit->next
        )
        {
            if (funclistit->libcall == REGISTERMACRO)
            {
                fprintf(out,
                        "%s_ENTRYPOINTS += --entry %s_%d_%s\n",
                        moduleversname, cfg->basename, funclistit->lvo, funclistit->name
                );
            }
        }
    }

    if (cfg->classlist != NULL && strcmp(cfg->basename, cfg->classlist->basename) == 0)
    {
        fprintf(out,"# Method entry points...\n");
        for(funclistit = cfg->classlist->methlist;
            funclistit != NULL;
            funclistit = funclistit->next
        )
        {
            fprintf(out,
                        "%s_ENTRYPOINTS += --entry %s\n",
                        moduleversname, funclistit->name
                );
        }
    }

    if (ferror(out))
    {
        perror("Error writing entrypoints");
        fclose(out);
        exit(20);
    }

    fclose(out);
}
