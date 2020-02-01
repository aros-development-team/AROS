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
    unsigned int lvo;

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

    /* lvo contains the number of functions already printed in the functable */
    lvo = 0;

    if (!(cfg->options & OPTION_NORESIDENT))
    {
        fprintf(out,"# Generated module entry points...\n");
        if (cfg->modtype != RESOURCE && cfg->modtype != HANDLER)
        {
            fprintf(out,
                    "%s_ENTRYPOINTS += -Wl,--undefined=%s_1_%s_OpenLib\n"
                    "%s_ENTRYPOINTS += -Wl,--undefined=%s_2_%s_CloseLib\n"
                    "%s_ENTRYPOINTS += -Wl,--undefined=%s_3_%s_ExpungeLib\n"
                    "%s_ENTRYPOINTS += -Wl,--undefined=%s_4_%s_ExtFuncLib\n",
                    moduleversname, cfg->basename, cfg->basename,
                    moduleversname, cfg->basename, cfg->basename,
                    moduleversname, cfg->basename, cfg->basename,
                    moduleversname, cfg->basename, cfg->basename
            );
            lvo += 4;
        }
        if (cfg->modtype == MCC || cfg->modtype == MUI || cfg->modtype == MCP)
        {
            lvo++;
            fprintf(out,
                    "%s_ENTRYPOINTS += -Wl,--undefined=%s_%d_MCC_Query\n",
                    moduleversname, cfg->basename, lvo
            );
        }
        else if (cfg->modtype == DATATYPE)
        {
            lvo++;
            fprintf(out,
                    "%s_ENTRYPOINTS += -Wl,--undefined=%s_%d_ObtainEngine\n",
                   moduleversname, cfg->basename, lvo
            );
        }
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
            fprintf(out,
                    "%s_ENTRYPOINTS += -Wl,--undefined=",
                    moduleversname
            );

            switch (funclistit->libcall)
            {
            case STACK:
                    {
                        fprintf(out,
                                "%s\n"
#if defined(LINKENTRY_USE_STACKENTRYFULL)
                                "%s_ENTRYPOINTS += -Wl,--undefined="
#endif
                                , funclistit->name
#if defined(LINKENTRY_USE_STACKENTRYFULL)
                                , moduleversname
#endif
                        );
                    }
#if !defined(LINKENTRY_USE_STACKENTRYFULL)
                    break;
#endif
            case REGISTER:
            case REGISTERMACRO:
                    {
                        fprintf(out,
                                "%s_%d_%s\n",
                                cfg->basename, funclistit->lvo, funclistit->name
                        );
                    }
                    break;
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
                        "%s_ENTRYPOINTS += -Wl,--undefined=%s\n",
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
