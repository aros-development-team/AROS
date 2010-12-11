
#define DEBUG 1
#include <aros/debug.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include "dos_intern.h"

/* Try to open name recursively calling itself in case it's a soft link.
   Store result in handle. Return boolean value indicating result. */
LONG InternalOpen(CONST_STRPTR name, LONG action, 
    struct FileHandle *handle, LONG soft_nesting, struct DosLibrary *DOSBase)
{
    /* Get pointer to process structure */
    struct Process *me = (struct Process *)FindTask(NULL);
    LONG ret = DOSFALSE;
    LONG error = 0;
    LONG error2 = 0;
    BPTR con, ast;

    D(bug("[Open] FH=%x Process: 0x%p \"%s\", Window: 0x%p, Name: \"%s\", Mode: %d \n",
        handle, me, me->pr_Task.tc_Node.ln_Name, me->pr_WindowPtr, name, action));

    if(soft_nesting == 0)
    {
	SetIoErr(ERROR_TOO_MANY_LEVELS);
	return DOSFALSE;
    }

    switch(action)
    {
	case ACTION_FINDINPUT:
	    ast = con = me->pr_CIS;
	    break;

	case ACTION_FINDOUTPUT:
	    con = me->pr_COS;
	    ast = me->pr_CES ? me->pr_CES : me->pr_COS;
	    break;

	case ACTION_FINDUPDATE:
	    con = me->pr_COS;
	    ast = me->pr_CES ? me->pr_CES : me->pr_COS;
	    break;

	default:
	    action = ACTION_FINDINPUT;
	    ast = con = me->pr_CIS;
	    break;
    }

    if(!Stricmp(name, (STRPTR) "NIL:"))
    {
    	SetIoErr(0);
    	handle->fh_Type = BNULL;
    	/* NIL: is considered interactive */
    	handle->fh_Port = (struct MsgPort*)DOSTRUE;
        return DOSTRUE;
    }
    else if(!Stricmp(name, (STRPTR) "CONSOLE:") || !Stricmp(name, (STRPTR) "*"))
    {
    	BPTR h = name[0] == '*' ? ast : con;
    	if (me->pr_ConsoleTask) {
  	    BSTR bstrname = C2BSTR(name);
            dopacket3(DOSBase, &error, me->pr_ConsoleTask, action, MKBADDR(handle), h, bstrname);
            FreeVec(BADDR(bstrname));
            handle->fh_Type = me->pr_ConsoleTask;
	} else {
	    /* was NIL: */
    	    SetIoErr(0);
    	    handle->fh_Type = BNULL;
            return DOSTRUE;
        }  
    }
    else if (isdosdevicec(name) < 0) { /* no ":" */
	BSTR bstrname = C2BSTR(name);
 	BPTR cur;
        struct FileLock *fl;
        cur = me->pr_CurrentDir;
        if (!cur)
            cur = DOSBase->dl_SYSLock;
        fl = BADDR(cur);
        dopacket3(DOSBase, &error, fl->fl_Task, action, MKBADDR(handle), cur, bstrname);
        handle->fh_Type = fl->fl_Task;
    	FreeVec(BADDR(bstrname));
    } else { /* ":" */
	BSTR bstrname = C2BSTR(name);
        struct DevProc *dvp = NULL;
        do {
            if ((dvp = GetDeviceProc(name, dvp)) == NULL) {
            	if (!error)
                    error = IoErr();
                break;
            }
            dopacket3(DOSBase, &error, dvp->dvp_Port, action, MKBADDR(handle), dvp->dvp_Lock, bstrname);
            handle->fh_Type = dvp->dvp_Port;
        } while(error == ERROR_OBJECT_NOT_FOUND && action != ACTION_FINDOUTPUT);
    	FreeVec(BADDR(bstrname));

        if (error == ERROR_NO_MORE_ENTRIES)
            error = me->pr_Result2 = ERROR_OBJECT_NOT_FOUND;

        if(error == ERROR_IS_SOFT_LINK)
        {
            ULONG buffer_size = 256;
            STRPTR softname;
            LONG continue_loop;
            LONG written;

            do
            {
                continue_loop = FALSE;
                if(!(softname = AllocVec(buffer_size, MEMF_ANY)))
                {
                    error2 = ERROR_NO_FREE_STORE;
                    break;
                }

                written = ReadLink(dvp->dvp_Port, dvp->dvp_Lock, name, softname, buffer_size);
                if(written == -1)
                {
                    /* An error occured */
                    error2 = IoErr();
                }
                else if(written == -2)
                {
                    /* If there's not enough space in the buffer, increase
                       it and try again */
                    continue_loop = TRUE;
                    buffer_size *= 2;
                }
                else if(written >= 0)
                {
                    /* All OK */
                    BPTR olddir;
                    olddir = CurrentDir(dvp->dvp_Lock);
                    ret = InternalOpen(softname, action, handle, soft_nesting - 1, DOSBase);
                    error2 = IoErr();
                    CurrentDir(olddir);
                }
                else
                    error2 = ERROR_UNKNOWN;
                
                FreeVec(softname);
            }
            while(continue_loop);
        }

        FreeDeviceProc(dvp);
    }

    D(bug("[Open]=%d. Port=%x\n", error, handle->fh_Type));
    if(!error)
    {
        if (IsInteractive(MKBADDR(handle)))
            SetVBuf(MKBADDR(handle), NULL, BUF_LINE, -1);
        
        return DOSTRUE;
    }
    else if(error == ERROR_IS_SOFT_LINK)
    {
	if(!ret)
	    SetIoErr(error2);
	else
	    SetIoErr(0);
	return ret;
    }
    else
    {
	SetIoErr(error);
	return DOSFALSE;
    }
}
