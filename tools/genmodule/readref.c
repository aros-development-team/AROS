/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
    
    Function to read in the function reference file. Part of genmodule.
*/
#include "genmodule.h"
#include "fileread.h"
#include <assert.h>

/* A structure for storing parse state information during scanning of the ref file */
struct _parseinfo
{
    char infunction;
    char newarg;
    struct arglist **arglistptr;
    struct functionlist *currentfunc;
};

void readref(void)
{
    struct functionlist *funclistit = NULL;
    struct functionlist *currentfunc = NULL; /* will be either funclistit or methlistit */
    struct _parseinfo parseinfo;
    unsigned int funcnamelength;
    char *begin, *end, *line;

    if (!fileopen(reffile))
    {
	fprintf(stderr, "Could not open %s\n", reffile);
	exit(20);
    }

    /* Go over all the lines in the ref file and look for the functions and their
     * arguments in the file. There are two states during parsing infunction==0 for
     * outside a function header and then just look for the next function name.
     * infunction==1 in a function header and then the arguments of this function
     * are parsed.
     */
    parseinfo.infunction = 0;
    while ((line=readline())!=NULL)
    {
	if (strlen(line)>0)
	{
	    /* When the registers are specified in the config file check if the number provided
	     * provided in the file corresponds with the number of arguments in the ref file
	     */
	    if (parseinfo.infunction &&
		(strncmp(line, "FILE", 4)==0 ||
		 strncmp(line, "INCLUDES", 8)==0 ||
		 strncmp(line, "DEFINES", 7)==0 ||
		 strncmp(line, "VARIABLE", 8)==0 ||
		 strncmp(line, "FUNCTION", 8)==0
		) &&
		libcall==REGISTER)
	    {
		/* About to leave function */
		if ((*parseinfo.arglistptr)!=NULL)
		{
		    fprintf(stderr, "Error: too many registers specified for function \"%s\"\n",
			    parseinfo.currentfunc->name);
		    exit(20);
		}
	    }

	    /* End of function header ? */
	    if (parseinfo.infunction &&
		(strncmp(line, "FILE", 4)==0 ||
		 strncmp(line, "INCLUDES", 8)==0 ||
		 strncmp(line, "DEFINES", 7)==0 ||
		 strncmp(line, "VARIABLE", 8)==0
		)
	       )
		parseinfo.infunction = 0;

	    /* Start of a new function header ? */
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

		funcnamelength = strlen(begin);
		
		parseinfo.infunction =
		(
		       parsemuimethodname(begin, &parseinfo)
		    || parsemacroname(begin, &parseinfo)
		    || parsefunctionname(begin, &parseinfo)
		);
	    }
	    else if (parseinfo.infunction)
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
				    begin, parseinfo.currentfunc->name);
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
			if (parseinfo.newarg)
                        {
			    assert(*parseinfo.arglistptr==NULL);
			    *parseinfo.arglistptr = malloc(sizeof(struct arglist));
			    (*parseinfo.arglistptr)->next = NULL;
			    (*parseinfo.arglistptr)->reg = "";
			    parseinfo.currentfunc->argcount++;
			}
			else
			{
			    if (*parseinfo.arglistptr==NULL)
			    {
				fprintf(stderr, "Error: argument count mismatch for funtion \"%s\"\n",
                                            parseinfo.currentfunc->name);
				exit(20);
			    }
			}
			
			(*parseinfo.arglistptr)->name = name;
			(*parseinfo.arglistptr)->type = strdup(begin);
			parseinfo.arglistptr = &((*parseinfo.arglistptr)->next);
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
		    end = begin+strlen(begin)-funcnamelength;
		    *end = '\0';
		    parseinfo.currentfunc->type = strdup(begin);
		}
	    }
	}
    };
    fileclose();

    /* For REGISTERMACRO libcall the name of the register is still in the parametername
     * so go over the function and fix this
     */
    if (libcall == REGISTERMACRO)
    {
	struct arglist * arglistit;
	char *s;
	
	for
	(
	    funclistit = funclist;
	    funclistit != NULL;
	    funclistit = funclistit->next
	)
	{
	    for
	    (
	        arglistit = funclistit->arguments;
	        arglistit != NULL;
	        arglistit = arglistit->next
	    )
	    {
		s = arglistit->name + strlen(arglistit->name) - 1;
		while (s != arglistit->name && *s != '_') s--;
		
		*s = '\0';
		
		arglistit->reg = strdup(s+1);
	    }
	}
    }
	    
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


int parsemuimethodname(char *name,
		       struct _parseinfo *parseinfo)
{
    int ok = 0;
    
    if (modtype == MCC || modtype == MUI || modtype == MCP)
    {
	char *sep;
	
	/* For a MUI class a custom dispatcher has the name
	 * 'modulename_Dispatcher'
	 */
	if
	(
	       strncmp(name, modulename, strlen(modulename)) == 0
	    && strcmp(name+strlen(modulename), "_Dispatcher") == 0
	)
	    customdispatcher = 1;	
	    
	sep = strstr(name, "__OM_");
	if (sep == NULL) sep = strstr(name, "__MUIM_");
                
	if 
	(
	       sep != NULL
	    && strncmp(modulename, name, sep - name) == 0
	)
	{
	    struct functionlist *method, *it;

	    method = malloc(sizeof(struct functionlist));
	    method->next = NULL;
	    method->name = strdup(sep + 2); 
	    method->type = NULL; /* not known yet */
	    method->argcount = 0;
	    method->arguments = NULL;
	    method->lvo = 0; /* not used */
                    
	    if (methlist == NULL )
		methlist = method;
	    else
	    {
		it = methlist;
		
		while (it->next != NULL) it = it->next;
		
		it->next = method;
	    }
                    
	    parseinfo->arglistptr = &(method->arguments);
	    parseinfo->currentfunc = method;
	    ok = 1;
	    parseinfo->newarg = 1;
	}
    }

    return ok;
}


int parsemacroname(char *name,
		   struct _parseinfo *parseinfo)
{
    if (libcall == REGISTERMACRO && strncmp(name, "AROS_LH_", 8) == 0)
    {
	struct functionlist *funclistit, *func;
	char *begin, *end;

	begin = name+8;
	if (strncmp(begin, basename, strlen(basename)) != 0 || begin[strlen(basename)] != '_')
	    return 0;
	
	begin = begin + strlen(basename) + 1;
	end = begin + strlen(begin) - 1;
	
	func = malloc(sizeof(struct functionlist));
	func->argcount = 0;
	func->arguments = NULL;
	
	while(end != begin && *end != '_') end--;
	*end = '\0';
	func->name = strdup(begin);
	
	begin = end+1;
	sscanf(begin, "%d", &func->lvo);

	if (funclist == NULL || funclist->lvo > func->lvo)
	{
	    func->next = funclist;
	    funclist = func;
	}
	else
	{
	    for
	    (
	        funclistit = funclist;
	        funclistit->next != NULL && funclistit->next->lvo < func->lvo;
	        funclistit = funclistit->next
	    )
		;
	 
	    if (funclistit->next != NULL && funclistit->next->lvo == func->lvo)
	    {
		fprintf(stderr, "Function '%s' and '%s' have the same LVO number\n",
			funclistit->next->name, func->name);
		exit(20);
	    }
		
	    func->next = funclistit->next;
	    funclistit->next = func;
	}

	parseinfo->currentfunc = func;
	parseinfo->newarg = 1;
	parseinfo->arglistptr = &(func->arguments);
	
	return 1;
    }
    else
	return 0;
}

int parsefunctionname(char *name,
		      struct _parseinfo *parseinfo)
{
    struct functionlist *funclistit;
    
    if (libcall == REGISTERMACRO)
	return 0;
    
    for (funclistit = funclist;
	 funclistit!=NULL && strcmp(funclistit->name, name)!=0;
	 funclistit = funclistit->next)
	;
    
    if (funclistit==NULL)
	return 0;
    else
    {
	parseinfo->arglistptr = &(funclistit->arguments);
	parseinfo->currentfunc = funclistit;
	
	switch (libcall)
	{
	case STACK:
	    parseinfo->newarg = 1;
	    break;
                                
	case REGISTER:
	    parseinfo->newarg = 0;
	    break;
                                
	default:
	    fprintf(stderr, "Internal error: unhandled libcall type in parsefunctionname\n");
	    exit(20);
	    break;
	}

	return 1;
    }
}
