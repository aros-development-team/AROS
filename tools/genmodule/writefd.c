/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
    
    Write the functionlist to a FD file for identify.library.
*/
#include "genmodule.h"

void writefd(struct config *cfg)
{
    FILE *out;
    char line[256];
    struct functionhead *funclistit;
    struct functionarg *arglistit;
    unsigned int lvo;

    snprintf(line, 255, "%s/%s_lib.fd", cfg->gendir, cfg->modulename);
    out = fopen(line, "w");

    if (out == NULL)
    {
	perror(line);
	exit(20);
    }

    /* When not a BOOPSI class write out the functionlist even if it is empty
     * when it is a BOOPSI write only the list when it is not empty
     * When cfg->basename != cfg->classlist->basename this means we are not in a BOOPSI class
     * but there are extra classes defined
     */
    if (cfg->classlist == NULL
	|| strcmp(cfg->basename, cfg->classlist->basename) != 0
	|| cfg->funclist != NULL)
    {
	fprintf(out, "##base _%s\n", cfg->libbase);
	fprintf(out, "##bias %u\n", cfg->firstlvo * 6);

	fprintf(out, "##public\n");
	
	for (funclistit = cfg->funclist, lvo = cfg->firstlvo - 1;
	     funclistit != NULL;
	     funclistit = funclistit->next
	)
	{
	    if (funclistit->libcall == REGISTERMACRO)
	    {
		if (funclistit->lvo > lvo + 1)
		{
		    if (funclistit->lvo == lvo + 2)
			fprintf(out, "\n");
		    else
			fprintf(out, ".skip %u\n", funclistit->lvo - lvo - 1);
		}
	    
		fprintf(out, "%s(", funclistit->name);
	    
		for (arglistit = funclistit->arguments;
		     arglistit!=NULL;
		     arglistit = arglistit->next
		)
		{
		    /* Print a , separator when not the first function argument */
		    if (arglistit != funclistit->arguments)
			fprintf(out, ", ");

		    /* FIXME: only the argument name should be printed */
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
			fprintf(out, ",");

		    fprintf(out, "%s", arglistit->reg);
		}
		fprintf(out, ")\n");

		lvo = funclistit->lvo;
	    }
	}

	fprintf(out, "##end\n");
    }

    if (ferror(out))
    {
	perror(line);
	exit(20);
    }

    fclose(out);
}
