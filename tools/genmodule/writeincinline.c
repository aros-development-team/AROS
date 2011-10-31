/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write inline/modulename.h. Part of genmodule.
*/
#include "genmodule.h"

static void writeinlineregister(FILE *, struct functionhead *, struct config *);
static void writeinlinevararg(FILE *, struct functionhead *, struct config *);
static void writealiases(FILE *, struct functionhead *, struct config *);

void writeincinline(struct config *cfg)
{
    FILE *out;
    char line[256], *banner;
    struct functionhead *funclistit;

    snprintf(line, 255, "%s/inline/%s.h", cfg->gendir, cfg->modulename);
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
        exit(20);
    }

    banner = getBanner(cfg);
    fprintf(out,
	    "#ifndef INLINE_%s_H\n"
	    "#define INLINE_%s_H\n"
	    "\n"
        "%s"
	    "\n"
        "/*\n"
	    "    Desc: Inline function for %s\n"
	    "*/\n"
	    "\n"
	    "#include <aros/libcall.h>\n"
	    "#include <exec/types.h>\n"
	    "#include <aros/symbolsets.h>\n"
	    "#include <aros/preprocessor/variadic/cast2iptr.hpp>\n"
	    "\n",
	    cfg->modulenameupper, cfg->modulenameupper, banner, cfg->modulename
    );
    freeBanner(banner);

    for (funclistit = cfg->funclist; funclistit!=NULL; funclistit = funclistit->next)
    {
        if (!funclistit->priv && (funclistit->lvo >= cfg->firstlvo))
        {
            if (funclistit->libcall != STACK)
            {
                writeinlineregister(out, funclistit, cfg);
                if (!funclistit->novararg)
                    writeinlinevararg(out, funclistit, cfg);
            }
            else /* libcall == STACK */
            {
		/* This is the same as in defines */
		writedefinestack(out, funclistit, cfg);
            }

            writealiases(out, funclistit, cfg);
        }
    }

    fprintf(out,
	    "\n"
	    "#endif /* INLINE_%s_H*/\n",
	    cfg->modulenameupper
    );
    fclose(out);
}


void
writeinlineregister(FILE *out, struct functionhead *funclistit, struct config *cfg)
{
    struct functionarg *arglistit;
    int count, isvoid;
    int narg=0,nquad=0;
    char *type;
    
    isvoid = strcmp(funclistit->type, "void") == 0
	|| strcmp(funclistit->type, "VOID") == 0;

    fprintf(out,
	    "\n"
	    "static inline %s __inline_%s_%s(",
	    funclistit->type, cfg->basename, funclistit->name
    );
    for (arglistit = funclistit->arguments, count = 1;
	 arglistit!=NULL;
	 arglistit = arglistit->next, count++
    )
    {
        type = getargtype(arglistit);
	fprintf(out, "%s __arg%d, ", type, count);
	if (strchr(arglistit->reg, '/') != NULL) {
	    nquad++;
	} else {
	    narg++;
	}
    }
    fprintf(out,
            "APTR __%s)\n"
            "{\n",
            cfg->libbase
    );
    if (cfg->options & OPTION_AUTOINIT) {
        fprintf(out,
            "    AROS_LIBREQ(%s, %d)\n",
            cfg->libbase, funclistit->version
        );
    }
    if (nquad==0)
    {
        fprintf(out,
                "    %sAROS_LC%d%s(%s, %s,\n",
                (isvoid) ? "" : "return ",
                funclistit->argcount, (isvoid) ? "NR" : "",
                funclistit->type, funclistit->name
         );

        for (arglistit = funclistit->arguments, count = 1;
             arglistit!=NULL;
             arglistit = arglistit->next, count++
        )
        {
            type = getargtype(arglistit);
            assert(type != NULL);
            fprintf(out,
                    "        AROS_LCA(%s,(__arg%d),%s),\n",
                    type, count, arglistit->reg
            );
            free(type);
        }
    }
    else /* nquad != 0 */
    {
        if (narg) {
	    fprintf(out,
		    "    %sAROS_LC%dQUAD%d%s(%s, %s,\n",
		    (isvoid) ? "" : "return ", narg,
		    nquad, (isvoid) ? "NR" : "",
		    funclistit->type, funclistit->name
	    );
	} else {
       	    fprintf(out,
		    "    %sAROS_LCQUAD%d%s(%s, %s,\n",
		    (isvoid) ? "" : "return ",
		    nquad, (isvoid) ? "NR" : "",
		    funclistit->type, funclistit->name
	    );
	}
  
	for (arglistit = funclistit->arguments, count = 1;
	     arglistit != NULL;
	     arglistit = arglistit->next, count++
	)
	{
	    char *quad2 = strchr(arglistit->reg, '/');
	 
	    arglistit->reg[2] = 0;
	    type = getargtype(arglistit);
	    assert(type != NULL);
	
	    if (quad2 != NULL) {
	    	*quad2 = 0;
		fprintf(out,
			"         AROS_LCAQUAD(%s, (__arg%d), %s, %s), \\\n",
			type, count, arglistit->reg, quad2+1
		);
		*quad2 = '/';
	    } else {
		fprintf(out,
			"         AROS_LCA(%s, (__arg%d), %s), \\\n",
			type, count, arglistit->reg
		);
	    }
	    free(type);
	}
    }
    fprintf(out,
	    "        %s, (__%s), %u, %s"
            "    );\n"
            "}\n\n",
	    cfg->libbasetypeptrextern, cfg->libbase,	funclistit->lvo, cfg->basename
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
    fprintf(out, ") \\\n    __inline_%s_%s(", cfg->basename, funclistit->name);
    for (arglistit = funclistit->arguments, count = 1;
	 arglistit != NULL;
	 arglistit = arglistit->next, count++
    )
	fprintf(out, "(arg%d), ", count);
    fprintf(out, "(APTR)%s)\n", cfg->libbase);
}

void
writeinlinevararg(FILE *out, struct functionhead *funclistit, struct config *cfg)
{
    struct functionarg *arglistit = funclistit->arguments;
    char isvararg = 0, *varargname, *lastname;
	
    /* Go to last argument */
    if (arglistit == NULL)
	return;
	    
    while (arglistit->next != NULL) arglistit = arglistit->next;
    
    lastname = getargname(arglistit);
    assert(lastname != NULL);
    
    if (*(funclistit->name + strlen(funclistit->name) - 1) == 'A')
    {
	isvararg = 1;
	varargname = strdup(funclistit->name);
	varargname[strlen(funclistit->name)-1] = '\0';
    }
    else if (strcmp(funclistit->name + strlen(funclistit->name) - 7, "TagList") == 0)
    {
	isvararg = 1;
	/* TagList has to be changed in Tags at the end of the functionname */
	varargname = strdup(funclistit->name);
	varargname[strlen(funclistit->name)-4] = 's';
	varargname[strlen(funclistit->name)-3] = '\0';
    }
    else if (strcmp(funclistit->name + strlen(funclistit->name) - 4, "Args") == 0
	     && (strcasecmp(lastname, "args") == 0 || strcasecmp(lastname, "arglist") == 0)
    )
    {
	isvararg = 1;
	varargname = strdup(funclistit->name);
	varargname[strlen(funclistit->name)-4] = '\0';
    }
    else if ((funclistit->name[0] == 'V') &&  (strncmp(arglistit->arg, "va_list", 7) == 0))
    {
	isvararg = 2;
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
		    isvararg = 1;
		    varargname = malloc(strlen(funclistit->name) + 5);
		    strcpy(varargname, funclistit->name);
		    strcat(varargname, "Tags");
		}
	    }
	}
    }
    if (isvararg == 1)
    {
	int count;
	char *type;
	
	fprintf(out,
		"\n#if !defined(NO_INLINE_STDARG) && !defined(%s_NO_INLINE_STDARG)\n"
		"#define %s(",
		cfg->modulenameupper, varargname
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
		"    IPTR __args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \\\n"
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
		type = getargtype(arglistit);
		assert(type != NULL);
		fprintf(out, "(%s)__args", type);
		free(type);
	    }
	    else
		fprintf(out, "(arg%d)", count);
	}
	fprintf(out,
		"); \\\n"
		"})\n"
		"#endif /* !NO_INLINE_STDARG */\n"
	);
	    
	free(varargname);
    }
    else if (isvararg == 2)
    {
	int count;

	fprintf(out,
		"\n#if !defined(NO_INLINE_STDARG) && !defined(%s_NO_INLINE_STDARG)\n"
		"static inline %s __inline_%s_%s(%s __%s",
		cfg->modulenameupper,
		funclistit->type, cfg->basename, varargname, cfg->libbasetypeptrextern, cfg->libbase
	);
	for (arglistit = funclistit->arguments, count = 0;
	     arglistit != NULL && arglistit->next != NULL;
	     arglistit = arglistit->next
	)
	{
	    char *type = getargtype(arglistit);

	    fprintf(out, ", %s __arg%d", type, ++count);
	}
	fprintf(out, ", ...)\n");

	fprintf(out,
		"{\n"
		"    %s retval;\n"
		"    va_list __args;\n"
		"\n"
		"    va_start(__args, __arg%d);\n"
		"    retval = __inline_%s_%s(",
		funclistit->type,
		count,
		cfg->basename, funclistit->name
	);
	for (arglistit = funclistit->arguments, count = 1;
	     arglistit != NULL && arglistit->next != NULL;
	     arglistit = arglistit->next, count++
	)
	{
	    fprintf(out, "__arg%d, ", count);
	}
	fprintf(out,
		"__args, __%s);\n"
		"    va_end(__args);\n"
		"    return retval;\n"
		"}\n"
		"\n"
		"#define %s(",
		cfg->libbase,
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
		"    __inline_%s_%s(%s, ",
		cfg->basename, varargname, cfg->libbase
	);
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

	free(varargname);
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
