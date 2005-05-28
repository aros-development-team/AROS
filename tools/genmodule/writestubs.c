/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write module_stubs.c. Part of genmodule.
*/
#include <aros/machine.h>

#include "genmodule.h"

void writestubs(struct config *cfg, struct functions *functions)
{
    FILE *out, *outasm;
    char line[256], *type, *name;
    struct functionhead *funclistit;
    struct stringlist *aliasesit;
    struct functionarg *arglistit;

    snprintf(line, 255, "%s/%s_stubs.c", cfg->gendir, cfg->modulename);
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
    	exit(20);
    }

    fprintf
    (
        out,
        "%s"
        "#define NOLIBDEFINES\n"
        "/* Be sure that the libbases are included in the stubs file */\n"
        "#undef __NOLIBBASE__\n"
        "#undef __%s_NOLIBBASE__\n",
        getBanner(cfg), cfg->modulenameupper
    );

    if (cfg->intcfg & CFG_GENASTUBS)
    {
	snprintf(line, 255, "%s/%s_astubs.S", cfg->gendir, cfg->modulename);
	outasm = fopen(line, "w");
	if (outasm==NULL)
	{
	    fprintf(stderr, "Could not write %s\n", line);
	    exit(20);
	}
	fprintf(outasm, "%s", getBanner(cfg));
	fprintf(outasm, STUBCODE_INIT);
    }

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
    
    for (funclistit = functions->funclist;
	 funclistit!=NULL;
	 funclistit = funclistit->next
    )
    {
        if (funclistit->lvo >= cfg->firstlvo)
	{
	    if (funclistit->libcall != STACK)
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
		    fprintf(out, "%s", arglistit->arg);
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
		{
		    type = getargtype(arglistit);
		    name = getargname(arglistit);
		    assert(type != NULL && name != NULL);
		    
		    fprintf(out, "                    AROS_LCA(%s,%s,%s),\n",
			    type, name, arglistit->reg
		    );
		    free(type);
		    free(name);
		}
	    
		fprintf(out, "                    %s, %s, %u, %s);\n}\n",
			cfg->libbasetypeptrextern, cfg->libbase, funclistit->lvo, cfg->basename
		);
	    }
	    else /* libcall==STACK */
	    {
		assert(cfg->intcfg & CFG_GENASTUBS);
		
		fprintf(outasm,
			STUBCODE,
			funclistit->name,
			cfg->libbase,
			&(__AROS_GETJUMPVEC(NULL, funclistit->lvo)->vec)
		 );
	    }
	
	    for (aliasesit = funclistit->aliases;
		 aliasesit != NULL;
		 aliasesit = aliasesit->next
	    )
	    {
		assert(cfg->intcfg & CFG_GENASTUBS);
		
		fprintf(outasm, ALIASCODE, funclistit->name, aliasesit->s);
	    }
	}
    }
    fclose(out);
    if (cfg->intcfg & CFG_GENASTUBS)
	fclose(outasm);
}
