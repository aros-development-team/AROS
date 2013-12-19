/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: check if the given window is an app window and send it a list of files
    Lang: English
*/

#include <exec/types.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <intuition/intuition.h>

#include <proto/utility.h>

#include "workbench_intern.h"
#include <workbench/workbench.h>

#include <string.h>

static char *allocPath(char *str, APTR WorkbenchBase);

/*****************************************************************************

    NAME */
    #include <proto/workbench.h>

        AROS_LH8(BOOL, SendAppWindowMessage,
/*  SYNOPSIS */
        AROS_LHA(struct Window *, win, A0),
        AROS_LHA(ULONG, numfiles, D0),
        AROS_LHA(char **, files, A1),
        AROS_LHA(UWORD, class, D1),
        AROS_LHA(WORD, mousex, D2),
        AROS_LHA(WORD, mousey, D3),
        AROS_LHA(ULONG, seconds, D4),
        AROS_LHA(ULONG, micros, D5),

/*  LOCATION */
        struct WorkbenchBase *, WorkbenchBase, 26, Workbench)

/*  FUNCTION
        This function sends the given list of files to a registered
        AppWindow's application. If the window is not an AppWindow, nothing
        is done.

    INPUTS
        win -  window that should be checked
        numfiles - number of files in the attached array of string pointers
        files - files "list"

    RESULT
        TRUE if action succeeded

    NOTES

    EXAMPLE

        char *FileList[] =
            {"images:image1.png", "images:image2.png", "images:image3.png"};

        SendAppWindowMessage(myWindow, 3, FileList);

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct List *awl = NULL;

    BOOL success = FALSE;

    if (numfiles == 0) return FALSE;

    LockWorkbenchShared();
    awl = &WorkbenchBase->wb_AppWindows;
    if (!IsListEmpty(awl))
    {
        struct Node *succ;
        struct Node *s = awl->lh_Head;
        struct AppWindow *aw = NULL;
        int    i;
        BOOL   fail = FALSE;

        struct AppMessage *am = AllocVec(sizeof (struct AppMessage), MEMF_CLEAR);
        if (am)
        {
            while (((succ = ((struct Node*) s)->ln_Succ) != NULL) && (aw == NULL))
            {
                if ((((struct AppWindow *) s)->aw_Window) == win)
                {
                    aw = (struct AppWindow *) s;
                }
                s =  succ;
            }
            if (aw)
            {
                struct WBArg *wbargs = AllocVec(sizeof(struct WBArg) * numfiles, MEMF_CLEAR);
                if (wbargs)
                {
                    struct WBArg *wb = wbargs;
                    for (i = 0; i < numfiles; i++)
                    {
                        wb->wa_Name = FilePart(files[i]);
                        char *path = allocPath(files[i], WorkbenchBase);
                        if (path)
                        {
                            wb->wa_Lock = Lock(path, SHARED_LOCK);
                            if (wb->wa_Lock == 0) fail = TRUE;
                            FreeVec(path);
                        }
                        wb++;
                    }
                    if (!fail)
                    {
                        struct MsgPort *port = aw->aw_MsgPort;
                        am->am_NumArgs = numfiles;
                        am->am_ArgList = wbargs;
                        am->am_ID = aw->aw_ID;
                        am->am_UserData = aw->aw_UserData;
                        am->am_Type = AMTYPE_APPWINDOW;
                        am->am_Version = AM_VERSION;
                        am->am_Class = class;
                        am->am_MouseX = mousex;
                        am->am_MouseY = mousey;
                        am->am_Seconds = seconds;
                        am->am_Micros = micros;
                        struct MsgPort *reply = CreateMsgPort();
                        if (reply)
                        {
                            am->am_Message.mn_ReplyPort = reply;
                            PutMsg(port, (struct Message *) am);
                            WaitPort(reply);
                            GetMsg(reply);
                            DeleteMsgPort(reply);
                            success = TRUE;
                        }
                    }
                    wb = wbargs;
                    for (i = 0; i < numfiles; i++)
                    {
                        if (wb->wa_Lock) UnLock(wb->wa_Lock);
                        wb++;
                    }
                    FreeVec(wbargs);
                }
            }
            FreeVec(am);
        }
    }
    UnlockWorkbench();

    return success;

    AROS_LIBFUNC_EXIT
} /* SendAppWindowMessage() */

/*****************************************************************************/

static char *allocPath(char *str, APTR WorkbenchBase)
{
    char *s0, *s1, *s;

    s = NULL;
    s0 = str;

    s1 = PathPart(str);
    if (s1)
    {
        int  l;

        for (l = 0; s0 != s1; s0++,l++);
        s = AllocVec(l + 1, MEMF_CLEAR);
        if (s) strncpy(s, str, l);
    }
    return s;
}
