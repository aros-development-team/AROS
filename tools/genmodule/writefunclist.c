/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id: writestart.c 23331 2005-05-28 11:38:57Z verhaegs $
    
    Write the functionlist to a file that can be includes the .conf file.
*/
#include "genmodule.h"

void writefunclist(struct config *cfg, struct functions *functions)
{
    FILE *out;
    char line[256];
    struct functionhead *funclistit;
    struct functionarg *arglistit;
    struct stringlist *aliaslistit;
    unsigned int lvo;
    
    snprintf(line, 255, "%s/%s.funclist", cfg->gendir, cfg->modulename);
    out = fopen(line, "w");

    if (out == NULL)
    {
        perror(line);
        exit(20);
    }

    fprintf(out, "##begin functionlist\n");
	
    for (funclistit = functions->funclist, lvo = cfg->firstlvo;
	 funclistit != NULL;
	 funclistit = funclistit->next
    )
    {
	if (funclistit->libcall == REGISTERMACRO)
	{
	    if (funclistit->lvo > lvo+1)
	    {
		if (funclistit->lvo == lvo+2)
		    fprintf(out, "\n");
		else
		    fprintf(out, ".skip %u\n", funclistit->lvo - lvo - 1);
	    }
	    
	    fprintf(out,
		    "%s %s(",
		    funclistit->type, funclistit->name
	    );
	    
	    for (arglistit = funclistit->arguments;
		 arglistit!=NULL;
		 arglistit = arglistit->next
	    )
	    {
	       /* Print a , separator when not the first function argument */
		if (arglistit != funclistit->arguments)
		    fprintf(out, ", ");

		fprintf(out, "%s", arglistit->arg);
	    }
	    fprintf(out, ") (");
	    
	    for (arglistit = funclistit->arguments;
		 arglistit != NULL;
		 arglistit = arglistit->next
	    )
	    {
	       /* Print a , separator when not the first function argument */
		if (arglistit != funclistit->arguments)
		    fprintf(out, ", ");

		fprintf(out, "%s", arglistit->reg);
	    }
	    fprintf(out, ")\n");

	    for (aliaslistit = funclistit->aliases;
		 aliaslistit != NULL;
		 aliaslistit = aliaslistit->next
	    )
	    {
		fprintf(out, ".alias %s\n", aliaslistit->s);
	    }
	    
	    if (funclistit->novararg)
		fprintf(out, ".novararg\n");
	    
	    if (funclistit->priv)
		fprintf(out, ".private\n");
	    
	    lvo = funclistit->lvo;
	}
    }

    fprintf(out, "##end functionlist\n");

    if (ferror(out))
    {
        perror(line);
        exit(20);
    }
    
    close(out);
}
