/*
    Copyright 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

//#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <proto/alib.h>

#include <aros/libcall.h>
#include <exec/resident.h>
#include <libraries/dos.h>

#include <stdio.h>
#include <string.h>

#include "gl_intern.h"

#define DOSBase GLB(base)->glb_DOS

static AROS_UFP3 (struct Library *, GM_UNIQUENAME(LibInit),
                  AROS_UFPA(struct Library *, base, D0),
                  AROS_UFPA(BPTR, librarySegment, A0),
                  AROS_UFPA(struct ExecBase *, sysBase, A6)
);
static AROS_LD1 (struct Library *, GM_UNIQUENAME(LibOpen),
                 AROS_LPA (ULONG, version, D0),
                 struct Library *, base, 1, GL
);
static AROS_LD0 (BPTR, GM_UNIQUENAME(LibClose),
                 struct Library *, base, 2, GL
);
static AROS_LD1(BPTR, GM_UNIQUENAME(LibExpunge),
                AROS_LPA(struct Library *, __extrabase, D0),
                struct Library *, base, 3, GL
);

__startup int Main(void)
{
    return RETURN_FAIL;
}

struct ExecBase *SysBase = NULL;

static const char GM_UNIQUENAME(LibName)[] = MOD_NAME_STRING;
static const char GM_UNIQUENAME(LibID)[]   = VERSION_STRING;

static IPTR GM_UNIQUENAME(LibNull)(VOID)
{
    return(0);
}

STATIC CONST CONST_APTR GM_UNIQUENAME(LibVectors)[] =
{
    (CONST_APTR)AROS_SLIB_ENTRY(GM_UNIQUENAME(LibOpen), GL, 1),
    (CONST_APTR)AROS_SLIB_ENTRY(GM_UNIQUENAME(LibClose), GL, 2),
    (CONST_APTR)AROS_SLIB_ENTRY(GM_UNIQUENAME(LibExpunge), GL, 3),
    (CONST_APTR)GM_UNIQUENAME(LibNull),
    (CONST_APTR)-1
};

STATIC CONST IPTR GM_UNIQUENAME(LibInitTab)[] =
{
    sizeof(struct LIBBASE),
    (IPTR)GM_UNIQUENAME(LibVectors),
    (IPTR)NULL,
    (IPTR)GM_UNIQUENAME(LibInit)
};

static const struct Resident ROMTag __attribute__((used)) =
{
    RTC_MATCHWORD,
    (struct Resident *)&ROMTag,
    (struct Resident *)(&ROMTag + 1),
    RESIDENTFLAGS,
    VERSION_NUMBER,
    NT_LIBRARY,
    0,
    (char *)GM_UNIQUENAME(LibName),
    (char *)(GM_UNIQUENAME(LibID) + 6),          // +6 to skip '$VER: '
    (APTR)GM_UNIQUENAME(LibInitTab),
    REVISION_NUMBER,
    0
};

#define BUFFER_SIZE 256

char *GetGLVarPath(struct Library *base)
{
    char pathBuff[BUFFER_SIZE];
    BPTR pathLock = BNULL;

    D(bug("[GL] %s()\n", __PRETTY_FUNCTION__));

    if ((pathLock = Lock("ENV:SYS", SHARED_LOCK)) != BNULL)
    {
        if (NameFromLock(pathLock, pathBuff, BUFFER_SIZE))
        {
            GLB(base)->glb_Notify.nr_Name = AllocVec(strlen(pathBuff) + 4, MEMF_PUBLIC);
            sprintf(GLB(base)->glb_Notify.nr_Name, "%s/GL", pathBuff);
            D(bug("[GL] %s: using '%s'\n", __PRETTY_FUNCTION__, GLB(base)->glb_Notify.nr_Name));
        }
        UnLock(pathLock);
    }
    return GLB(base)->glb_Notify.nr_Name;
}

void GetGLVar(struct Library *base)
{
    LONG            Var_Length;
    char            Var_Value[BUFFER_SIZE];

    D(bug("[GL] %s()\n", __PRETTY_FUNCTION__));

    FreeVec(GLB(base)->glb_GLImpl);

    Var_Length = GetVar((STRPTR)"SYS/GL",
              &Var_Value[0],
              BUFFER_SIZE,
              GVF_GLOBAL_ONLY | LV_VAR
     );

    if (Var_Length != -1)
    {
        if ((GLB(base)->glb_GLImpl = AllocVec(strlen(Var_Value) + strlen(".library") + 1, MEMF_PUBLIC)) != NULL)
        {
            sprintf(GLB(base)->glb_GLImpl, "%s.library", Var_Value);
            D(bug("[GL] %s: using '%s' for %s\n", __PRETTY_FUNCTION__, GLB(base)->glb_GLImpl, Var_Value));
        }
   }
}

void gl_EnvNotifyProc(void)
{
    struct Process *pr = (struct Process *) FindTask (NULL);
    struct Library *base = ((struct Task *)pr)->tc_UserData;
    struct Message *msg;

    D(bug("[GL] %s()\n", __PRETTY_FUNCTION__));

    while (TRUE)
    {
        WaitPort (&pr->pr_MsgPort);
        while ((msg = GetMsg (&pr->pr_MsgPort)))
            ReplyMsg(msg);

        GetGLVar(base);
    }
}

void SetupGLVarNotification(struct Library *base)
{
    D(bug("[GL] %s()\n", __PRETTY_FUNCTION__));

    if (!GLB(base)->glb_Notify.nr_Name)
        GetGLVarPath(base);

    if (GLB(base)->glb_Notify.nr_Name)
    {
        if (!GLB(base)->glb_Notify.nr_stuff.nr_Msg.nr_Port)
        {
            struct TagItem enprocTags[] =
            {
                { NP_Entry,         (IPTR)gl_EnvNotifyProc  },
                { NP_UserData,      (IPTR)base              },
                { TAG_END,          0                       }
            };
            struct Process *enProc;
            if ((enProc = CreateNewProc(enprocTags)) != NULL)
            {
                D(bug("[GL] %s: FS Notification Proc @ 0x%p for '%s'\n", __PRETTY_FUNCTION__, enProc, GLB(base)->glb_Notify.nr_Name));

                GLB(base)->glb_Notify.nr_stuff.nr_Msg.nr_Port	= &enProc->pr_MsgPort;
            }
            D(bug("[GL] %s: MsgPort @ 0x%p\n", __PRETTY_FUNCTION__,GLB(base)->glb_Notify.nr_stuff.nr_Msg.nr_Port));
        }

        if (GLB(base)->glb_Notify.nr_stuff.nr_Msg.nr_Port && !GLB(base)->glb_Notify.nr_FullName)
        {
            GLB(base)->glb_Notify.nr_Flags		        = NRF_SEND_MESSAGE|NRB_NOTIFY_INITIAL;
            StartNotify(&GLB(base)->glb_Notify);
            D(bug("[GL] %s: Started FS Notification for '%s'\n", __PRETTY_FUNCTION__, GLB(base)->glb_Notify.nr_FullName));
        }
    }
}

static AROS_UFH3(struct Library *, GM_UNIQUENAME(LibInit),
                 AROS_UFHA(struct Library *, base, D0),
                 AROS_UFHA(BPTR, librarySegment, A0),
                 AROS_UFHA(struct ExecBase *, sysBase, A6)
)
{
    AROS_USERFUNC_INIT

    SysBase = sysBase;

    D(bug("[GL] %s()\n", __PRETTY_FUNCTION__));

    D(bug("[GL] %s: base @ 0x%p, SysBase @ 0x%p\n", __PRETTY_FUNCTION__, base, SysBase));

    // setup the library structure.
    GLB(base)->glb_Lib.lib_Node.ln_Type = NT_LIBRARY;
    GLB(base)->glb_Lib.lib_Node.ln_Pri  = 0;
    GLB(base)->glb_Lib.lib_Node.ln_Name = (char *)GM_UNIQUENAME(LibName);
    GLB(base)->glb_Lib.lib_Flags        = LIBF_CHANGED | LIBF_SUMUSED;
    GLB(base)->glb_Lib.lib_Version      = VERSION_NUMBER;
    GLB(base)->glb_Lib.lib_Revision     = REVISION_NUMBER;
    GLB(base)->glb_Lib.lib_IdString     = (char *)(GM_UNIQUENAME(LibID) + 6);

    GLB(base)->glb_DOS = OpenLibrary("dos.library", 0);

    D(bug("[GL] %s: DOSBase @ 0x%p\n", __PRETTY_FUNCTION__, DOSBase));

    memset(&GLB(base)->glb_Sem, 0,
        sizeof(struct SignalSemaphore) + sizeof(struct NotifyRequest));
    InitSemaphore(&GLB(base)->glb_Sem);

    SetupGLVarNotification(base);

    // return the library base as success
    return base;

    AROS_USERFUNC_EXIT
}

static AROS_LH1(struct Library *, GM_UNIQUENAME(LibOpen),
                AROS_LHA(ULONG, version, D0),
                struct Library *, base, 1, GL
)
{
    AROS_LIBFUNC_INIT

    struct Library *res = NULL;
    char *glImplementation;

    D(bug("[GL] %s()\n", __PRETTY_FUNCTION__));

    // delete the late expunge flag
    GLB(base)->glb_Lib.lib_Flags &= ~LIBF_DELEXP;

    if (GLB(base)->glb_GLImpl)
        glImplementation = GLB(base)->glb_GLImpl;
    else
        glImplementation = "mesa3dgl.library";

    D(bug("[GL] %s: Attempting to use '%s' version %d\n", __PRETTY_FUNCTION__, glImplementation, version));

    res = OpenLibrary(glImplementation, version);

    return res;

    AROS_LIBFUNC_EXIT
}

/* NB: we dont ever really close */

static AROS_LH0(BPTR, GM_UNIQUENAME(LibClose),
                struct Library *, base, 2, GL
)
{
    AROS_LIBFUNC_INIT

    BPTR rc = 0;

    D(bug("[GL] %s()\n", __PRETTY_FUNCTION__));

    return rc;

    AROS_LIBFUNC_EXIT
}

static AROS_LH1(BPTR, GM_UNIQUENAME(LibExpunge),
  AROS_LHA(struct Library *, __extrabase, D0),
  struct Library *, base, 3, GL
)
{
    AROS_LIBFUNC_INIT

    BPTR rc = 0;

    D(bug("[GL] %s()\n", __PRETTY_FUNCTION__));

    return rc;

    AROS_LIBFUNC_EXIT
}
