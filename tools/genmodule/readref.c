/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: function to read in the function reference file. Part of genmodule.
*/
#include "genmodule.h"
#include <assert.h>

void readref(void)
{
    FILE *in;
    struct functionlist *funclistit = NULL;
    char *begin, *end;
    unsigned int len;
    char *initfunction, *openfunction, *closefunction, *expungefunction, *s;

    s = "_L_InitLib";
    len = strlen(basename)+strlen(s)+1;
    initfunction = malloc(len);
    snprintf(initfunction, len, "%s%s", basename, s);
    s = "_L_OpenLib";
    len = strlen(basename)+strlen(s)+1;
    openfunction = malloc(len);
    snprintf(openfunction, len, "%s%s", basename, s);
    s = "_L_CloseLib";
    len = strlen(basename)+strlen(s)+1;
    closefunction = malloc(len);
    snprintf(closefunction, len, "%s%s", basename, s);
    s = "_L_ExpungeLib";
    len = strlen(basename)+strlen(s)+1;
    expungefunction = malloc(len);
    snprintf(expungefunction, len, "%s%s", basename, s);

    in = fopen(reffile, "r");
    if (in==NULL)
    {
	fprintf(stderr, "Could not open %s\n", reffile);
	exit(20);
    }
    do
    {
	static char infunction=0;
	static struct arglist **arglistptr;
	
	readline(in);
	if (strlen(line)>0)
	{
	    if (infunction &&
		(strncmp(line, "FILE", 4)==0 ||
		 strncmp(line, "INCLUDES", 8)==0 ||
		 strncmp(line, "DEFINES", 7)==0 ||
		 strncmp(line, "VARIABLE", 8)==0 ||
		 strncmp(line, "FUNCTION", 8)==0
		) &&
		libcall==REGISTER)
	    {
		/* About to leave function */
		if ((*arglistptr)!=NULL)
		{
		    fprintf(stderr, "Error: too many registers specified for function \"%s\"\n",
			    funclistit->name);
		    exit(20);
		}
	    }
	    
	    if (infunction &&
		(strncmp(line, "FILE", 4)==0 ||
		 strncmp(line, "INCLUDES", 8)==0 ||
		 strncmp(line, "DEFINES", 7)==0 ||
		 strncmp(line, "VARIABLE", 8)==0
		)
	       )
		infunction = 0;

	    if (strncmp(line, "FUNCTION", 8)==0)
	    {
		begin = strchr(line,':');
		if (begin==NULL)
		{
		    fprintf(stderr, "Syntax error in reffile %s\n", reffile);
		    exit(20);
		}
		begin++;
		while (isspace(*begin)) begin++;
		end = strchr(begin, '[');
		if (end==NULL)
		{
		    fprintf(stderr, "Syntax error in reffile %s\n", reffile);
		    exit(20);
		}
		while (isspace(*(end-1))) end--;
		*end = '\0';

		for (funclistit = funclist;
		     funclistit!=NULL && strcmp(funclistit->name, begin)!=0;
		     funclistit = funclistit->next)
		    ;

		if (funclistit==NULL)
		{
		    infunction = 0;
		    if (strcmp(begin, initfunction)==0)
			hasinit = 1;
		    if (strcmp(begin, openfunction)==0)
			hasopen = 1;
		    if (strcmp(begin, closefunction)==0)
			hasclose = 1;
		    if (strcmp(begin, expungefunction)==0)
			hasexpunge = 1;
		}
		else
		{
		    infunction = 1;
		    arglistptr = &(funclistit->arguments);
		}
	    }
	    else if (infunction)
	    {
		if (strncmp(line, "Arguments", 9)==0)
		{
		    switch (libcall)
		    {
		    case STACK:
			assert(*arglistptr==NULL);
			*arglistptr = malloc(sizeof(struct arglist));
			(*arglistptr)->next = NULL;
			(*arglistptr)->reg = "";
			funclistit->argcount++;
			break;
			
		    case REGISTER:
			if (*arglistptr==NULL)
			{
			    fprintf(stderr, "Error: not enough register specified for function \"%s\"\n",
				    funclistit->name);
			    exit(20);
			}
			break;
			
		    default:
			fprintf(stderr, "Internal error: unhandled libcall type in readref\n");
			exit(20);
			break;
		    }
		    begin = strchr(line, ':');
		    if (begin==NULL)
		    {
			fprintf(stderr, "Syntax error in reffile %s\n", reffile);
			exit(20);
		    }
		    begin++;
		    while (isspace(*begin)) begin++;
		    end = begin+strlen(begin);
		    while (isspace(*(end-1))) end--;
		    while (!isspace(*(end-1)) && *(end-1)!='*') end--;
		    (*arglistptr)->name = strdup(end);
		    while (isspace(*(end-1))) end--;
		    *end='\0';
		    (*arglistptr)->type = strdup(begin);
		    arglistptr = &((*arglistptr)->next);
		}
		else if (strncmp(line, "Type", 4)==0)
		{
		    begin = strchr(line, ':');
		    if (begin==NULL)
		    {
			fprintf(stderr, "Syntax error in reffile %s\n", reffile);
			exit(20);
		    }
		    begin++;
		    while (isspace(*begin)) begin++;
		    end = begin+strlen(begin)-strlen(funclistit->name);
		    while (isspace(*(end-1))) end--;
		    *end = '\0';
		    funclistit->type = strdup(begin);
		}
	    }
	}
    } while (!feof(in));
    fclose(in);

    /* Checking to see if every function has a prototype */
    for (funclistit = funclist;
	 funclistit!=NULL;
	 funclistit = funclistit->next)
    {
	if (funclistit->type==NULL)
	{
	    fprintf(stderr, "Did not find function %s in reffile %s\n", funclistit->name, reffile);
	    exit(20);
	}
    }
    
    free(initfunction);
    free(openfunction);
    free(closefunction);
    free(expungefunction);
}
