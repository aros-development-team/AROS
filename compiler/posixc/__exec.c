/*
    Copyright © 2008-2013, The AROS Development Team. All rights reserved.
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
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "__posixc_intbase.h"
#include "__exec.h"
#include "__upath.h"
#include "__fdesc.h"
#include "__vfork.h"

#define DEBUG 0
#include <aros/debug.h>

static BOOL containswhite(const char *str);
static char *escape(const char *str);
static char *appendarg(char *argptr, int *argptrsize, const char *arg, APTR pool);
static char *appendargs(char *argptr, int *argptrsize, char *const args[], APTR pool);
static void __exec_cleanup(struct PosixCIntBase *PosixCBase);

/* Public functions */
/********************/

APTR __exec_prepare(const char *filename, int searchpath, char *const argv[], char *const envp[])
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    char *filename2 = NULL;
    const char *filename2_dos = NULL;
    int argssize = 512;
    struct Process *me;
    char ***environptr = __posixc_get_environptr();
    char **environ = (environptr != NULL) ? *environptr : NULL;

    D(bug("Entering __exec_prepare(\"%s\", %d, %x, %x)\n",
          filename, searchpath, argv, envp
    ));
    /* Sanity check */
    if (filename == NULL || filename[0] == '\0' || argv == NULL)
    {
        errno = EINVAL;
        goto error;
    }

    /* Use own memory to allocate so that no stdc.library functions need to be called
       exec_pool can also be allocated in __exec_valist2array
    */
    if (!PosixCBase->exec_pool)
        PosixCBase->exec_pool = CreatePool(MEMF_PUBLIC, 1024, 512);
    if (!PosixCBase->exec_pool)
    {
        errno = ENOMEM;
        goto error;
    }

    /* Remember current stdcbase, child may overwrite it and mess up exiting */
    PosixCBase->exec_oldstdcbase = PosixCBase->PosixCBase.StdCBase;

    /* Search path if asked and no directory separator is present in the file */
    if (searchpath && index(filename, '/') == NULL && index(filename, ':') == NULL)
    {
        int i, len, size;
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

        if (!path)
            path = ":/bin:/usr/bin";

        D(bug("__exec_prepare: PATH('%s')\n", path));

        path_ptr = AllocPooled(PosixCBase->exec_pool, strlen(path) + 1);
        strcpy(path_ptr, path);
        path = path_ptr;

        D(bug("__exec_prepare: PATH('%s')\n", path));

        size = 128;
        filename2 = AllocPooled(PosixCBase->exec_pool, size);
        if (!filename2)
        {
            errno = ENOMEM;
            goto error;
        }

        for(path_ptr = path, lock = (BPTR)NULL, path_item = strsep(&path_ptr, ",:");
            lock == (BPTR)NULL && path_item != NULL;
            path_item = strsep(&path_ptr, ",:")
        ) 
        {
            if(path_item[0] == '\0')
                path_item = ".";

            len = strlen(path_item) + strlen(filename) + 2;

            if (len > size)
            {
                FreePooled(PosixCBase->exec_pool, filename2, size);
                size = len;
                filename2 = AllocPooled(PosixCBase->exec_pool, size);
                if (!filename2)
                {
                    errno = ENOMEM;
                    goto error;
                }
            }

            strcpy(filename2, path_item);
            strcat(filename2, "/");
            strcat(filename2, filename);
            filename2_dos = __path_u2a(filename2);
            lock = Lock(filename2_dos, SHARED_LOCK);
            D(bug("__exec_prepare: Lock(\"%s\") == %x\n", filename2, (APTR)lock));
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
    {
        filename2 = (char *)filename;
        filename2_dos = __path_u2a(filename2);
    }
    
    if (PosixCBase->flags & PRETEND_CHILD)
    {
        struct vfork_data *udata = PosixCBase->vfork_data;
            
        udata->exec_filename = filename2;
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

            return NULL;
        }
        
        D(bug("__exec_prepare: Exiting from forked __exec_prepare id=%x, errno=%d\n",
              udata->exec_id, udata->child_errno
        ));
        
        return (APTR)PosixCBase;
    }

    D(bug("__exec_prepare: Not running as PRETEND_CHILD\n"));
    PosixCBase->exec_args = AllocPooled(PosixCBase->exec_pool, argssize);
    PosixCBase->exec_args[0] = '\0';

    /* Let's check if it's a script */
    BPTR fh = Open((CONST_STRPTR)filename2_dos, MODE_OLDFILE);
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
                    PosixCBase->exec_args = appendargs(
                        PosixCBase->exec_args, &argssize, args, PosixCBase->exec_pool
                    );
                    if (!PosixCBase->exec_args)
                    {
                        errno = ENOMEM;
                        goto error;
                    }

                    /* Set file to execute as the script interpreter */
                    filename2 = AllocPooled(PosixCBase->exec_pool, strlen(inter) + 1);
                    strcpy(filename2, inter);
                    filename2_dos = __path_u2a(filename2);
                }
            }
        }
    	Close(fh);
    }
    else
    {
        /* Simply assume it doesn't exist */
        errno = __stdc_ioerr2errno(IoErr());
        goto error;
    }

    /* Add arguments to command line args */
    PosixCBase->exec_args = appendargs(PosixCBase->exec_args, &argssize, argv + 1, PosixCBase->exec_pool);
    if (!PosixCBase->exec_args)
    {
        errno = ENOMEM;
        goto error;
    }

    /* End command line args with '\n' */
    if(strlen(PosixCBase->exec_args) > 0)
        PosixCBase->exec_args[strlen(PosixCBase->exec_args) - 1] = '\n';
    else
        strcat(PosixCBase->exec_args, "\n");

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
    PosixCBase->exec_taskname = AllocPooled(PosixCBase->exec_pool, strlen(filename2_dos) + 1);
    if (!PosixCBase->exec_taskname)
    {
        errno = ENOMEM;
        goto error;
    }
    strcpy(PosixCBase->exec_taskname, filename2_dos);
    
    /* Load file to execute */
    PosixCBase->exec_seglist = LoadSeg((CONST_STRPTR)filename2_dos);
    if (!PosixCBase->exec_seglist)
    {
        errno = ENOEXEC;
        goto error;
    }

    me = (struct Process *)FindTask(NULL);

    if (envp && envp != environ)
    {
        struct MinList tempenv;
        struct LocalVar *lv, *lv2;
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

    D(bug("__exec_prepare: Done, returning %p\n", aroscbase));

    /* Everything OK */
    return (APTR)PosixCBase;

error:
    __exec_cleanup(PosixCBase);
    
    return (APTR)NULL;
}


void __exec_do(APTR id)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    char *oldtaskname;
    struct CommandLineInterface *cli = Cli();
    struct Task *self = FindTask(NULL);
    LONG returncode;

    D(bug("[__exec_do] Entering, id(%x)\n", id));

    /* id is unused */
    (void)id;

    /* When exec is called under vfork condition, the PRETEND_CHILD flag is set
       and we need to signal child that exec is called.
    */
    if (PosixCBase->flags & PRETEND_CHILD)
    {
        struct vfork_data *udata = PosixCBase->vfork_data;

        D(bug("[__exec_do] PRETEND_CHILD\n"));

        __close_on_exec_fdescs();

	D(bug("[__exec_do] Notify child to call __exec_do\n"));

        /* Signal child that __exec_do is called */
        Signal(udata->child, 1 << udata->child_signal);

        /* Clean up in parent */
	D(bug("[__exec_do] Cleaning up parent\n"));
        __exec_cleanup(PosixCBase);
        
        /* Continue as parent process */
	D(bug("[__exec_do] Stop running as PRETEND_CHILD\n"));
        _exit(0);
        
        assert(0); /* Should not be reached */
        return;
    }

    D(bug("[__exec_do] !PRETEND_CHILD\n"));

    PosixCBase->flags |= EXEC_PARENT;

    oldtaskname = self->tc_Node.ln_Name;
    self->tc_Node.ln_Name = PosixCBase->exec_taskname;
    SetProgramName((STRPTR)PosixCBase->exec_taskname);


    /* Set standard files to the standard files from arosc */
    fdesc *in = __getfdesc(STDIN_FILENO), *out = __getfdesc(STDOUT_FILENO),
        *err = __getfdesc(STDERR_FILENO);
    BPTR oldin = BNULL, oldout = BNULL, olderr = BNULL;
    char inchanged = 0, outchanged = 0, errchanged = 0;
    struct Process *me = (struct Process *)FindTask(NULL);

    if(in && in->fcb->fh != Input())
    {
        oldin = SelectInput(in->fcb->fh);
        inchanged = 1;
    }
    if(out && in->fcb->fh != Output())
    {
        oldout = SelectOutput(out->fcb->fh);
        outchanged = 1;
    }
    if (err)
    {
        if (me->pr_CES)
        {
            if (me->pr_CES != err->fcb->fh)
                errchanged = 1;
        }
        else /* me->pr_CES */
        {
            /* Only replace if stdout != stderr */
            if (out && out->fcb->fh != err->fcb->fh)
                errchanged = 1;
        }
        if (errchanged)
        {
            olderr = me->pr_CES;
            me->pr_CES = err->fcb->fh;
        }
    }

    D(bug("[__exec_do] Running program, PosixCBase=%x\n", PosixCBase));
    returncode = RunCommand(
        PosixCBase->exec_seglist,
        cli->cli_DefaultStack * CLI_DEFAULTSTACK_UNIT,
        (STRPTR)PosixCBase->exec_args,
        strlen(PosixCBase->exec_args)
    );

    /* RunCommand() does not Close() standard output so may not flush either.
       So to be sure flush them here */
    Flush(Output());
    if (me->pr_CES)
        Flush(me->pr_CES);

    if(inchanged)
        SelectInput(oldin);
    if(outchanged)
        SelectOutput(oldout);
    if(errchanged)
        me->pr_CES = olderr;

    D(bug("[__exec_do] Program ran, PosixCBase=%x, __aros_getbase_PosixCBase()=%x\n",
          PosixCBase, __aros_getbase_PosixCBase()
      )
    );

    self->tc_Node.ln_Name = oldtaskname;
    SetProgramName((STRPTR)oldtaskname);

    __exec_cleanup(PosixCBase);
    
    D(bug("[__exec_do] exiting from non-forked __exec()\n"));
    _Exit(returncode);
}


char *const *__exec_valist2array(const char *arg1, va_list list)
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    int argc, i;
    static char *no_arg[] = {NULL};
    va_list list2;
    char *argit;
    
    assert(PosixCBase->exec_tmparray == NULL);
    
    va_copy(list2, list);
    
    if (arg1 == NULL)
        return no_arg;
    
    for (argit = va_arg(list, char *), argc = 1;
         argit != NULL;
         argit = va_arg(list, char *)
    )
        argc++;
    
    if (!(PosixCBase->exec_tmparray = malloc((argc+1)*(sizeof(char *)))))
    {
        D(bug("__exec_valist2array: Memory allocation failed\n"));
        va_end(list2);
        return NULL;
    }
    
    PosixCBase->exec_tmparray[0] = (char *)arg1;
    for (argit = va_arg(list2, char *), i = 1;
         i <= argc; /* i == argc will copy the NULL pointer */
         argit = va_arg(list2, char *), i++
    )
    {
        D(bug("arg %d: %x\n", i, argit));
        PosixCBase->exec_tmparray[i] = argit;
    }
   
    va_end(list2);
    
    return PosixCBase->exec_tmparray;
}


void __exec_cleanup_array()
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    if (PosixCBase->exec_tmparray)
    {
        free((void *)PosixCBase->exec_tmparray);
        PosixCBase->exec_tmparray = NULL;
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
static char *appendarg(char *argptr, int *argptrsize, const char *arg, APTR pool)
{
    while(strlen(argptr) + strlen(arg) + 2 > *argptrsize)
    {
        char *argptr2;
        int argptrsize2 = 2*(*argptrsize);

        argptr2 = AllocPooled(pool, argptrsize2);
        if(!argptr2)
        {
            FreePooled(pool, argptr, *argptrsize);
            return NULL;
        }
        strcpy(argptr2, argptr);
        FreePooled(pool, argptr, *argptrsize);
        argptr = argptr2;
        *argptrsize = argptrsize2;
    }
    strcat(argptr, arg);
    strcat(argptr, " ");
    
    return argptr;
}

static char *appendargs(char *argptr, int *argssizeptr, char *const args[], APTR pool)
{
    char *const *argsit;
    
    for (argsit = args; *argsit && argptr; argsit++)
    {
        if(containswhite(*argsit))
        {
            char *escaped = escape(*argsit);
            if(!escaped)
            {
                FreePooled(pool, argptr, *argssizeptr);
                return NULL;
            }
            argptr = appendarg(argptr, argssizeptr, escaped, pool);
            free(escaped);
        }
        else
            argptr = appendarg(argptr, argssizeptr, *argsit, pool);
    }
    
    return argptr;
}

static void __exec_cleanup(struct PosixCIntBase *PosixCBase)
{
    D(bug("__exec_cleanup: me(%x)\n", FindTask(NULL)));

    if (PosixCBase->exec_pool)
    {
        DeletePool(PosixCBase->exec_pool);
        PosixCBase->exec_pool = NULL;
    }
    if (PosixCBase->exec_seglist)
    {
        UnLoadSeg(PosixCBase->exec_seglist);
        PosixCBase->exec_seglist = (BPTR)NULL;
    }

    if (PosixCBase->exec_oldstdcbase)
    {
        PosixCBase->PosixCBase.StdCBase = PosixCBase->exec_oldstdcbase;
        PosixCBase->exec_oldstdcbase = NULL;
    }
}
