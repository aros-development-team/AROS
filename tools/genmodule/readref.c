/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to read in the function reference file. Part of genmodule.
*/
#include "genmodule.h"
#include "fileread.h"
#include <assert.h>

void readref(void)
{
    struct functionlist *funclistit = NULL, *methlistit = NULL;
    struct functionlist *list = NULL; /* will be either funclistit or methlistit */
    char *begin, *end, *line, *sep;
    unsigned int len;

    if (!fileopen(reffile))
    {
	fprintf(stderr, "Could not open %s\n", reffile);
	exit(20);
    }

    while ((line=readline())!=NULL)
    {
	static char infunction = 0, inmethod = 0;
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
                
                sep = strstr(begin, "__OM_");
                if (sep == NULL) sep = strstr(begin, "__MUIM_");
                
                if 
                (
                       (modtype == MCC || modtype == MUI || modtype == MCP)
                    && sep != NULL
                    && strncmp(modulename, begin, sep - begin) == 0
                )
		{
                    struct functionlist *method;
                    infunction = 1; 
                    
                    method = malloc(sizeof(struct functionlist));
                    method->next = NULL;
                    method->name = strdup(sep + 2); 
                    method->type = NULL; /* not known yet */
                    method->argcount = 0;
                    method->arguments = NULL;
                    method->lvo = 0; /* not used */
                    
                    if (methlistit != NULL)
                        methlistit->next = method;
                        
                    methlistit = method;
                    
                    if (methlist == NULL )
                        methlist = methlistit;
                    
                    arglistptr = &(methlistit->arguments);
                    list = methlistit;
                }
                else
                {
                    for (funclistit = funclist;
                         funclistit!=NULL && strcmp(funclistit->name, begin)!=0;
                         funclistit = funclistit->next)
                        ;
    
                    if (funclistit==NULL)
                    {
                        infunction = 0;
                    }
                    else
                    {
                        infunction = 1;
                        
                        arglistptr = &(funclistit->arguments);
                        list = funclistit;
                    }
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
				    begin, list->name);
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
			if (list == funclistit)
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
                        }
                        else /* method */
                        {
                            *arglistptr = malloc(sizeof(struct arglist));
                            (*arglistptr)->next = NULL;
                            (*arglistptr)->reg = "";
                            methlistit->argcount++;
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
		    if (list == funclistit)
                        end = begin+strlen(begin)-strlen(list->name);
		    else /* methlistit */
                        end = begin+strlen(begin)-(strlen(list->name)+strlen(modulename)+2);
                    while (isspace(*(end-1))) end--;
		    *end = '\0';
		    list->type = strdup(begin);
		}
	    }
	}
    };
    fileclose();

    /* Checking to see if every function has a prototype */
    for 
    (
        funclistit =  funclist; 
        funclistit != NULL; 
        funclistit =  funclistit->next
    )
    {
	if (funclistit->type==NULL)
	{
	    if 
            (
                   (modtype == MCC || modtype == MUI || modtype == MCP) 
                && strcmp(funclistit->name, "MCC_Query") == 0 
            )
            {
                struct arglist *arglistit = funclistit->arguments;
                
                if (arglistit == NULL)
                {
                    fprintf(stderr, "Wrong number of arguments for MCC_Query");
                    exit(20);
                }
                
                funclistit->type = "IPTR";
                
                arglistit->type = "LONG";
                arglistit->name = "what";
            }
            else
            {
                fprintf
                (
                    stderr, "Did not find function %s in reffile %s\n", 
                    funclistit->name, reffile
                );
                exit(20);
            }
	}
    }
}

void readrefmcc(void)
{
    char dispatcher[512];
    char *line = NULL;
    
    snprintf(dispatcher, 511, "FUNCTION : %s_Dispatcher", modulename);
    dispatcher[511] = '\0';
    
    if (!fileopen(reffile))
    {
	fprintf(stderr, "Could not open %s\n", reffile);
	exit(20);
    }
   
    while ((line = readline()) != NULL)
    {
        static char instruct = 0;
        
        if (strncmp(line, dispatcher, strlen(dispatcher)) == 0)
        {
            customdispatcher = 1;
            break;
        }
    }
}
