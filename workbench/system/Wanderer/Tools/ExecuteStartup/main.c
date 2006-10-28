/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Executes programs in sys:WBStartup
    Lang: English
*/

/*****************************************************************************

    NAME
        ExecuteStartup

    LOCATION
        Wanderer:Tools

    FUNCTION
        Executes programs in sys:WBStartup.
        
        It checks the following tooltypes of the called programs:
                STARTPRI=n    n can be between +127 and -128. The program
                              with the highest priority is started first.
                              
                WAIT=n        Wait n seconds after execution.
                              n can be between 0 and 60.  
                
                DONOTWAIT     don't wait till the program is finished.	

    EXAMPLE
    
    BUGS
        All programs are treated like DONOTWAIT is set. 
    
    SEE ALSO

******************************************************************************/

#define DEBUG 0

#include <string.h>
#include <stdlib.h>

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/workbench.h>
#include <proto/intuition.h>

#define STARTUPDIR "sys:WBStartup/"

struct InfoNode
{
        struct Node node;
        ULONG waittime;
        BOOL donotwait;
};

const TEXT version_string[] = "$VER: ExecuteStartup 0.2 (29.1.2006)";

static TEXT fileNameBuffer[MAXFILENAMELENGTH + 1];

static BOOL checkIcon(STRPTR name, LONG *pri, ULONG *time, BOOL *notwait);
static BOOL searchInfo(struct List *infoList);
static void executePrograms(struct List *infolist);
static BOOL executeWBStartup(void);

int
main(void)
{
        executeWBStartup();
        return RETURN_OK;
}                                       

static BOOL
checkIcon(STRPTR name, LONG *pri, ULONG *time, BOOL *notwait)
{
        *pri = 0;
        *time = 0;
        *notwait = FALSE;

        struct DiskObject *dobj = GetDiskObject(name);
        if (dobj == NULL)
        {
                struct EasyStruct es = {sizeof(struct EasyStruct), 0,
                        "Error", "ExecuteStartup\nGetDiskObject failed for:\n%s", "OK"};
                EasyRequest(0, &es, 0, name);
                return FALSE;
        }

        if (dobj->do_Type == WBTOOL)
        {
                const STRPTR *toolarray = (const STRPTR *)dobj->do_ToolTypes;
                STRPTR s;
                if ((s = FindToolType(toolarray, "STARTPRI")))
                {
                        *pri = atol(s);
                        if (*pri < -128) *pri = -128;
                        if (*pri > 127) *pri = 127;
                }
                if ((s = FindToolType(toolarray, "WAIT")))
                {
                        *time = atol(s);
                        if (*time > 60) *time = 60;
                }
                if ((s = FindToolType(toolarray, "DONOTWAIT")))
                {
                        *notwait = TRUE;
                }
                FreeDiskObject(dobj);
                return TRUE;
        }
        return FALSE;
}

static BOOL
searchInfo(struct List *infoList)
{
        BOOL retvalue = TRUE;
        LONG error;
        struct AnchorPath *ap = AllocVec(sizeof(struct AnchorPath), MEMF_ANY | MEMF_CLEAR);
        if (ap)
        {
                error = MatchFirst(STARTUPDIR "?#?.info", ap);
                while (!error)
                {
                        strcpy(fileNameBuffer, ap->ap_Info.fib_FileName);
                        BYTE * pointPos = strrchr(fileNameBuffer, '.');
                        *pointPos = '\0';

                        LONG pri;
                        ULONG time;
                        BOOL notwait;
                        if (checkIcon(fileNameBuffer, &pri, &time, &notwait))
                        {
                                struct InfoNode *newnode = AllocVec(sizeof (*newnode), MEMF_ANY | MEMF_CLEAR);
                                if (newnode == NULL)
                                {
                                        struct EasyStruct es = {sizeof(struct EasyStruct), 0,
                                                "Error", "ExecuteStartup\nOut of memory for InfoNode", "OK"};
                                        EasyRequest(0, &es, 0);
                                        retvalue = FALSE;
                                        goto exit;
                                }
                                newnode->node.ln_Name = AllocVec(strlen(fileNameBuffer) + 1, MEMF_ANY);
                                if (newnode->node.ln_Name == NULL)
                                {
                                        struct EasyStruct es = {sizeof(struct EasyStruct), 0,
                                                "Error", "ExecuteStartup\nOut of memory for ln_Name", "OK"};
                                        EasyRequest(0, &es, 0);
                                        retvalue = FALSE;
                                        goto exit;
                                }
                                strcpy(newnode->node.ln_Name, fileNameBuffer);
                                newnode->node.ln_Pri = pri;
                                newnode->waittime = time;
                                newnode->donotwait = notwait;
                                Enqueue(infoList, (struct Node*)newnode);
                        }
                        error = MatchNext(ap);

                }
                if (error != ERROR_NO_MORE_ENTRIES)
                {
                        struct EasyStruct es = {sizeof(struct EasyStruct), 0,
                                "Error", "ExecuteStartup\nError %ld occured while scanning directory", "OK"};
                        EasyRequest(0, &es, 0, error);
                        retvalue = FALSE;
                        goto exit;
                }
                MatchEnd(ap);
        }
        else
        {
                struct EasyStruct es = {sizeof(struct EasyStruct), 0,
                        "Error", "ExecuteStartup\nOut of memory for AnchorPath", "OK"};
                EasyRequest(0, &es, 0);
                retvalue = FALSE;
        }
exit:
        FreeVec(ap);
        return retvalue;
}

static void
executePrograms(struct List *infolist)
{
        struct InfoNode *infonode;
        ForeachNode(infolist, infonode)
        {
                D(bug("ExecuteStartup Name %s Pri %d Time %d Notwait %d\n",
                        infonode->node.ln_Name, infonode->node.ln_Pri,
                        infonode->waittime, infonode->donotwait));
                if (OpenWorkbenchObject(infonode->node.ln_Name, TAG_END))
                {
                        Delay(infonode->waittime * 50);
                }
                else
                {
                        struct EasyStruct es = {sizeof(struct EasyStruct), 0,
                                "Warning", "ExecuteStartup\nOpenWorkbenchObject failed for:\n%s", "OK"};
                        EasyRequest(0, &es, 0, infonode->node.ln_Name);
                }        
        }
}

static BOOL
executeWBStartup(void)
{
        struct List infoList;
        NEWLIST(&infoList);
        BOOL retvalue = FALSE;
        BPTR olddir = (BPTR)-1;
        BPTR startupdir = 0;
        
        struct FileInfoBlock *fib = AllocDosObjectTags(DOS_FIB, TAG_END);
        if (fib == NULL)
        {
                struct EasyStruct es = {sizeof(struct EasyStruct), 0,
                        "Error", "ExecuteStartup\nOut of memory for FileInfoBlock", "OK"};
                EasyRequest(0, &es, 0);
                goto exit;
        }
        
        if ( (startupdir = Lock(STARTUPDIR, ACCESS_READ) ) == 0)
        {
                D(bug("ExecuteStartup: Couldn't lock " STARTUPDIR "\n"));
                goto exit;
        }        
        
        if ( ! Examine(startupdir, fib))
        {
                struct EasyStruct es = {sizeof(struct EasyStruct), 0,
                        "Error", "ExecuteStartup\nCouldn't examine " STARTUPDIR, "OK"};
                EasyRequest(0, &es, 0);
                goto exit;
        }
        
        // check if startupdir is a directory
        if (fib->fib_DirEntryType >= 0)
        {
                olddir = CurrentDir(startupdir);
                if (searchInfo(&infoList))
                {
                        executePrograms(&infoList);
                        retvalue = TRUE;
                }
        }        
        else
        {
                D(bug("ExecuteStartup: " STARTUPDIR " isn't a directory\n"));
        }

exit:
        // Cleanup
        if (startupdir) UnLock(startupdir);
        if (olddir != (BPTR)-1) CurrentDir(olddir);
        if (fib) FreeDosObject(DOS_FIB, fib);
        
        struct Node *node;
        while ((node = GetHead(&infoList)))
        {
                FreeVec(node->ln_Name);
                Remove(node);
                FreeVec(node);
        }

        return retvalue;
}
