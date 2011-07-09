/*
    Copyright � 1995-2009, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to write module_stubs.c. Part of genmodule.
*/
#include "genmodule.h"

void writestubs(struct config *cfg)
{
    FILE *out;
    char line[256], *type, *name, *banner;
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

    banner = getBanner(cfg);
    fprintf
    (
        out,
        "%s"
        "#define NOLIBDEFINES\n"
        "#define NOLIBINLINE\n"
        "/* Be sure that the libbases are included in the stubs file */\n"
        "#undef __NOLIBBASE__\n"
        "#undef __%s_NOLIBBASE__\n",
        banner, cfg->modulenameupper
    );
    freeBanner(banner);

    if (cfg->modtype != MCC && cfg->modtype != MUI && cfg->modtype != MCP)
    {
        fprintf(out, "#include <proto/%s.h>\n", cfg->modulename);
    }
    
    fprintf
    (
        out,
        "#include <exec/types.h>\n"
	"#include <aros/cpu.h>\n"
        "#include <aros/libcall.h>\n"
        "\n"
    );
    
    for (funclistit = cfg->funclist;
	 funclistit!=NULL;
	 funclistit = funclistit->next
    )
    {
        if (funclistit->lvo >= cfg->firstlvo)
	{
	    if (funclistit->libcall != STACK)
	    {
	    	int nargs = 0, nquad = 0;
		int isvoid = strcmp(funclistit->type, "void") == 0
		    || strcmp(funclistit->type, "VOID") == 0;

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
		    if (strchr(arglistit->reg, '/')) {
		    	nquad++;
		    } else {
		    	nargs++;
		    }
		}

		if (nquad == 0)
		{
                    fprintf(out,
                            ")\n"
                            "{\n"
                            "    %sAROS_LC%d%s(%s, %s,\n",
                            (isvoid) ? "" : "return ",
                            funclistit->argcount, (isvoid) ? "NR" : "",
                            funclistit->type, funclistit->name
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
                }
                else /* nquad != 0 */
                {
                    if (nargs == 0) {
                        fprintf(out,
                                ") \\\n"
                                "{\n"
                                "    %sAROS_LCQUAD%d%s(%s, %s, \\\n",
                                (isvoid) ? "" : "return ",
                                funclistit->argcount, (isvoid) ? "NR" : "",
                                funclistit->type, funclistit->name
                        );
                    }
                    else
                    {
                        fprintf(out,
                                ") \\\n"
                                "{\n"
                                "    %sAROS_LC%dQUAD%d%s(%s, %s, \\\n",
                                (isvoid) ? "" : "return ",
                                nargs, nquad, (isvoid) ? "NR" : "",
                                funclistit->type, funclistit->name
                        );
                    }

                    for (arglistit = funclistit->arguments;
                         arglistit != NULL;
                         arglistit = arglistit->next
                    )
                    {
                        char *quad2 = strchr(arglistit->reg, '/');

                        type = getargtype(arglistit);
                        name = getargname(arglistit);
                        assert(type != NULL && name != NULL);
		
                        if (quad2) {
                            *quad2 = 0;
                            fprintf(out,
                                    "         AROS_LCAQUAD(%s, %s, %s, %s), \\\n",
                                    type, name, arglistit->reg, quad2+1
			    );
                            *quad2 = '/';
                        }
                        else
                        {
                            fprintf(out,
                                    "         AROS_LCA(%s, %s, %s), \\\n",
                                    type, name, arglistit->reg
                            );
                        }
                        free(type);
                        free(name);
                    }
		}
 
		fprintf(out, "                    %s, %s, %u, %s);\n}\n",
			cfg->libbasetypeptrextern, cfg->libbase, funclistit->lvo, cfg->basename
                );
	    }
	    else /* libcall==STACK */
	    {
		fprintf(out, "AROS_LIBFUNCSTUB(%s, %s, %d)\n",
			funclistit->name, cfg->libbase,	funclistit->lvo
		);
	    }
	
	    for (aliasesit = funclistit->aliases;
		 aliasesit != NULL;
		 aliasesit = aliasesit->next
	    )
	    {
		fprintf(out, "AROS_FUNCALIAS(%s, %s)\n",
			funclistit->name, aliasesit->s
		);
	    }
	}
    }
    fclose(out);
}
