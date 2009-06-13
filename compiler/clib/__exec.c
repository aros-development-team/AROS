/*
    Copyright © 2008-2009, The AROS Development Team. All rights reserved.
    $Id$

    Support functions for POSIX exec*() functions.
*/
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include <aros/libcall.h>

#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <strings.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>

#include "__arosc_privdata.h"
#include "__exec.h"
#include "__upath.h"
#include "__errno.h"
#include "__open.h"

#define DEBUG 0
#include <aros/debug.h>

static BOOL containswhite(const char *str);
static char *escape(const char *str);
static char *appendarg(char *argptr, int *argptrsize, const char *arg);
static char *appendargs(char *argptr, int *argptrsize, char *const args[]);
static void __exec_cleanup(struct arosc_privdata *privdata);

/* Public functions */
/********************/

APTR __exec_prepare(const char *filename, int searchpath, char *const argv[], char *const envp[])
{
    struct arosc_privdata *privdata = __get_arosc_privdata();
    char *filename2 = NULL, *filenamefree = NULL;
    int argssize = 1024;

    D(bug("Entering __exec_prepare(\"%s\", %d, %x, %x)\n",
          filename, searchpath, argv, envp
    ));
    /* Sanity check */
    if (filename == NULL || filename[0] == '\0' || argv == NULL)
    {
        errno = EINVAL;
        goto error;
    }

    if (privdata->acpd_flags & PRETEND_CHILD)
    {
        struct vfork_data *udata = privdata->acpd_vfork_data;
            
        udata->exec_filename = filename;
        udata->exec_searchpath = searchpath;
        udata->exec_argv = argv;
        udata->exec_envp = envp;
            
        /* Set this so the child knows that __exec_prepare was called */
        udata->child_executed = 1;

        D(bug("__exec_prepare: Calling child\n"));
        /* Now call child process, so it will call __exec_prepare */
        Signal(udata->child, 1 << udata->child_signal);	    

        D(bug("__exec_prepare: Waiting for child to finish __exec_prepare\n"));
        /* __exec_prepare should be finished now on child */
        Wait(1 << udata->parent_signal);

        if (!udata->exec_id)
        {
            D(bug("__exec_prepare: Continue child immediately on error\n"));
            Signal(udata->child, 1 << udata->child_signal);
        }
        
        D(bug("__exec_prepare: Exiting from forked __exec_prepare id=%x, errno=%d\n",
              udata->exec_id, udata->child_errno
        ));
        
        return udata->exec_id;
    }
    privdata->acpd_exec_args = malloc(argssize);
    privdata->acpd_exec_args[0] = '\0';
    
    /* Search path if asked and no directory separator is present in the file */
    if (searchpath && index(filename, '/') == NULL && index(filename, ':') == NULL)
    {
        int i;
        char *path = NULL, *path_ptr, *path_item;
        BPTR lock;
        
        if (environ)
        {
            for (i=0; environ[i]; i++)
            {
                if (strncmp(environ[i], "PATH=", 5) == 0)
                {
                    path = &environ[i][5];
                    break;
                }
            }
        }
        
        if (!path)
            path = getenv("PATH");
        
        path = strdup(path ? path : ":/bin:/usr/bin");

        for(path_ptr = path, lock = (BPTR)NULL, path_item = strsep(&path_ptr, ",:");
            lock == (BPTR)NULL && path_item != NULL;
            path_item = strsep(&path_ptr, ",:")
        ) 
        {
            if(filenamefree)
                free(filenamefree);
            
            if(path_item[0] == '\0')
                path_item = ".";

            filenamefree = filename2 = malloc(strlen(path_item) + strlen(filename) + 2);
            if(filename2)
            {
                filename2[0] = '\0';
                strcat(filename2, path_item);
                strcat(filename2, "/");
                strcat(filename2, filename);
                lock = Lock(__path_u2a(filename2), SHARED_LOCK);
                D(bug("__exec_prepare: Lock(\"%s\") == %x\n", filename2, (APTR)lock));
            }
            else
            {
                errno = ENOMEM;
                goto error;
            }
        }
        if(lock != (BPTR)NULL)
            UnLock(lock);
        else
        {
            errno = ENOENT;
            goto error;
        }
    }
    else
        filename2 = (char *)filename;

    
    /* Let's check if it's a script */
    BPTR fh = Open((CONST_STRPTR)__path_u2a(filename2), MODE_OLDFILE);
    if(fh)
    {
    	if(FGetC(fh) == '#' && FGetC(fh) == '!')
    	{
           char firstline[128], *linebuf, *inter, *interargs = NULL;
            
            /* It is a script, let's read the first line */
            if(FGets(fh, (STRPTR)firstline, sizeof(firstline) - 1))
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

                    /* Add interpeter args and the script name to command line args */
                    char *args[] = {NULL, NULL, NULL};
                    if (interargs)
                    {
                        args[0] = interargs;
                        args[1] = filename2;
                    }
                    else
                    {
                        args[0] = filename2;
                    }
                    privdata->acpd_exec_args = appendargs(
                        privdata->acpd_exec_args, &argssize, args
                    );
                    if (!privdata->acpd_exec_args)
                    {
                        errno = ENOMEM;
                        goto error;
                    }

                    /* Set file to execute as the script interpreter */
                    if (filenamefree)
                        free(filenamefree);
                    filenamefree = filename2 = strdup(inter);
                }
            }
        }
    	Close(fh);
    }
    else
    {
        /* Simply assume it doesn't exist */
        errno = IoErr2errno(IoErr());
        goto error;
    }

    /* Add arguments to command line args */
    privdata->acpd_exec_args = appendargs(privdata->acpd_exec_args, &argssize, argv + 1);
    if (!privdata->acpd_exec_args)
    {
        errno = ENOMEM;
        goto error;
    }

    /* End command line args with '\n' */
    if(strlen(privdata->acpd_exec_args) > 0)
    	privdata->acpd_exec_args[strlen(privdata->acpd_exec_args) - 1] = '\n';
    else
    	strcat(privdata->acpd_exec_args, "\n");
    

    /* let's make some sanity tests */
    struct stat st;
    if(stat(filename2, &st) == 0)
    {
	if(!(st.st_mode & S_IXUSR) && !(st.st_mode & S_IXGRP) && !(st.st_mode & S_IXOTH))
	{
	    /* file is not executable */
	    errno = EACCES;
	    goto error;
	}
	if(st.st_mode & S_IFDIR)
	{
	    /* it's a directory */
	    errno = EACCES;
	    goto error;	
	}
    }
    else
    {
	/* couldn't stat file */
	goto error;
    }

    /* Set taskname */
    if (!(privdata->acpd_exec_taskname = strdup(filename2)))
    {
        errno = ENOMEM;
        goto error;
    }
    
    /* Load file to execute */
    privdata->acpd_exec_seglist = LoadSeg((CONST_STRPTR)__path_u2a(filename2));
    if (!privdata->acpd_exec_seglist)
    {
        errno = ENOEXEC;
        goto error;
    }

    if (envp && envp != environ)
    {
        struct MinList tempenv;
        struct LocalVar *lv, *lv2;
        struct Process *me = (struct Process *)FindTask(NULL);
        char *const *envit;
        int env_ok = 1;
        
        /* Remember previous environment variables so they can be put back
         * if something goes wrong
         */
        NEWLIST(&tempenv);
        ForeachNodeSafe(&me->pr_LocalVars, lv, lv2)
        {
            Remove((struct Node *)lv);
            AddTail((struct List *)&tempenv, (struct Node *)lv);
        }
        
        NEWLIST(&me->pr_LocalVars);
        environ = NULL;

        for (envit = envp; *envit && env_ok; envit++)
            env_ok = putenv(*envit) == 0;
        
        if (env_ok)
            /* Old vars may be freed */
            ForeachNodeSafe(&tempenv, lv, lv2)
            {
                Remove((struct Node *)lv);
                FreeMem(lv->lv_Value, lv->lv_Len);
                FreeVec(lv);
            }
        else
        {
            /* Free new nodes */
            ForeachNodeSafe(&me->pr_LocalVars, lv, lv2)
            {
                Remove((struct Node *)lv);
                FreeMem(lv->lv_Value, lv->lv_Len);
                FreeVec(lv);
            }

            /* Put old ones back */
            ForeachNodeSafe(&tempenv, lv, lv2)
            {
                Remove((struct Node *)lv);
                AddTail((struct List *)&me->pr_LocalVars, (struct Node *)lv);
            }
            
            errno = ENOMEM;
            goto error;
        }
    }
    
    /* Set standard files to the standard files from arosc */
    fdesc *in = privdata->acpd_fd_array[STDIN_FILENO],
        *out = privdata->acpd_fd_array[STDOUT_FILENO],
        *err = privdata->acpd_fd_array[STDERR_FILENO];

    if(in) 
        privdata->acpd_exec_oldin = SelectInput(in->fcb->fh);
    if(out) 
        privdata->acpd_exec_oldout = SelectOutput(out->fcb->fh);
    if(err)
        privdata->acpd_exec_olderr = SelectError(err->fcb->fh);

    /* Clean up data */
    if (filenamefree)
        free(filenamefree);

    /* Generate new privdata for the exec */
    assert(!(privdata->acpd_flags & KEEP_OLD_ACPD));
    privdata->acpd_flags |= CREATE_NEW_ACPD;
    privdata->acpd_exec_aroscbase = OpenLibrary("arosc.library", 0);
    if(!privdata->acpd_exec_aroscbase)
    {
	errno = ENOMEM;
	goto error;
    }

    /* Initialize some data of the new generated privdata */
    struct arosc_privdata *newprivdata = __get_arosc_privdata();
    assert(!(newprivdata->acpd_flags & CREATE_NEW_ACPD));
    newprivdata->acpd_flags |= KEEP_OLD_ACPD;
    newprivdata->acpd_parent_does_upath = privdata->acpd_doupath;

    /* Everything OK */
    return (APTR)privdata;

error:
    __exec_cleanup(privdata);
    
    if (filenamefree)
        free(filenamefree);
    
    return (APTR)NULL;
}


void __exec_do(APTR id)
{
    struct arosc_privdata *privdata = id;
    char *oldtaskname;
    struct CommandLineInterface *cli = Cli();
    struct Task *self = FindTask(NULL);
    LONG returncode;

    if (__get_arosc_privdata()->acpd_flags & PRETEND_CHILD)
    {
        struct vfork_data *udata = __get_arosc_privdata()->acpd_vfork_data;
        
        /* Signal child that __exec_do is called */
        Signal(udata->child, 1 << udata->child_signal);
        
        /* Continue as parent process */
        _exit(0);
        
        assert(0); /* Should not be reached */
        return;
    }

    oldtaskname = self->tc_Node.ln_Name;
    self->tc_Node.ln_Name = privdata->acpd_exec_taskname;
    SetProgramName((STRPTR)privdata->acpd_exec_taskname);

    returncode = RunCommand(
        privdata->acpd_exec_seglist,
        cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT,
        (STRPTR)privdata->acpd_exec_args,
        strlen(privdata->acpd_exec_args)
    );

    self->tc_Node.ln_Name = oldtaskname;
    SetProgramName((STRPTR)oldtaskname);

    __exec_cleanup(privdata);
    
    D(bug("exiting from non-forked execve()\n"));
    _exit(returncode);
}


char *const *__exec_valist2array(const char *arg1, va_list list)
{
    struct arosc_privdata *privdata = __get_arosc_privdata();
    int argc, i;
    static char *no_arg[] = {NULL};
    va_list list2;
    char *argit;
    
    assert(privdata->acpd_exec_tmparray == NULL);
    
    va_copy(list2, list);
    
    if (arg1 == NULL)
        return no_arg;
    
    for (argit = va_arg(list, char *), argc = 1;
         argit != NULL;
         argit = va_arg(list, char *)
    )
        argc++;
    
    if (!(privdata->acpd_exec_tmparray = malloc((argc+1)*(sizeof(char *)))))
    {
        D(bug("__exec_valist2array: Memory allocation failed\n"));
        va_end(list2);
        return NULL;
    }
    
    privdata->acpd_exec_tmparray[0] = (char *)arg1;
    for (argit = va_arg(list2, char *), i = 1;
         i <= argc; /* i == argc will copy the NULL pointer */
         argit = va_arg(list2, char *), i++
    )
    {
        D(bug("arg %d: %x\n", i, argit));
        privdata->acpd_exec_tmparray[i] = argit;
    }
   
    va_end(list2);
    
    return privdata->acpd_exec_tmparray;
}


void __exec_cleanup_array()
{
    struct arosc_privdata *privdata = __get_arosc_privdata();
    if (privdata->acpd_exec_tmparray)
    {
        free((void *)privdata->acpd_exec_tmparray);
        privdata->acpd_exec_tmparray = NULL;
    }
}


/* Support functions */
/*********************/

/* Return TRUE if there are any white spaces in the string */
static BOOL containswhite(const char *str)
{
    while(*str != '\0')
    	if(isspace(*str++)) return TRUE;
    return FALSE;
}

/* Escape the string and quote it */ 
static char *escape(const char *str)
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
static char *appendarg(char *argptr, int *argptrsize, const char *arg)
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

static char *appendargs(char *argptr, int *argssizeptr, char *const args[])
{
    char *const *argsit;
    
    for (argsit = args; *argsit && argptr; argsit++)
    {
        if(containswhite(*argsit))
        {
            char *escaped = escape(*argsit);
            if(!escaped) {
                free(argptr);
                return NULL;
            }
            argptr = appendarg(argptr, argssizeptr, escaped);
            free(escaped);
        }
        else
            argptr = appendarg(argptr, argssizeptr, *argsit);
    }
    
    return argptr;
}

void __exec_cleanup(struct arosc_privdata *privdata)
{

    /* Delete old private data */
    if (privdata->acpd_exec_aroscbase)
    {
        CloseLibrary(privdata->acpd_exec_aroscbase);
        privdata->acpd_exec_aroscbase = NULL;
    }

    if(privdata->acpd_exec_oldin)
    {
        SelectInput(privdata->acpd_exec_oldin);
        privdata->acpd_exec_oldin = (BPTR)NULL;
    }
    if(privdata->acpd_exec_oldout)
    {
        SelectOutput(privdata->acpd_exec_oldout);
        privdata->acpd_exec_oldout = (BPTR)NULL;
    }
    if(privdata->acpd_exec_olderr)
    {
        SelectError(privdata->acpd_exec_olderr);
        privdata->acpd_exec_olderr = (BPTR)NULL;
    }

    if (privdata->acpd_exec_args)
    {
        free(privdata->acpd_exec_args);
        privdata->acpd_exec_args = NULL;
    }
    if (privdata->acpd_exec_seglist)
    {
        UnLoadSeg(privdata->acpd_exec_seglist);
        privdata->acpd_exec_seglist = (BPTR)NULL;
    }
    if (privdata->acpd_exec_taskname)
    {
        free(privdata->acpd_exec_taskname);
        privdata->acpd_exec_taskname = NULL;
    }
}
