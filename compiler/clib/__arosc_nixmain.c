/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Desc: special main function for code which has to use special *nix features.
          This function gets called from a function with a similar name statically
	  linked with the program. This is so to make the program not depend on a
	  particular libc version.

    Lang: english
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <aros/startup.h>

#include <aros/debug.h>

#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>

#include "__upath.h"

static BOOL clone_vars(struct MinList *old_vars);
static void restore_vars(struct MinList *old_vars);
static void free_vars(struct MinList *vars);
static void update_PATH(void);

int __arosc_nixmain(int (*main)(int argc, char *argv[]), int argc, char *argv[])
{
    char *old_argv0 = NULL;
    struct MinList old_vars;

    /* Trigger *nix path handling on.  */
    __doupath = 1;

    /* argv[0] usually contains the name of the program, possibly with the full
       path to it. Here we translate that path, which is an AmigaDOS-style path,
       into an unix-style one.  */
    if (argv && argv[0] && !__get_arosc_privdata()->acpd_do_not_substitute_argv0)
    {
        char *new_argv0;

	new_argv0 = strdup(__path_a2u(argv[0]));
	if (new_argv0 == NULL)
	    return RETURN_FAIL;

        old_argv0 = argv[0];
	argv[0] = new_argv0;
    }

    /* Here we clone environment variables. We do this because
       if we've been invoked as a coroutine via dos.library/RunCommand()
       rather than as a newly created process, then we share our env variables
       with the caller, but we do not want that. It's kind of wasteful to do
       it even if we've been started as a fresh process, though, so if we can
       we avoid it. */
    if (!__get_arosc_privdata()->acpd_spawned)
    {
        if (!clone_vars(&old_vars))
	{
	    __aros_startup_error = RETURN_FAIL;
	    goto err_vars;
	}
    }

    /* If the PATH variable is not defined, then define it to be what CLI's path list
       points to.  */
    if (!getenv("PATH"))
    {
        update_PATH();
    }

    /* Call the real main.  */
    if (setjmp(__aros_startup_jmp_buf) == 0)
    {
        __aros_startup_error = (*main)(argc, argv);
    }


    if (!__get_arosc_privdata()->acpd_spawned)
        restore_vars(&old_vars);

err_vars:

    /* Restore the old argv[0].  */
    if (old_argv0 != NULL)
    {
        free(argv[0]);
	argv[0] = (char *)old_argv0;
    }

    return __aros_startup_error;
}

/* Clone the process' environment variables list. Once this function returns,
   the _cloned_ vars are used in place of the old ones.

   Returns the old list's content in the old_vars argument. Use this list only as
   argument to restore_vars() and for _nothing_ else.

   If this function fails, then FALSE is returned, otherwise TRUE is returned.

   One might argue that the whole affair is solved the wrong way around, that is
   clone_vars() should return the _cloned_ list and restore_vars() should be really
   named replace_vars() and take the cloned list as argument and replace the current
   one with that one, but doing it this way means breaking any programs which saved
   vars list's internals before invoking this program, although that is not even guaranteed
   to work normally, as the called program could change the env list anytime...  Well,
   in either case it doesn't do much of a difference, since the same amount of operations
   would be performed, only the order would change. */

BOOL clone_vars(struct MinList *old_vars)
{
    struct MinList l;
    struct LocalVar *lv, *newVar;
    struct Process *me;

    NEWLIST(&l);

    me = (struct Process *)FindTask(NULL);
    /* Copied and adapted from rom/dos/createnewproc.c. Perhaps there should
       be a public function for this?  */
    ForeachNode((struct List *)&me->pr_LocalVars, (struct Node *)lv)
    {
        size_t copyLength = strlen(lv->lv_Node.ln_Name) + 1 + sizeof(struct LocalVar);

        newVar = (struct LocalVar *)AllocVec(copyLength, MEMF_PUBLIC);
        if (newVar == NULL)
	{
	    free_vars(&l);
	    return FALSE;
	}

	memcpy(newVar, lv, copyLength);
	newVar->lv_Node.ln_Name = (char *)newVar + sizeof(struct LocalVar);
        newVar->lv_Value        = AllocMem(lv->lv_Len, MEMF_PUBLIC);
        if (newVar->lv_Value == NULL)
        {
	    FreeVec(newVar);
	    free_vars(&l);
	    return FALSE;
	}

        memcpy(newVar->lv_Value, lv->lv_Value, lv->lv_Len);

        ADDTAIL(&l, newVar);
    }

    *old_vars = me->pr_LocalVars;
    me->pr_LocalVars = l;

    l.mlh_Head->mln_Pred     = (struct MinNode *)&me->pr_LocalVars.mlh_Head;
    l.mlh_TailPred->mln_Succ = (struct MinNode *)&me->pr_LocalVars.mlh_Tail;

    return TRUE;
}

/* Restores the old env var's list content */
static void restore_vars(struct MinList *old_vars)
{
    struct Process *me = (struct Process *)FindTask(NULL);

    free_vars(&me->pr_LocalVars);
    me->pr_LocalVars = *old_vars;
}

/* taken from rom/dos/createnewproc.c.  */
static void free_vars(struct MinList *vars)
{
    struct LocalVar *varNode;
    struct Node     *tempNode;

    ForeachNodeSafe((struct List *)vars, (struct Node *)varNode, tempNode)
    {
        FreeMem(varNode->lv_Value, varNode->lv_Len);
        Remove((struct Node *)varNode);
        FreeVec(varNode);
    }
}

/* setenv("PATH", current_cli_path, 1) */
static void update_PATH(void)
{
    typedef struct
    {
        BPTR next;
        BPTR lock;
    } PathEntry;

    #define PE(x) ((PathEntry *)(BADDR(x)))

    UBYTE aname[PATH_MAX]; /* PATH_MAX Ought to enough, it would be too complicated
                              handling aname dynamically (thanks to our sucky dos.library).  */
    char *PATH = NULL;
    size_t oldlen = 0;
    PathEntry *cur;
    struct CommandLineInterface *cli = Cli();

    /* No cli, no luck.  */
    if (cli == NULL)
        return;

    for
    (
        cur = PE(cli->cli_CommandDir);
        cur != NULL;
        cur = PE(cur->next)
    )
    {
        char *new_PATH;
	const char *uname;
	size_t ulen, newlen;

        if (NameFromLock(cur->lock, aname, sizeof(aname)) == DOSFALSE)
	    continue;

	D(bug("aname = %s\n", aname));

        uname = __path_a2u(aname);
	if (!uname)
	    continue;

	D(bug("uname = %s\n", uname));

	ulen = strlen(uname);
	newlen = oldlen + ulen + 1;

	new_PATH = realloc(PATH, newlen);
	if (!new_PATH)
	    continue;

	PATH = new_PATH;

	D(new_PATH[oldlen] = '\0');
	D(bug("new_PATH = %s\n", new_PATH));

	if (oldlen)
	{
            new_PATH[oldlen] = ':';
	    new_PATH++;
	}
	memcpy(new_PATH + oldlen, uname, ulen + 1);

        D(bug("PATH = %s - oldlen = %d, newlen = %d\n", PATH, oldlen, newlen));
	oldlen = newlen - 1;
    }

    if (PATH)
        setenv("PATH", PATH, 1);
}
