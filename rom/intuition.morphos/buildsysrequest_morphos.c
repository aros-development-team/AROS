/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#define DEBUG_BUILDSYSREQUEST(x)    ;

/**********************************************************************************************/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <stdio.h>
#include <stdarg.h>
#include <strings.h>
#include <clib/macros.h>
#include <exec/memory.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/screens.h>
#include <graphics/rastport.h>
#include <graphics/gfxmacros.h>
#include <utility/tagitem.h>
#include "intuition_intern.h"
#include "requesters.h"

/**********************************************************************************************

    NAME */
#include <proto/intuition.h>
#include <exec/types.h>
#include <intuition/intuition.h>

AROS_LH7(struct Window *, BuildSysRequest,

         /*  SYNOPSIS */
         AROS_LHA(struct Window *   , window, A0),
         AROS_LHA(struct IntuiText *, bodytext, A1),
         AROS_LHA(struct IntuiText *, postext, A2),
         AROS_LHA(struct IntuiText *, negtext, A3),
         AROS_LHA(ULONG             , IDCMPFlags , D0),
         AROS_LHA(WORD              , width, D2),
         AROS_LHA(WORD              , height, D3),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 60, Intuition)

/*  FUNCTION
 
    INPUTS
    window - The window in which the requester will appear
    bodytext - The Text to be shown in the body of the requester
    postext - The Text to be shown in the positive choice gadget
    negtext - The Text to be shown in the negative choice gadget
    IDCMPFlags - The IDCMP Flags for this requester
    width, height - The dimensions of the requester
 
    RESULT
 
    NOTES
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
    FreeSysRequest(), DisplayAlert(), ModifyIDCMP(), exec-library/Wait(),
    Request(), AutoRequest(), EasyRequest(), BuildEasyRequestArgs()
 
    INTERNALS
 
    HISTORY
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    struct Screen               *scr = NULL, *lockedscr = NULL;
    struct Window               *req;
    //struct Gadget             *gadgets;
    STRPTR                      reqtitle;
    //STRPTR                    gadgetlabels[3];
    struct IntRequestUserData   *requserdata;
    int                         negtextlen;

    DEBUG_BUILDSYSREQUEST(dprintf("intrequest_buildsysrequest: window 0x%lx body <%s> postext <%s> negtext <%s> IDCMPFlags 0x%lx width %ld height %ld\n",
                                  (ULONG) window,
                                  bodytext ? (char *) bodytext->IText : "<NULL>",
                                  postext->IText ? (char *) postext->IText : "<NULL>",
                                  negtext->IText ? (char *) negtext->IText : "<NULL>",
                                  IDCMPFlags,
                                  (LONG) width,
                                  (LONG) height));

    /* negtext and bodytest must be specified, postext is optional */
    if (!negtext || !bodytext) return NULL;

    /* get requester title */

    reqtitle = NULL;
    if (window) reqtitle = window->Title;
    if (!reqtitle) reqtitle = "System Request"; /* stegerg: should be localized */

    /* get screen and screendrawinfo */
    if (window)
        scr = window->WScreen;
    if (!scr)
    {
        scr = LockPubScreen(NULL);
        if (!scr)
            return NULL;
        lockedscr = scr;
    }

    requserdata = AllocVec(sizeof (struct IntRequestUserData),MEMF_PUBLIC|MEMF_CLEAR);
    if (!requserdata) goto fail;

    requserdata->ReqScreen = scr;

    negtextlen = strlen(negtext->IText) + 1;

    if (postext)
    {
        int postextlen = strlen(postext->IText) + 1;

        requserdata->NumGadgets = 2;
        requserdata->GadgetLabels = AllocVec((requserdata->NumGadgets + 1) * sizeof(STRPTR), MEMF_PUBLIC|MEMF_CLEAR);
        if (!requserdata->GadgetLabels) goto fail;
        requserdata->GadgetLabels[0] = AllocVec(postextlen + negtextlen, MEMF_PUBLIC);
        if (!requserdata->GadgetLabels[0])
        {
            FreeVec(requserdata->GadgetLabels); goto fail;
        }
        CopyMem(postext->IText,requserdata->GadgetLabels[0], postextlen);
        requserdata->GadgetLabels[1] = requserdata->GadgetLabels[0] + postextlen;
        CopyMem(negtext->IText,requserdata->GadgetLabels[1], negtextlen);
    } else {
        requserdata->NumGadgets = 1;
        requserdata->GadgetLabels = AllocVec((requserdata->NumGadgets + 1) * sizeof(STRPTR), MEMF_PUBLIC|MEMF_CLEAR);
        if (!requserdata->GadgetLabels) goto fail;
        requserdata->GadgetLabels[0] = AllocVec(negtextlen, MEMF_PUBLIC);
        if (!requserdata->GadgetLabels[0])
        {
            FreeVec(requserdata->GadgetLabels); goto fail;
        }
        CopyMem(negtext->IText,requserdata->GadgetLabels[0], negtextlen);
    }

    /* create everything */

    requserdata->Text = bodytext;
    
    {
        struct IntuiText *it = bodytext;
        while (it)
        {
            requserdata->NumLines++;
            it = it->NextText;
        }
    }

    requserdata->ReqGadgets = AllocVec(requserdata->NumGadgets * (sizeof (struct RequesterGadget)),MEMF_PUBLIC);
    if (!requserdata->ReqGadgets) goto fail;

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
            { WA_IDCMP                    , IDCMP_GADGETUP | IDCMP_RAWKEY | (IDCMPFlags & ~IDCMP_VANILLAKEY) | IDCMP_REFRESHWINDOW},
            { WA_Gadgets                      , (IPTR)requserdata->Gadgets      },
            { WA_Title                    , (IPTR)reqtitle      },
            { (lockedscr ? WA_PubScreen : WA_CustomScreen), (IPTR)scr           },
            { WA_Flags                    , WFLG_DRAGBAR     |
                                        WFLG_DEPTHGADGET |
                                        WFLG_ACTIVATE    |
                                        WFLG_SIMPLE_REFRESH |
                                        WFLG_RMBTRAP    },
            { WA_SkinInfo                 , NULL        },
            {TAG_DONE                                   }
        };
    
        req = OpenWindowTagList(NULL, win_tags);
    }

    if (req)
    {

        if (lockedscr) UnlockPubScreen(NULL, lockedscr);

        req->UserData = (BYTE *)requserdata;
        requserdata->IDCMP = IDCMPFlags;
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
        FreeVec(requserdata);
    }

    if (lockedscr) UnlockPubScreen(NULL, lockedscr);

    return NULL;

    AROS_LIBFUNC_EXIT

} /* BuildSysRequest */

/**********************************************************************************************/
