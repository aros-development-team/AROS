/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function execve().
*/

#define DEBUG 0

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/debug.h>
#include <dos/filesystem.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <__errno.h>
#include "__upath.h"
#include "__open.h"
#include "__arosc_privdata.h"
#include "__vfork.h"

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

BPTR DupFHFromfd(int fd, ULONG mode);

LONG exec_command(BPTR seglist, char *taskname, char *args, ULONG stacksize)
{
    BPTR oldin = MKBADDR(NULL), oldout = MKBADDR(NULL), olderr = MKBADDR(NULL);
    BPTR in, out, err;
    char *oldtaskname;
    APTR udata;
    LONG returncode;
        
    in  = DupFHFromfd(STDIN_FILENO,  FMF_READ);
    out = DupFHFromfd(STDOUT_FILENO, FMF_WRITE);
    err = DupFHFromfd(STDERR_FILENO, FMF_WRITE);
    
    if(in) 
        oldin = SelectInput(in);
    if(out) 
        oldout = SelectOutput(out);
    if(err)
        olderr = SelectError(err);

    oldtaskname = FindTask(NULL)->tc_Node.ln_Name;
    FindTask(NULL)->tc_Node.ln_Name = taskname;
    SetProgramName(taskname);

    udata = FindTask(NULL)->tc_UserData;
    FindTask(NULL)->tc_UserData = NULL;
    returncode = RunCommand(
        seglist,
        stacksize, 
        args,
        strlen(args)
    );
    FindTask(NULL)->tc_UserData = udata;

    FindTask(NULL)->tc_Node.ln_Name = oldtaskname;
    SetProgramName(oldtaskname);
    UnLoadSeg(seglist);
        
    if(in)
    {
	Close(in);
        SelectInput(oldin);
    }
    if(out)
    {
	Close(out);
        SelectOutput(oldout);
    }
    if(err)
    {
	Close(err);
	SelectError(olderr);
    }
    
    return returncode;
}

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
	doesn't return.

    NOTES

    EXAMPLE

    BUGS

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
    char *afilename = NULL;
    int saved_errno = 0;

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
    		    /* delete end of line if present */
    		    if(firstline[0] && firstline[strlen(firstline)-1] == '\n')
    			firstline[strlen(firstline)-1] = '\0';
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
            arg = (char**) argv + 1;
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
    afilename = strdup(__path_u2a(inter ? inter : (char*) filename));
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
    if((seglist = LoadSeg((CONST_STRPTR) afilename)))
    {
        free(afilename);
        
        struct vfork_data *udata = FindTask(NULL)->tc_UserData;
	if(udata && udata->magic == VFORK_MAGIC)
	{
	    /* This execve() was called from vfork()ed process. We are going
	       to switch from stack borrowed from the parent process to the 
	       original stack of this process. First we have to store all
	       local variables in udata to let them survive the stack change */
	    udata->exec_seglist = seglist;
	    udata->exec_arguments = argptr;
	    udata->exec_stacksize = cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT;
	    udata->exec_taskname = (inter ? inter : argv[0]);
	    
	    /* Set this so we know that execve was called */
	    udata->child_executed = 1;

	    D(bug("Restoring child stack to %p < %p < %p\n", 
		udata->child_SPLower, 
		udata->child_SPReg, 
		udata->child_SPUpper)
	    );

            /* Restoring child stack */
	    Forbid();
	    udata->child->tc_SPLower = udata->child_SPLower;
	    udata->child->tc_SPUpper = udata->child_SPUpper;
	    /* Since we are switching to child's stack and in return jmp_buf we
	       have parent's stack pointer saved, we have to set stack value in
	       jmp_buf to child's stack pointer, otherwise it will be outside
	       tc_SPLower-tc_SPUpper range after longjmp and exception will 
	       occur. */
	    udata->child_startup.as_startup_jmp_buf[0].regs[STACK_INDEX] = (unsigned long) udata->child_SPReg;
	    AROS_GET_SP = udata->child_SPReg;
	    Permit();
	    
	    D(bug("Calling daddy\n"));
	    /* Now call parent process, so it will resume his work */
	    Signal(GETUDATA->parent, GETUDATA->parent_signal);

	    GETUDATA->exec_returncode = exec_command(
		GETUDATA->exec_seglist, 
		GETUDATA->exec_taskname, 
		GETUDATA->exec_arguments, 
		GETUDATA->exec_stacksize
	    );
	    
	    free(GETUDATA->exec_arguments);

	    D(bug("exiting from forked execve()\n"));
            _exit(GETUDATA->exec_returncode);
	}
	else
	{
	    returncode = exec_command(
		seglist, 
		(inter ? inter : argv[0]), 
		argptr, 
		cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT
	    );
	        
	    free(argptr);

	    D(bug("exiting from non-forked execve()\n"));
	    _exit(returncode);		
	}
    }
    else
    {
	/* most likely file is not a executable */
    	saved_errno = ENOEXEC;
    	goto error;
    }

error:
    free(argptr);
    if(afilename != NULL) free(afilename);
    if(saved_errno) errno = saved_errno;
    return -1;
} /* execve() */
