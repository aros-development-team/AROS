/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.

    Desc: function to read in the function reference file. Part of genmodule.
*/
#include "genmodule.h"
#include "fileread.h"
#include <assert.h>

void readref(void)
{
    struct functionlist *funclistit = NULL;
    char *begin, *end, *line;
    unsigned int len;

    if (!fileopen(reffile))
    {
	fprintf(stderr, "Could not open %s\n", reffile);
	exit(20);
    }

    while ((line=readline())!=NULL)
    {
	static char infunction=0;
	static struct arglist **arglistptr;
	
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
		    infunction = 0;
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
		    int i, brackets=0;
		    char *name;
		    
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
		    while (*(end-1)==']')
		    {
			brackets++;
			end--;
			while (isspace(*(end-1))) end--;
			if (*(end-1)!='[')
			{
			    fprintf(stderr, "Argument \"%s\" not understood for function %s\n",
				    begin, funclistit->name);
			    exit(20);
			}
			end--;
			while (isspace(*(end-1))) end--;
		    }
		    *end='\0';
		    while (!isspace(*(end-1)) && *(end-1)!='*') end--;
		    name = strdup(end);
		    while (isspace(*(end-1))) end--;
		    for (i=0; i<brackets; i++)
		    {
			*end='*';
			end++;
		    }
		    *end='\0';
		    if (strcasecmp(begin, "void")==0)
			free(name);
		    else
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
			(*arglistptr)->name = name;
			(*arglistptr)->type = strdup(begin);
			arglistptr = &((*arglistptr)->next);
		    }
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
    };
    fileclose();

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
}
