/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write module_stubs.c. Part of genmodule.
*/
#include "genmodule.h"

void writestubs(struct config *cfg)
{
    FILE *out;
    char line[256];
    struct functionhead *funclistit;
    struct functionarg *arglistit;
    struct functionalias *aliasesit;

    snprintf(line, 255, "%s/%s_stubs.c", cfg->gendir, cfg->modulename);
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
        "    Copyright © 1995-2004, The AROS Development Team. All rights reserved.\n"
        "*/\n"
        "#define NOLIBDEFINES\n"
        "/* Be sure that the libbases are included in the stubs file */\n"
        "#undef __NOLIBBASE__\n"
        "#undef __%s_NOLIBBASE__\n",
        cfg->modulenameupper
    );
    
    if (cfg->modtype != MCC && cfg->modtype != MUI && cfg->modtype != MCP)
    {
        fprintf(out, "#include <proto/%s.h>\n", cfg->modulename);
    }
    
    fprintf
    (
        out,
        "#include <exec/types.h>\n"
        "#include <aros/libcall.h>\n"
        "\n"
    );
    
    for (funclistit = funclist; funclistit!=NULL; funclistit = funclistit->next)
    {
        if (funclistit->lvo >= cfg->firstlvo)
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
	    
	    fprintf(out, "                    %s, %s, %u, %s);\n}\n",
		    cfg->libbasetypeptrextern, cfg->libbase, funclistit->lvo, cfg->basename
	    );
	}
    }
    fclose(out);
}
