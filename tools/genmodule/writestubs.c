/*
    Copyright � 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write module_autoinit.c. Part of genmodule.
*/
#include "genmodule.h"
#include <aros/machine.h>

void writestubs(void)
{
    FILE *out, *outasm, *f;
    char line[256];
    struct functionlist *funclistit;
    struct arglist *arglistit;
    struct aliaslist *aliasesit;

    snprintf(line, 255, "%s/%s_stubs.c", gendir, modulename);
    out = fopen(line, "w");
    if (out==NULL)
    {
	fprintf(stderr, "Could not write %s\n", line);
	exit(20);
    }
    fprintf
    (
        out,
        "/*\n"
        "    *** Automatically generated file. Do not edit ***\n"
        "    Copyright � 1995-2003, The AROS Development Team. All rights reserved.\n"
        "*/\n"
        "#define NOLIBDEFINES\n"
        "/* Be sure that the libbases are included in the stubs file */\n"
        "#undef __NOLIBBASE__\n"
        "#undef __%s_NOLIBBASE__\n",
        modulenameupper
    );
    
    if (modtype != MCC && modtype != MUI && modtype != MCP)
    {
        fprintf(out, "#include <proto/%s.h>\n", modulename);
    }
    
    fprintf
    (
        out,
        "#include <exec/types.h>\n"
        "#include <aros/libcall.h>\n"
        "\n"
    );
    
    snprintf(line, 255, "%s/%s_astubs.S", gendir, modulename);
    outasm = fopen(line, "w");
    if (outasm==NULL)
    {
	fprintf(stderr, "Could not write %s\n", line);
	exit(20);
    }
    fprintf
    (
        outasm,
        "/*\n"
        "    *** Automatically generated file. Do not edit ***\n"
        "    Copyright � 1995-2003, The AROS Development Team. All rights reserved.\n"
        "*/\n"
    );
    fprintf(outasm, STUBCODE_INIT);
    
    for (funclistit = funclist; funclistit!=NULL; funclistit = funclistit->next)
    {
	if (libcall != STACK)
	{
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
		fprintf(out, "%s %s", arglistit->type, arglistit->name);
	    }
	    fprintf(out,
		    ")\n"
		    "{\n"
		    "    return AROS_LC%d(%s, %s,\n",
		    funclistit->argcount, funclistit->type, funclistit->name
	    );
	    for (arglistit = funclistit->arguments;
		 arglistit!=NULL;
		 arglistit = arglistit->next
	    )
		fprintf(out, "                    AROS_LCA(%s,%s,%s),\n",
			arglistit->type, arglistit->name, arglistit->reg
		);
	    fprintf(out, "                    %s *, %s, %u, %s);\n}\n", libbasetypeextern,
		    libbase, funclistit->lvo, basename
	    );
	}
	else /* libcall==STACK */
	{
	    fprintf(outasm,
		    STUBCODE,
		    funclistit->name,
		    libbase,
		    &(__AROS_GETJUMPVEC(NULL, funclistit->lvo)->vec)
	    );
	}
	
	for (aliasesit = funclistit->aliases;
	     aliasesit != NULL;
	     aliasesit = aliasesit->next
	)
	    fprintf(outasm, ALIASCODE, funclistit->name, aliasesit->alias);
    }
    fclose(out);
    fclose(outasm);
}
