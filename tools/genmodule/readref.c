/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
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
    struct functionarg *funcarg;
    struct functionhead *currentfunc;
};

void readref(struct config *cfg)
{
    struct functionhead *funclistit = NULL;
    struct functionhead *currentfunc = NULL; /* will be either funclistit or methlistit */
    struct _parseinfo parseinfo;
    unsigned int funcnamelength;
    char *begin, *end, *line;

    if (!fileopen(cfg->reffile))
    {
	fprintf(stderr, "Could not open %s\n", cfg->reffile);
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
		cfg->libcall==REGISTER)
	    {
		/* About to leave function */
		if (parseinfo.funcarg!=NULL)
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
		    fprintf(stderr, "Syntax error in reffile %s\n", cfg->reffile);
		    exit(20);
		}
		begin++;
		while (isspace(*begin)) begin++;
		end = strchr(begin, '[');
		if (end==NULL)
		{
		    fprintf(stderr, "Syntax error in reffile %s\n", cfg->reffile);
		    exit(20);
		}
		while (isspace(*(end-1))) end--;
		*end = '\0';

		funcnamelength = strlen(begin);
		
		parseinfo.infunction =
		(
		       parsemuimethodname(begin, &parseinfo, cfg)
		    || parsemacroname(begin, &parseinfo, cfg)
		    || parsefunctionname(begin, &parseinfo, cfg)
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
			fprintf(stderr, "Syntax error in reffile %s\n", cfg->reffile);
			exit(20);
		    }
		    begin++;
		    while (isspace(*begin)) begin++;

		    /* for libcall == STACK the whole argument is the type
		     * otherwise split the argument in type and name
		     */
		    if (cfg->libcall != STACK)
		    {
			/* Count the [] specification at the end of the argument */
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
			
			/* Skip over the argument name and duplicate it */
			while (!isspace(*(end-1)) && *(end-1)!='*') end--;
			name = strdup(end);

			/* Add * for the brackets */
			while (isspace(*(end-1))) end--;
			for (i=0; i<brackets; i++)
			{
			    *end='*';
			    end++;
			}
			*end='\0';
		    }

		    if (strcasecmp(begin, "void")==0)
		    {
			if (cfg->libcall != STACK)
			    free(name);
		    }
		    else
		    {
			if (parseinfo.newarg)
			    parseinfo.funcarg = funcaddarg(parseinfo.currentfunc, NULL, NULL, "");
			else
			{
			    if (parseinfo.funcarg==NULL)
			    {
				fprintf(stderr, "Error: argument count mismatch for funtion \"%s\"\n",
                                            parseinfo.currentfunc->name);
				exit(20);
			    }
			}
			
			if (cfg->libcall == STACK)
			{
			    parseinfo.funcarg->type = strdup(begin);
			    parseinfo.funcarg->name = NULL;
			}
			else
			{
			    parseinfo.funcarg->type = strdup(begin);
			    parseinfo.funcarg->name = name;
			}
			
			parseinfo.funcarg = parseinfo.funcarg->next;
		    }
		}
		else if (strncmp(line, "Type", 4)==0)
		{
		    begin = strchr(line, ':');
		    if (begin==NULL)
		    {
			fprintf(stderr, "Syntax error in reffile %s\n", cfg->reffile);
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
    if (cfg->libcall == REGISTERMACRO)
    {
	struct functionarg * arglistit;
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
                   (cfg->modtype == MCC || cfg->modtype == MUI || cfg->modtype == MCP) 
                && strcmp(funclistit->name, "MCC_Query") == 0 
            )
            {
                struct functionarg *arglistit = funclistit->arguments;
                
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
                    funclistit->name, cfg->reffile
                );
                exit(20);
            }
	}
    }
}


int parsemuimethodname(char *name,
		       struct _parseinfo *parseinfo,
		       struct config *cfg)
{
    int ok = 0;
    
    if (cfg->modtype == MCC || cfg->modtype == MUI || cfg->modtype == MCP)
    {
	char *sep;
	
	/* For a MUI class a custom dispatcher has the name
	 * 'modulename_Dispatcher'
	 */
	if
	(
	       strncmp(name, cfg->modulename, strlen(cfg->modulename)) == 0
	    && strcmp(name+strlen(cfg->modulename), "_Dispatcher") == 0
	)
	    cfg->customdispatcher = 1;	
	    
	sep = strstr(name, "__OM_");
	if (sep == NULL) sep = strstr(name, "__MUIM_");
                
	if 
	(
	       sep != NULL
	    && strncmp(cfg->modulename, name, sep - name) == 0
	)
	{
	    struct functionhead *method, *it;

	    method = newfunctionhead(sep+2, NULL, 0);
	    
	    if (methlist == NULL )
		methlist = method;
	    else
	    {
		it = methlist;
		
		while (it->next != NULL) it = it->next;
		
		it->next = method;
	    }
                    
	    parseinfo->funcarg = method->arguments;
	    parseinfo->currentfunc = method;
	    ok = 1;
	    parseinfo->newarg = 1;
	}
    }

    return ok;
}


int parsemacroname(char *name,
		   struct _parseinfo *parseinfo,
		   struct config *cfg)
{
    if
    (
        cfg->libcall == REGISTERMACRO && 
	(
	    strncmp(name, "AROS_LH_", 8)    == 0 || 
	    strncmp(name, "AROS_PLH_", 9)   == 0 || 
	    strncmp(name, "AROS_NTLH_", 10) == 0
        ) 
    ) 
    {
	struct functionhead *funclistit, *func;
	char *begin, *end, *funcname;
	int novararg = 0, priv = 0;
	unsigned int lvo;
	
	if (strncmp(name, "AROS_LH_", 8) == 0)
	{
	    begin = name+8;
	}
	else
	if (strncmp(name, "AROS_PLH_", 9) == 0)
	{
	    begin = name+9;
	    priv = 1;
	}
	else
	{
	    begin = name+10;
	    novararg = 1;
	}
	
	if (strncmp(begin, cfg->basename, strlen(cfg->basename)) != 0
	    || begin[strlen(cfg->basename)] != '_'
	)
	    return 0;
	
	begin = begin + strlen(cfg->basename) + 1;
	end = begin + strlen(begin) - 1;
	
	while(end != begin && *end != '_') end--;
	*end = '\0';
	funcname = begin;
	
	begin = end+1;
	sscanf(begin, "%d", &lvo);

	func = newfunctionhead(funcname, NULL, lvo);
	func->novararg = novararg;
	func->priv     = priv;

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
	parseinfo->funcarg = func->arguments;
	
	return 1;
    }
    else
	return 0;
}

int parsefunctionname(char *name,
		      struct _parseinfo *parseinfo,
		      struct config *cfg)
{
    struct functionhead *funclistit;
    
    if (cfg->libcall == REGISTERMACRO)
	return 0;
    
    for (funclistit = funclist;
	 funclistit!=NULL && strcmp(funclistit->name, name)!=0;
	 funclistit = funclistit->next)
	;
    
    if (funclistit==NULL)
	return 0;
    else
    {
	parseinfo->funcarg = funclistit->arguments;
	parseinfo->currentfunc = funclistit;
	
	switch (cfg->libcall)
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
