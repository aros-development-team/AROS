/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: 
    Lang: English
*/

/*
    HISTORY

    9.1.2000  SDuvan  implemented

*/


#  define  DEBUG  0
#  include <aros/debug.h>

#include "dos_intern.h"

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <utility/tagitem.h>
#include <exec/memory.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <string.h>

#define  PROCESS(x)  ((struct Process *)x)

BOOL ExecCommand(ULONG type, STRPTR command, STRPTR shell, BPTR input,
		 BPTR output, struct TagItem *tl, struct DosLibrary *DOSBase)
{

    struct Process *me = PROCESS(FindTask(NULL));

    struct TagItem tags[] = { { NP_Seglist    , NULL        },        /* 0 */
			      { NP_FreeSeglist, (IPTR)TRUE  },        /* 1 */
			      { NP_Priority   , me->pr_Task.tc_Node.ln_Pri }, /* 2 */
			      { NP_StackSize  , AROS_STACKSIZE },     /* 3 */
			      { NP_Path       , NULL        },        /* 4 */
			      { NP_Name       , (IPTR)"Background CLI" }, /*5*/
			      { NP_Input      , NULL        },        /* 6 */
			      { NP_Output     , NULL        },        /* 7 */
			      { NP_CloseInput , (IPTR)FALSE },        /* 8 */
			      { NP_CloseOutput, (IPTR)FALSE },        /* 9 */
			      { NP_CurrentDir , ~0ul        },        /* 10 */
			      { NP_HomeDir    , NULL        },        /* 11 */
			      { NP_Cli        , (IPTR)TRUE  },        /* 12 */
			      { NP_WindowPtr  , NULL        },        /* 13 */
			      { NP_Arguments  , NULL        },        /* 14 */
			      { NP_Synchronous, (IPTR)FALSE },        /* 15 */
			      { TAG_END       , NULL        } };

    Tag filterList[] = { NP_Seglist, NP_FreeSeglist, NP_Entry,
			 NP_Input, NP_Output, NP_CloseInput,
			 NP_CloseOutput, NP_HomeDir, NP_Cli, NULL };

    STRPTR          comStr;
    struct Process *process;
    BPTR            shellSeg;
    struct TagItem *newTags;

    if (command != NULL)
    {
	LONG length = strlen(command) + strlen("COMMAND ") + 1;

	comStr = AllocVec(length, MEMF_PUBLIC | MEMF_CLEAR);

	if(comStr == NULL)
	{
	    SetIoErr(ERROR_NO_FREE_STORE);

	    return FALSE;
	}

	strcpy(comStr, "COMMAND ");
	strcat(comStr, command);

	tags[14].ti_Data = (IPTR)comStr;
    }

    D(bug("Execcommand: Got commandline... %s\n", comStr));

    shellSeg = LoadSeg(shell);

    tags[0].ti_Data = (IPTR)shellSeg;

    /* If this is a synchronous call, we set the process' windowptr
       to our own. */
    if(type != RUN_SYSTEM_ASYNCH)
    {
	tags[13].ti_Data = (IPTR)me->pr_WindowPtr;
    }

    tags[6].ti_Data = (IPTR)input;
    tags[7].ti_Data = (IPTR)output;

    if (output == NULL)
    {
	if (input && IsInteractive(input))
	{
	    BPTR olddir = CurrentDir(input);
	    tags[7].ti_Data = (IPTR)Open("", FMF_WRITE);
	    CurrentDir(olddir);
	}
	else
	    tags[7].ti_Data = (IPTR)Open("*", FMF_WRITE);

	tags[9].ti_Data  = (IPTR)TRUE;     /* NP_CloseOutput */
    }

    D(bug("Former input: %p, output: %p\n"
	    "New    input: %p, output: %p\n", Input(), Output(),
	    tags[6].ti_Data, tags[7].ti_Data));


    /* Clone tag items so we don't mess up the users memory when filtering
       It's OK if tl == NULL, as this is handled by CloneTagItems() */
    newTags = CloneTagItems(tl);

    if (newTags == NULL)
    {
	FreeVec(comStr);
	SetIoErr(ERROR_NO_FREE_STORE);
	return FALSE;
    }

    /* If this is an asynchronous call, we will close the input and output
       streams when exiting this process */
    if (type == RUN_SYSTEM_ASYNCH)
    {
	tags[8].ti_Data  = (IPTR)TRUE;     /* NP_CloseInput  */
	tags[9].ti_Data  = (IPTR)TRUE;     /* NP_CloseOutput */
    }

    if (type != RUN_SYSTEM_ASYNCH)
    {
	/* RUN_EXECUTE means we shall execute this process synchronously */
	tags[15].ti_Data = (IPTR)TRUE;	   /* NP_Synchronous */
    }

    FilterTagItems(newTags, filterList, TAGFILTER_NOT);
    ApplyTagChanges(tags, newTags);
    FreeTagItems(newTags);

    /* NP_CurrentDir -- if nothing specified, we use the parent's dir --
       This is done in CreateNewProc() too, but we need it here due to our
       filter approach ;-( */
    if (tags[10].ti_Data == ~0ul)
    {    
	BPTR  curDir = CurrentDir(NULL);    /* Get this process' current dir */

	tags[10].ti_Data = (IPTR)DupLock(curDir);
	CurrentDir(curDir);	            /* ... and replace it */
    }

    process = CreateNewProc(tags);

    /* For now, we just return a boolean telling whether we could start the
       shell or not. To be able to return the right thing here, we must
       implement ChildStatus() and define a new tag to CreateNewProc();
       something like NP_StatusPtr, a location where the status of the
       child should be saved when the child exits. */
    if (process == NULL)
    {
	/* If we couldn't create the process, CreateNewProc() will already
	   have freed our resources (command string, current dir etc.) */
	return FALSE;
    }

    return TRUE;
}
