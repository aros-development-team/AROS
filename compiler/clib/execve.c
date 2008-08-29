/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function execve().
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include <__errno.h>
#include "__upath.h"

/* Return TRUE if there are any white spaces in the string */
BOOL containswhite(const char *str)
{
    while(*str != '\0')
    	if(isspace(*str++)) return TRUE;
    return FALSE;
}

/* Escape the string and quote it */ 
char *escape(const char *str)
{
    const char *strptr = str;
    char *escaped, *escptr;
    /* Additional two characters for '"', and one for '\0' */
    int bufsize = strlen(str) + 3;
    /* Take into account characters to ecape */
    while(*strptr != '\0')
    {
        switch(*strptr++)
        {
        case '\n':
        case '"':
        case '*':
        	bufsize++;
        }
    }
    escptr = escaped = (char*) malloc(bufsize);
    if(!escaped)
    	return NULL;
    *escptr++ = '"';
    for(strptr = str; *strptr != '\0'; strptr++)
    {
        switch(*strptr)
        {
        case '\n':
        case '"':
        case '*':
        	*escptr++ = '*';
        	*escptr++ = (*strptr == '\n' ? 'N' : *strptr);
        	break;
        default:
        	*escptr++ = *strptr;
        	break;
        }
    }
    *escptr++ = '"';
    *escptr = '\0';
    return escaped;
}

/* Append arg string to argptr increasing argptr if needed */
char * appendarg(char *argptr, int *argptrsize, const char *arg)
{
	while(strlen(argptr) + strlen(arg) + 2 > *argptrsize)
	{
		*argptrsize *= 2;
        argptr = realloc(argptr, *argptrsize);
        if(!argptr)
        	return NULL;
	}
	strcat(argptr, arg);
	strcat(argptr, " ");
	
	return argptr;
}

extern void __execve_exit(int returncode);

/*****************************************************************************

    NAME */
#include <unistd.h>

	int execve(

/*  SYNOPSIS */
	const char *filename,
	char *const argv[],
	char *const envp[])
        
/*  FUNCTION
	Executes a file with given name.

    INPUTS
        filename - Name of the file to execute.
        argv - Array of arguments provided to main() function of the executed
        file.
        envp - Array of environment variables passed as environment to the
        executed program.

    RESULT
	Returns -1 and sets errno appropriately in case of error, otherwise
	returns 0.

    NOTES

    EXAMPLE

    BUGS
	As opposed to *nix execve implementations, this implementation DOES
	RETURN to caller. Its purpose is to ease porting of programs using
	exec functions, you still have to make sure that after calling this
	function program will behave as anticipated.

    SEE ALSO
	
    INTERNALS

******************************************************************************/
{
    FILE *program;
    char firstline[128];    /* buffer to read first line of the script */
    char *inter = NULL;     /* interpreter in case of script */
    char *interargs = "";   /* interpreter arguments */
    char *linebuf;
    char *escaped;
    int argptrsize = 1024;
    char *argptr = (char*) malloc(argptrsize); /* arguments buffer for 
                                                   RunCommand() */
    char **arg, **env;
    char *varname, *varvalue;
    BPTR seglist;
    int returncode;
    struct CommandLineInterface *cli = Cli();
    const char *afilename;
    int saved_errno = 0;
    char *oldtaskname;

    /* Sanity check */
    if (filename == NULL || filename[0] == '\0' || argv == NULL || envp == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    /* Let's check if it's a script */
    if((program = fopen(filename, "r")))
    {
    	if(fgetc(program) == '#' && fgetc(program) == '!')
    	{
    		/* It is a script, let's read the first line */
    		if(fgets(firstline, sizeof(firstline) - 1, program))
    		{
    			linebuf = firstline;
    		    while(isblank(*linebuf)) linebuf++;
    		    if(*linebuf != '\0')
    		    {
    		    	/* Interpreter name is here */
    		    	inter = linebuf;
    		    	while(*linebuf != '\0' && !isblank(*linebuf)) linebuf++;
    		    	if(*linebuf != '\0')
    		    	{
    		    		*linebuf++ = '\0';
    		    		while(isblank(*linebuf)) linebuf++;
    	    		    if(*linebuf != '\0')
    	    		    	/* Interpreter arguments are here */
                            interargs = linebuf;   		    		
    		    	}
    		    }
    		}
    	}
    	fclose(program);
    }
    else
    {
        /* Simply assume it doesn't exist */
        saved_errno = ENOENT;
        goto error;
    }

    /* Build RunCommand argument string by escaping and appending all
       arguments */
    argptr[0] = '\0';
    arg = (inter != NULL ? &interargs : (char**) argv + 1);
    while(*arg)
    {
        if(containswhite(*arg))
        {
            escaped = escape(*arg);
            if(!escaped) {
            	saved_errno = ENOMEM;
            	goto error;
            }
            argptr = appendarg(argptr, &argptrsize, escaped);
            free(escaped);
        }
        else
            argptr = appendarg(argptr, &argptrsize, *arg);

        if(!argptr) {
        	saved_errno = ENOMEM;
        	goto error;
        }
        
        /* If interpreter args were first then we have to insert script name
           here */
        if(arg == &interargs)
        {
            argptr = appendarg(argptr, &argptrsize, filename);
            if(!argptr) {
            	saved_errno = ENOMEM;
            	goto error;
            }
            /* Follow with argv */
            arg = (char**) argv;
        }
        else
        	arg++;
    }
    /* End argptr with '\n' */
    if(strlen(argptr) > 0)
    	argptr[strlen(argptr) - 1] = '\n';
    else
    	strcat(argptr, "\n");
    
    /* Store environment variables */
    env = (char**) envp;
    while(*env)
    {
    	varvalue = strchr(*env, '=');
    	if(!varvalue || varvalue == *env)
    	{
    		/* No variable value? Empty variable name? */
    		saved_errno = EINVAL;
    		goto error;
    	}
    	varname = malloc(1 + varvalue - *env);
    	if(!varname)
    	{
    		saved_errno = ENOMEM;
    		goto error;
    	}
        varname[0] = '\0';
    	strncat(varname, *env, varvalue - *env);
    	setenv(varname, varvalue, 1);
    	free(varname);
    	env++;
    }
    
    /* Get the path for LoadSeg() */
    afilename = __path_u2a(inter ? inter : (char*) filename);
    if(!afilename)
    {
    	saved_errno = errno;
    	goto error;
    }
    
    /* let's make some sanity tests */
    
    struct stat st;
    if(stat(inter ? inter : (char*) filename, &st) == 0)
    {
	if(!(st.st_mode & S_IXUSR) && !(st.st_mode & S_IXGRP) && !(st.st_mode & S_IXOTH))
	{
	    /* file is not executable */
	    saved_errno = EACCES;
	    goto error;
	}
    }
    else
    {
	/* couldn't stat file */
	saved_errno = errno;
	goto error;
    }
    
    /* Load and run the command */
    if((seglist = LoadSeg(afilename)))
    {
    	oldtaskname = FindTask(NULL)->tc_Node.ln_Name;
    	FindTask(NULL)->tc_Node.ln_Name = (afilename);
        SetProgramName(inter ? inter : argv[0]);
        returncode = RunCommand(
            seglist,
            cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT, 
            argptr,
            strlen(argptr)
        );
        FindTask(NULL)->tc_Node.ln_Name = oldtaskname;
        UnLoadSeg(seglist);
        return 0;
    }
    else
    {
	/* most likely file is not a executable */
    	saved_errno = ENOEXEC;
    	goto error;
    }

error:
    free(argptr);
    if(saved_errno) errno = saved_errno;
    return -1;
} /* execve() */
