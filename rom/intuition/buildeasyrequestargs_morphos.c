/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#define DEBUG_BUILDEASYREQUEST(x)   ;

/**********************************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <strings.h>
#include <clib/macros.h>
#include <aros/asmcall.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <exec/memory.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <graphics/rastport.h>
#include <graphics/gfxmacros.h>
#include <utility/tagitem.h>
#include "intuition_intern.h"
#include "requesters.h"

/**********************************************************************************************/

static STRPTR *buildeasyreq_makelabels(struct IntRequestUserData *requserdata,STRPTR labeltext,ULONG *args,struct IntuitionBase *IntuitionBase);
static int charsinstring(STRPTR string, char c);

/*****************************************************************************
 
    NAME */
#include <proto/intuition.h>
#include <exec/types.h>
#include <intuition/intuition.h>

AROS_LH4(struct Window *, BuildEasyRequestArgs,

         /*  SYNOPSIS */
         AROS_LHA(struct Window     *, RefWindow, A0),
         AROS_LHA(struct EasyStruct *, easyStruct, A1),
         AROS_LHA(ULONG              , IDCMP, D0),
         AROS_LHA(APTR               , Args, A3),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 99, Intuition)

/*  FUNCTION
    Opens a requester, which provides one or more choices. The control is
    returned to the application after the requester was opened. It is
    handled by subsequent calls to SysReqHandler() and closed by calling
    FreeSysRequest().
 
    INPUTS
    RefWindow - A reference window. If NULL, the requester opens on
            the default public screen.
    easyStruct - The EasyStruct structure (<intuition/intuition.h>),
             which describes the requester.
    IDCMP - IDCMP flags, which should satisfy the requester, too. This is
        useful for requesters, which want to listen to disk changes,
        etc. Note that this is not a pointer to the flags as in
        EasyRequestArgs().
    Args - The arguments for easyStruct->es_TextFormat.
 
    RESULT
    Returns a pointer to the requester. Use this pointer only for calls
    to SysReqHandler() and FreeSysRequest().
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    EasyRequestArgs(), SysReqHandler(), FreeSysRequest()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Screen       *scr = NULL, *lockedscr = NULL;
    struct Window       *req;
    //struct Gadget     *gadgets;
    STRPTR          reqtitle;
    struct IntRequestUserData   *requserdata;
    APTR            nextarg = Args;

    DEBUG_BUILDEASYREQUEST(dprintf("intrequest_buildeasyrequest: window 0x%lx easystruct 0x%lx IDCMPFlags 0x%lx args 0x%lx\n",
                                   (ULONG) RefWindow,
                                   (ULONG) easyStruct,
                                   IDCMP,
                                   (ULONG) Args));

    if (!easyStruct)
        return FALSE;

    DEBUG_BUILDEASYREQUEST(dprintf("intrequest_buildeasyrequest: easy title <%s> Format <%s> Gadgets <%s>\n",
                                   easyStruct->es_Title,
                                   easyStruct->es_TextFormat,
                                   easyStruct->es_GadgetFormat));

    /* get requester title */
    reqtitle = easyStruct->es_Title;
    if ((!reqtitle) && (RefWindow))
        reqtitle = RefWindow->Title;

    if (!reqtitle) reqtitle = "System Request"; /* stegerg: should be localized */

    /* get screen and screendrawinfo */
    if (RefWindow)
        scr = RefWindow->WScreen;

    if (!scr)
    {
        struct Process *proc = (struct Process *)FindTask(0);
        if (proc->pr_Task.tc_Node.ln_Type == NT_PROCESS)
        {
            if (proc->pr_WindowPtr && (proc->pr_WindowPtr != (struct Window *)~0)) scr = ((struct Window *)(proc->pr_WindowPtr))->WScreen;
        }
    }


    if (!scr)
    {
        scr = LockPubScreen(NULL);
        if (!scr)
            return FALSE;
        lockedscr = scr;
    }

    requserdata = AllocVec(sizeof (struct IntRequestUserData),MEMF_PUBLIC|MEMF_CLEAR);
    if (!requserdata) goto fail;

    requserdata->ReqScreen = scr;

    requserdata->Text = intrequest_createitext(requserdata,easyStruct->es_TextFormat,&nextarg,IntuitionBase);
    if (!requserdata->Text) goto fail;

    /* create everything */
    requserdata->GadgetLabels = buildeasyreq_makelabels(requserdata,
                                           easyStruct->es_GadgetFormat,nextarg,
                                           IntuitionBase);
    if (!requserdata->GadgetLabels) goto fail;

    requserdata->ReqGadgets = AllocVec(requserdata->NumGadgets * (sizeof (struct RequesterGadget)),MEMF_PUBLIC);
    if (!requserdata->ReqGadgets) goto fail;

    intrequest_initeasyreq(requserdata,(struct ExtEasyStruct *)easyStruct,IntuitionBase);
    intrequest_layoutrequester(requserdata,IntuitionBase);
    if (!requserdata->wwidth) goto fail;

    if (!(intrequest_creategadgets(requserdata,IntuitionBase))) goto fail;

    {
        struct TagItem win_tags[] =
        {
            { WA_Width                    , requserdata->wwidth },
            { WA_Height                   , requserdata->wheight },
            { WA_Left                     , (scr->Width/2) - (requserdata->wwidth/2) },
            { WA_Top                      , (scr->Height/2) - (requserdata->wheight/2) },
            { WA_IDCMP                    , IDCMP_GADGETUP | IDCMP_RAWKEY | (IDCMP & ~IDCMP_VANILLAKEY) | IDCMP_REFRESHWINDOW},
            { WA_Gadgets                      , (IPTR)requserdata->Gadgets      },
            { WA_Title                    , (IPTR)reqtitle      },
            { (lockedscr ? WA_PubScreen : WA_CustomScreen), (IPTR)scr           },
            { WA_Flags                    , WFLG_DRAGBAR     |
                                        WFLG_DEPTHGADGET |
                                        WFLG_ACTIVATE    |
                                        WFLG_SIMPLE_REFRESH |
                                        WFLG_RMBTRAP    },
            { WA_SkinInfo                     , NULL        },
            { (requserdata->backfilldata.image) ? WA_BackFill : TAG_IGNORE, (ULONG)&requserdata->backfillhook},
            {TAG_DONE                                   }
        };
    
        req = OpenWindowTagList(NULL, win_tags);
    }

    if (req)
    {

        if (lockedscr) UnlockPubScreen(NULL, lockedscr);

        req->UserData = (BYTE *)requserdata;
        requserdata->IDCMP = IDCMP;
        requserdata->ReqWindow = req;

        intrequest_drawrequester(requserdata,IntuitionBase);

        return req;
    }

fail:

    if (requserdata)
    {
        intrequest_freegadgets(requserdata->Gadgets,IntuitionBase);
        intrequest_freelabels(requserdata->GadgetLabels, IntuitionBase);
        if (requserdata->dri) FreeScreenDrawInfo(requserdata->ReqScreen,(struct DrawInfo *)requserdata->dri);
        if (requserdata->freeitext) intrequest_freeitext(requserdata->Text,IntuitionBase);
        if (requserdata->ReqGadgets) FreeVec(requserdata->ReqGadgets);
        if (requserdata->backfilldata.image) int_FreeCustomImage(TYPE_REQUESTERCLASS,requserdata->dri,IntuitionBase);
        if (requserdata->Logo) int_FreeCustomImage(TYPE_REQUESTERCLASS,requserdata->dri,IntuitionBase);
        FreeVec(requserdata);
    }
    if (lockedscr) UnlockPubScreen(NULL, lockedscr);

    return NULL;

    AROS_LIBFUNC_EXIT

} /* BuildEasyRequestArgs */

/**********************************************************************************************/

UWORD BgPattern[2]  = { 0xAAAA, 0x5555 };

/**********************************************************************************************/

/* create an array of gadgetlabels */
static STRPTR *buildeasyreq_makelabels(struct IntRequestUserData *requserdata,
                                       STRPTR labeltext,
                                       ULONG *args,
                                       struct IntuitionBase *IntuitionBase)
{
    STRPTR  *gadgetlabels;
    STRPTR  label;
    int currentgadget;
    struct RawInfo  RawInfo;
    UBYTE   *rawbuf;

    /* copy label-string */
    RawInfo.Len =   0;
    RawInfo.Lines   =   1;
    RawDoFmt(labeltext, args, (VOID_FUNC)AROS_ASMSYMNAME(RequesterCountChar), &RawInfo);

    label = AllocVec(RawInfo.Len, MEMF_PUBLIC);
    if (!label)
    {
        return NULL;
    }
    rawbuf = label;
    RawDoFmt(labeltext, args, (VOID_FUNC)AROS_ASMSYMNAME(RequesterPutChar), &rawbuf);

    /* make room for pointer-array */
    requserdata->NumGadgets = charsinstring(label, '|') + 1;
    gadgetlabels = AllocVec((requserdata->NumGadgets + 1) * sizeof(STRPTR), MEMF_PUBLIC);
    if (!gadgetlabels)
    {
        FreeVec(label);
        return NULL;
    }
    gadgetlabels[requserdata->NumGadgets] = NULL;

    /* set up the pointers and insert null-bytes */
    for (currentgadget = 0; currentgadget < requserdata->NumGadgets; currentgadget++)
    {
        gadgetlabels[currentgadget] = label;
        if (currentgadget != (requserdata->NumGadgets - 1))
        {
            while (label[0] != '|')
                label++;
            label[0] = '\0';
            label++;
        }
    }

    return gadgetlabels;
}

/**********************************************************************************************/

static int charsinstring(STRPTR string, char c)
{
    int count = 0;

    while (string[0])
    {
        if (string[0] == c)
            count++;
        string++;
    }
    return count;
}

/**********************************************************************************************/
