/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write defines/modulename.h. Part of genmodule.
*/
#include "genmodule.h"

static void writedefineregister(FILE *, struct functionhead *);
static void writedefinevararg(FILE *, struct functionhead *);
static void writedefinestack(FILE *, struct functionhead *);
static void writealiases(FILE *, struct functionhead *);

void writeincdefines(int dummy)
{
    FILE *out;
    char line[256];
    struct functionhead *funclistit;

    snprintf(line, 255, "%s/defines/%s.h", genincdir, modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could not write %s\n", line);
	exit(20);
    }
    fprintf(out,
	    "#ifndef DEFINES_%s_PROTOS_H\n"
	    "#define DEFINES_%s_PROTOS_H\n"
	    "\n"
	    "/*\n"
	    "    *** Automatically generated file. Do not edit ***\n"
	    "    Copyright © 1995-2003, The AROS Development Team. All rights reserved.\n"
	    "\n"
	    "    Desc: Defines for %s\n"
	    "*/\n"
	    "\n"
	    "#include <aros/libcall.h>\n"
	    "#include <exec/types.h>\n"
	    "\n",
	    modulenameupper, modulenameupper, modulename);
    if (!dummy)
    {
	for (funclistit = funclist; funclistit!=NULL; funclistit = funclistit->next)
	{
	    if (funclistit->lvo >= firstlvo)
	    {
		if (libcall != STACK)
		{
		    writedefineregister(out, funclistit);
		    if (!funclistit->novararg)
			writedefinevararg(out, funclistit);
		}
		else /* libcall == STACK */
		{
		    writedefinestack(out, funclistit);
		}
		
		writealiases(out, funclistit);
	    }
	}
    }
    fprintf(out,
	    "\n"
	    "#endif /* DEFINES_%s_PROTOS_H*/\n",
	    modulenameupper);
    fclose(out);
}


void
writedefineregister(FILE *out, struct functionhead *funclistit)
{
    struct functionarg *arglistit;
    
    fprintf(out,
	    "\n"
	    "#define __%s_WB(__%s",
	    funclistit->name, libbase
    );
    for (arglistit = funclistit->arguments;
	 arglistit!=NULL;
	 arglistit = arglistit->next
    )
    {
	fprintf(out, ", __%s", arglistit->name);
    }
    fprintf(out,
	    ") \\\n"
	    "        AROS_LC%d(%s, %s, \\\n",
	    funclistit->argcount, funclistit->type, funclistit->name
    );
    for (arglistit = funclistit->arguments;
	 arglistit!=NULL;
	 arglistit = arglistit->next
    )
    {
	fprintf(out,
		"                  AROS_LCA(%s,(__%s),%s), \\\n",
		arglistit->type, arglistit->name, arglistit->reg
	);
    }
    fprintf(out,
	    "        %s *, (__%s), %u, %s)\n\n",
	    libbasetypeextern, libbase,	funclistit->lvo, basename
    );

    fprintf(out, "#define %s(", funclistit->name);
    for (arglistit = funclistit->arguments;
	 arglistit != NULL;
	 arglistit = arglistit->next
    )
    {
	if (arglistit != funclistit->arguments)
	    fprintf(out, ", ");
	fprintf(out, "%s", arglistit->name);
    }
    fprintf(out, ") \\\n    __%s_WB(%s", funclistit->name, libbase);
    for (arglistit = funclistit->arguments;
	 arglistit != NULL;
	 arglistit = arglistit->next
    )
	fprintf(out, ", (%s)", arglistit->name);
    fprintf(out, ")\n");
}

void
writedefinevararg(FILE *out, struct functionhead *funclistit)
{
    struct functionarg *arglistit = funclistit->arguments;
    char isvararg = 0, *varargname;
	
    /* Go to last argument */
    if (arglistit == NULL)
	return;
	    
    while (arglistit->next != NULL) arglistit = arglistit->next;

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
	     && (strcasecmp(arglistit->name, "args") == 0 || strcasecmp(arglistit->name, "argist") == 0)
    )
    {
	isvararg = 1;
	varargname = strdup(funclistit->name);
	varargname[strlen(funclistit->name)-4] = '\0';
    }
    else
    {
	char *p;
		    
	if (strncmp(arglistit->type, "struct", 6)==0)
	{
	    p = arglistit->type + 6;
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
    if (isvararg)
    {
	fprintf(out,
		"\n#if !defined(NO_INLINE_STDARG) && !defined(%s_NO_INLINE_STDARG)\n"
		"#define %s(",
		modulenameupper, varargname);
	for (arglistit = funclistit->arguments;
	     arglistit != NULL && arglistit->next != NULL;
	     arglistit = arglistit->next
	)
	{
	    fprintf(out, "%s, ", arglistit->name);
	}
	fprintf(out,
		"args...) \\\n"
		"({ \\\n"
		"    IPTR __args[] = { args }; \\\n"
		"    %s(",
		funclistit->name
	);
	for (arglistit = funclistit->arguments;
	     arglistit != NULL;
	     arglistit = arglistit->next
	)
	{
	    if (arglistit != funclistit->arguments)
		fprintf(out, ", ");
			
	    if (arglistit->next == NULL)
		fprintf(out, "(%s)__args", arglistit->type);
	    else
		fprintf(out, "(%s)", arglistit->name);
	}
	fprintf(out,
		"); \\\n"
		"})\n"
		"#endif /* !NO_INLINE_STDARG */\n"
	);
	    
	free(varargname);
    }
}

void
writedefinestack(FILE *out, struct functionhead *funclistit)
{
    struct functionarg *arglistit;
    
    fprintf(out, "#define %s ((%s (*)(", funclistit->name, funclistit->type);
    for (arglistit = funclistit->arguments;
	 arglistit != NULL;
	 arglistit = arglistit->next
    )
    {
	fprintf(out, "%s", arglistit->type);
	if (arglistit->next != NULL)
	    fprintf(out, ", ");
    }
    fprintf(out, "))__AROS_GETVECADDR(%s,%d))\n", libbase, funclistit->lvo);
}

void
writealiases(FILE *out, struct functionhead *funclistit)
{
    struct functionalias *aliasesit;
    
    for (aliasesit = funclistit->aliases;
	 aliasesit != NULL;
	 aliasesit = aliasesit->next
    )
    {
	fprintf(out, "#define %s %s\n", aliasesit->alias, funclistit->name);
    }
}
