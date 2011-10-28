/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write defines/modulename.h. Part of genmodule.
*/
#include "genmodule.h"

static void writedefineregister(FILE *, struct functionhead *, struct config *);
static void writedefinevararg(FILE *, struct functionhead *, struct config *);
static void writealiases(FILE *, struct functionhead *, struct config *);

void writeincdefines(struct config *cfg)
{
    FILE *out;
    char line[256], *banner;
    struct functionhead *funclistit;

    snprintf(line, 255, "%s/defines/%s.h", cfg->gendir, cfg->modulename);
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
        exit(20);
    }

    banner = getBanner(cfg);
    fprintf(out,
	    "#ifndef DEFINES_%s_H\n"
	    "#define DEFINES_%s_H\n"
	    "\n"
	    "%s"
	    "\n"
	    "/*\n"
	    "    Desc: Defines for %s\n"
	    "*/\n"
	    "\n",
	    cfg->modulenameupper, cfg->modulenameupper, banner, cfg->modulename);
    if (cfg->options & OPTION_BASEREL)
    	fprintf(out,
    	    "#define AROS_BASEREL_LIBRARY\n");
    fprintf(out,
	    "#include <aros/libcall.h>\n"
	    "#include <exec/types.h>\n"
	    "#include <aros/symbolsets.h>\n"
	    "#include <aros/preprocessor/variadic/cast2iptr.hpp>\n"
	    "\n"
	    "__BEGIN_DECLS\n"
	    "\n"
    );
    freeBanner(banner);

    for (funclistit = cfg->funclist; funclistit!=NULL; funclistit = funclistit->next)
    {
	if (!funclistit->priv && (funclistit->lvo >= cfg->firstlvo))
	{
	    if (funclistit->libcall != STACK)
	    {
		writedefineregister(out, funclistit, cfg);
		if (!funclistit->novararg)
		    writedefinevararg(out, funclistit, cfg);
	    }
	    else /* libcall == STACK */
	    {
		writedefinestack(out, funclistit, cfg);
	    }
	    
	    writealiases(out, funclistit, cfg);
	}
    }
    fprintf(out,
	    "\n"
	    "__END_DECLS\n"
	    "\n"
	    "#endif /* DEFINES_%s_H*/\n",
	    cfg->modulenameupper
    );
    fclose(out);
}


void
writedefineregister(FILE *out, struct functionhead *funclistit, struct config *cfg)
{
    struct functionarg *arglistit;
    int count, isvoid;
    char *type;
    
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
    }
    fprintf(out,
                    ") ({\\\n"
    );
    if (cfg->options & OPTION_AUTOINIT) {
        fprintf(out,
		    "        AROS_LIBREQ(%s,%d)\\\n",
		    cfg->libbase, funclistit->version
        );       
    }
    if (funclistit->arguments == NULL
	|| strchr(funclistit->arguments->reg, '/') == NULL
    )
    {
	    fprintf(out,
		    "        AROS_LC%d%s(%s, %s, \\\n",
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
			"                  AROS_LCA(%s,(__arg%d),%s), \\\n",
			type, count, arglistit->reg
		);
		free(type);
	    }
    }
    else
    {
	 fprintf(out,
		    "        AROS_LCQUAD%d%s(%s, %s, \\\n",
		    funclistit->argcount, (isvoid) ? "NR" : "",
		    funclistit->type, funclistit->name
	);

	for (arglistit = funclistit->arguments, count = 1;
	     arglistit != NULL;
	     arglistit = arglistit->next, count++
	)
	{
	    if (strlen(arglistit->reg) != 5)
	    {
		fprintf(stderr, "Internal error: ../.. register format expected\n");
		exit(20);
	    }
	    arglistit->reg[2] = 0;

	    type = getargtype(arglistit);
	    assert(type != NULL);
	
	    fprintf(out,
		    "         AROS_LCAQUAD(%s, (__arg%d), %s, %s), \\\n",
		    type, count, arglistit->reg, arglistit->reg+3
	    );
	    arglistit->reg[2] = '/';
	    free(type);
	}
    }
    fprintf(out,
	    "        %s, (__%s), %u, %s);\\\n})\n\n",
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
    fprintf(out, ") \\\n    __%s_WB(%s", funclistit->name, cfg->libbase);
    for (arglistit = funclistit->arguments, count = 1;
	 arglistit != NULL;
	 arglistit = arglistit->next, count++
    )
	fprintf(out, ", (arg%d)", count);
    fprintf(out, ")\n");
}

void
writedefinevararg(FILE *out, struct functionhead *funclistit, struct config *cfg)
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
	struct functionarg *lastarg;

	fprintf(out,
		"\n#if !defined(NO_INLINE_STDARG) && !defined(%s_NO_INLINE_STDARG)\n"
		"static inline %s __%s_WB(%s __%s",
		cfg->modulenameupper,
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
		getargname(lastarg),
		funclistit->name, cfg->libbase
	);
	for (arglistit = funclistit->arguments;
	     arglistit != NULL && arglistit->next != NULL;
	     arglistit = arglistit->next
	)
	{
	    fprintf(out, "%s, ", getargname(arglistit));
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
	     arglistit != NULL && arglistit->next != NULL;
	     arglistit = arglistit->next, count++
	)
	{
	    fprintf(out, "arg%d, ", count);
	}
	fprintf(out,
		"...) __%s_WB(%s, ",
		varargname, cfg->libbase
	);
	for (arglistit = funclistit->arguments, count = 1;
	     arglistit != NULL && arglistit->next != NULL;
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
writedefinestack(FILE *out, struct functionhead *funclistit, struct config *cfg)
{
    struct functionarg *arglistit;

    /* Only if no baserel. define can't handle libbase passing */
    if (cfg->options & (OPTION_BASEREL | OPTION_PERTASKBASE))
    	return;

    fprintf(out, "#define %s ((%s (*)(", funclistit->name, funclistit->type);
    for (arglistit = funclistit->arguments;
	 arglistit != NULL;
	 arglistit = arglistit->next
    )
    {
	fprintf(out, "%s", arglistit->arg);
	if (arglistit->next != NULL)
	    fprintf(out, ", ");
    }
    fprintf(out, "))__AROS_GETVECADDR(%s,%d))\n", cfg->libbase, funclistit->lvo);
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
