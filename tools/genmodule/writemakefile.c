/*
    Copyright © 2005-2014, The AROS Development Team. All rights reserved.
    $Id$

    Code to write a Makefile with variables that provides the files
    and configuration for building the module
*/
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "genmodule.h"
#include "config.h"

static inline const char *upname(const char *s)
{
    static char name[512];
    int i = 0;

    while (s && i < (sizeof(name)-1))
        name[i++] = toupper(*(s++));
    name[i] = 0;

    return &name[0];
}

static inline void writemakefilestubs(struct config *cfg, int is_rel, FILE *out)
{
    struct functionhead *funclistit;

    for (funclistit = cfg->funclist;
         funclistit!=NULL;
         funclistit = funclistit->next
    )
    {
        if (funclistit->lvo >= cfg->firstlvo && funclistit->libcall == STACK)
        {
            fprintf(out, " %s_%s_%sstub\\\n", cfg->modulename, funclistit->name, is_rel ? "rel" : "");
        }
    }

   fprintf(out, " %s_regcall_%sstubs", cfg->modulename, is_rel ? "rel" : "");
}

void writemakefile(struct config *cfg)
{
    FILE *out;
    char name[512];
    struct stringlist *s;

    snprintf(name, sizeof(name), "%s/Makefile.%s%s", cfg->gendir, cfg->modulename, cfg->modtypestr);

    out = fopen(name, "w");

    if (out == NULL)
    {
        perror(name);
        exit(20);
    }

    fprintf(out,
            "%s_STARTFILES += %s_start\n"
            "%s_ENDFILES += %s_end\n"
            "%s_MODDIR += %s\n",
            cfg->modulename, cfg->modulename,
            cfg->modulename, cfg->modulename,
            cfg->modulename, cfg->moddir
    );

    fprintf(out, "%s_LINKLIBFILES +=", cfg->modulename);
    if (cfg->options & OPTION_STUBS)
        writemakefilestubs(cfg, 0, out);
    if (cfg->options & OPTION_AUTOINIT)
        fprintf(out, " %s_autoinit", cfg->modulename);
    if (cfg->modtype == LIBRARY)
        fprintf(out, " %s_getlibbase", cfg->modulename);
    fprintf(out, "\n");
    fprintf(out, "%s_RELLINKLIBFILES +=", cfg->modulename);
    if (cfg->options & OPTION_STUBS)
        writemakefilestubs(cfg, 1, out);
    if (cfg->options & OPTION_AUTOINIT)
        fprintf(out, " %s_relautoinit", cfg->modulename);
    if (cfg->modtype == LIBRARY)
        fprintf(out, " %s_relgetlibbase", cfg->modulename);
    fprintf(out, "\n");

    /* Currently there are no asm files anymore */
    fprintf(out, "%s_LINKLIBAFILES +=\n", cfg->modulename);
    fprintf(out, "%s_RELLINKLIBAFILES +=\n", cfg->modulename);

    fprintf(out, "%s_INCLUDES += ", cfg->modulename);
    if (cfg->options & OPTION_INCLUDES)
    {
        fprintf(out,
                "clib/%s_protos.h inline/%s.h defines/%s.h proto/%s.h",
                cfg->includename, cfg->includename, cfg->includename, cfg->includename
        );
    }
    if (cfg->interfacelist)
    {
        struct interfaceinfo *in;
        for (in = cfg->interfacelist; in; in = in->next)
            fprintf(out,
                    " interface/%s.h"
                    , in->interfacename
            );
    }
    fprintf(out, "\n");

    fprintf(out, "%s_CFLAGS  +=", cfg->modulename);
    for (s = cfg->rellibs; s ; s = s->next)
        fprintf(out, " -D__%s_RELLIBBASE__", upname(s->s));
    fprintf(out, "\n");

    fprintf(out, "%s_DFLAGS  +=", cfg->modulename);
    for (s = cfg->rellibs; s ; s = s->next)
        fprintf(out, " -D__%s_RELLIBBASE__", upname(s->s));
    fprintf(out, "\n");

    fprintf(out, "%s_LDFLAGS +=", cfg->modulename);
    fprintf(out,"\n");

    fprintf(out, "%s_LIBS +=", cfg->modulename);
    for (s = cfg->rellibs; s ; s = s->next)
        fprintf(out, " %s_rel", s->s);
    fprintf(out,"\n");

    if (ferror(out))
    {
        perror("Error writing Makefile");
        fclose(out);
        exit(20);
    }

    fclose(out);
}
