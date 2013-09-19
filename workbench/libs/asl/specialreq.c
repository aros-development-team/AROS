/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Special requesters needed for things like Delete in File Requester.
    Lang: english
*/


#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <intuition/screens.h>
#include <intuition/gadgetclass.h>
#include <graphics/gfx.h>
#include <string.h>

#include "asl_intern.h"
#include "layout.h"
#include "specialreq.h"

#if USE_SHARED_COOLIMAGES
#include <libraries/coolimages.h>
#include <proto/coolimages.h>
#else
#include "coolimages.h"
#endif

#define SDEBUG 0
#define DEBUG 0

#include <aros/debug.h>

/*****************************************************************************************/

#define OK_ID           1
#define CANCEL_ID       2
#define STRING_ID       3

/*****************************************************************************************/

STRPTR REQ_String(STRPTR title, STRPTR stringtext, STRPTR oktext, STRPTR canceltext,
                  struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct Window       *win;
    struct Hook         edithook;
    Object              *okbutton = NULL;
    Object              *cancelbutton = NULL;
    Object              *textstring = NULL;
    WORD                x, x2, y, y2, winwidth, winheight;
    WORD                buttonwidth, buttonheight;
#if USE_SHARED_COOLIMAGES
    const struct CoolImage *useimg = NULL;
    const struct CoolImage *cancelimg = NULL;

    #define cool_useimage       (*useimg)
    #define cool_cancelimage    (*cancelimg)

#endif

    STRPTR              retval = NULL;

    /* Init */

#if USE_SHARED_COOLIMAGES
#if SPECIALREQ_COOL_BUTTONS
    if (CoolImagesBase)
    {
        useimg    = COOL_ObtainImageA(COOL_USEIMAGE_ID, NULL);
        cancelimg = COOL_ObtainImageA(COOL_CANCELIMAGE_ID, NULL);
    }
#endif
#endif

    if (!stringtext) stringtext = "";

    edithook.h_Entry    = (APTR)AROS_ASMSYMNAME(StringEditFunc);
    edithook.h_SubEntry = NULL;
    edithook.h_Data     = AslBase;

    /* Calc. button width */

    x = TextLength(&ld->ld_DummyRP, oktext, strlen(oktext));

#if USE_SHARED_COOLIMAGES
    if (useimg)
#endif
#if SPECIALREQ_COOL_BUTTONS
    if (ld->ld_TrueColor)
    {
        x += IMAGEBUTTONEXTRAWIDTH + cool_useimage.width;
    }
#endif

    x2 = TextLength(&ld->ld_DummyRP, canceltext, strlen(canceltext));

#if USE_SHARED_COOLIMAGES
    if (cancelimg)
#endif
#if SPECIALREQ_COOL_BUTTONS
    if (ld->ld_TrueColor)
    {
        x2 += IMAGEBUTTONEXTRAWIDTH + cool_cancelimage.width;
    }
#endif

    buttonwidth = BUTTONEXTRAWIDTH + ((x2 > x) ? x2 : x);

    /* Calc. button height */

#if SPECIALREQ_COOL_BUTTONS
    y = BUTTONEXTRAHEIGHT + ld->ld_Font->tf_YSize;
    if (ld->ld_TrueColor)
    {
        y2 = IMAGEBUTTONEXTRAHEIGHT + DEF_COOLIMAGEHEIGHT;
    }
    else
    {
        y2 = 0;
    }
    buttonheight = (y > y2) ? y : y2;
#else
    buttonheight = BUTTONEXTRAHEIGHT + ld->ld_Font->tf_YSize;
#endif

    /* calc. win size */

    winwidth = OUTERSPACINGX * 2 +
               buttonwidth * 3;

    winheight = OUTERSPACINGY * 2 + buttonheight * 2 + GADGETSPACINGY;

    x = ld->ld_WBorLeft + OUTERSPACINGX;
    y = ld->ld_WBorTop + OUTERSPACINGY + buttonheight + GADGETSPACINGY;

    /* Make button gadgets */

    {
        struct TagItem button_tags[] =
        {
            {GA_Text            , (IPTR)oktext          },
            {TAG_IGNORE         , 0                     },
            {GA_ID              , OK_ID                 },
            {TAG_IGNORE         , 0                     },
            {GA_UserData        , (IPTR)ld              },
            {GA_Left            , x                     },
            {GA_Top             , y                     },
            {GA_Width           , buttonwidth           },
            {GA_Height          , buttonheight          },
            {GA_Image           , 0                     }, /* 0 means we want a frame */
            {GA_RelVerify       , TRUE                  },
            {TAG_DONE                                   }
        };

#if USE_SHARED_COOLIMAGES
        if (useimg)
        {
            button_tags[3].ti_Tag  = ASLBT_CoolImage;
            button_tags[3].ti_Data = (IPTR)useimg;
        }
#else
        button_tags[3].ti_Tag  = ASLBT_CoolImage;
        button_tags[3].ti_Data = (IPTR)&cool_useimage;
#endif

        okbutton = NewObjectA(AslBase->aslbuttonclass, NULL, button_tags);

        button_tags[0].ti_Data = (IPTR)canceltext;
        if (okbutton) button_tags[1].ti_Tag = GA_Previous;
        button_tags[1].ti_Data = (IPTR)okbutton;
        button_tags[2].ti_Data = CANCEL_ID;

#if USE_SHARED_COOLIMAGES
        if (!cancelimg)
        {
            button_tags[3].ti_Tag = TAG_IGNORE;
        }
        else
        {
            button_tags[3].ti_Data = (IPTR)cancelimg;
        }
#else
        button_tags[3].ti_Data = (IPTR)&cool_cancelimage;
#endif

        button_tags[5].ti_Data += buttonwidth * 2;

        cancelbutton = NewObjectA(AslBase->aslbuttonclass, NULL, button_tags);

        if (okbutton && cancelbutton)
        {
            /* Make String Gadget */

            struct TagItem string_tags[] =
            {
                {GA_Previous            , (IPTR)cancelbutton                    },
                {GA_ID                  , STRING_ID                             },
                {GA_Left                , x                                     },
                {GA_Top                 , y - buttonheight - GADGETSPACINGY     },
                {GA_Width               , winwidth - OUTERSPACINGX * 2          },
                {GA_Height              , buttonheight                          },
                {GA_RelVerify           , TRUE                                  },
                {GA_UserData            , (IPTR)ld                              },
                {STRINGA_MaxChars       , 256                                   },
                {STRINGA_TextVal        , (IPTR)stringtext                      },
                {STRINGA_BufferPos      , strlen(stringtext)                    },
                {STRINGA_EditHook       , (IPTR)&edithook                       },
                {TAG_DONE                                                       }
            };

            textstring = NewObjectA(AslBase->aslstringclass, NULL, string_tags);
            if (textstring)
            {
                x = ld->ld_Window->LeftEdge +
                    ld->ld_Window->Width / 2 -
                    (winwidth + ld->ld_WBorLeft + ld->ld_WBorRight) / 2;

                y = ld->ld_Window->TopEdge +
                    ld->ld_Window->Height / 2 -
                    (winheight + ld->ld_WBorTop + ld->ld_WBorBottom) / 2;

                win = OpenWindowTags(NULL, WA_CustomScreen, (IPTR)ld->ld_Screen,
                                           WA_Left              , x                     ,
                                           WA_Top               , y                     ,
                                           WA_InnerWidth        , winwidth              ,
                                           WA_InnerHeight       , winheight             ,
                                           WA_AutoAdjust        , TRUE                  ,
                                           WA_Title             , (IPTR)title           ,
                                           WA_CloseGadget       , TRUE                  ,
                                           WA_DragBar           , TRUE                  ,
                                           WA_DepthGadget       , TRUE                  ,
                                           WA_Activate          , TRUE                  ,
                                           WA_SimpleRefresh     , TRUE                  ,
                                           WA_NoCareRefresh     , TRUE                  ,
                                           WA_RMBTrap           , TRUE                  ,
                                           WA_IDCMP             , IDCMP_CLOSEWINDOW |
                                                                  IDCMP_GADGETUP    |
                                                                  IDCMP_VANILLAKEY  |
                                                                  IDCMP_RAWKEY          ,
                                           WA_Gadgets           , (IPTR)okbutton        ,
                                           TAG_DONE);

                if (win)
                {
                    BOOL quitme = FALSE, doit = FALSE;

                    ActivateGadget((struct Gadget *)textstring, win, NULL);

                    while(!quitme && !doit)
                    {
                        struct IntuiMessage *msg;

                        WaitPort(win->UserPort);
                        while((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
                        {
                            switch(msg->Class)
                            {
                                case IDCMP_CLOSEWINDOW:
                                    quitme = TRUE;
                                    break;

                                case IDCMP_GADGETUP:
                                    switch(((struct Gadget *)msg->IAddress)->GadgetID)
                                    {
                                        case OK_ID:
                                            doit = TRUE;
                                            break;

                                        case CANCEL_ID:
                                            quitme = TRUE;
                                            break;

                                        case STRING_ID:
                                            switch(msg->Code)
                                            {
                                                case 0:
                                                   doit = TRUE;
                                                   break;

                                                case 27:
                                                   quitme = TRUE;
                                                   break;

                                            }
                                            break;

                                    } /* switch(((struct Gadget *)msg->IAddress)->GadgetID) */
                                    break;

                                case IDCMP_VANILLAKEY:
                                    switch(msg->Code)
                                    {
                                        case 27:
                                            quitme = TRUE;
                                            break;

                                        case 9:
                                            ActivateGadget((struct Gadget *)textstring, win, NULL);
                                            break;

                                    }
                                    break;

                            } /* switch(msg->Class) */
                            ReplyMsg((struct Message *)msg);

                        } /* while((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) */

                    } /* while(!quitme && !doit) */

                    if (doit)
                    {
                        STRPTR text;
                        LONG   len;

                        GetAttr(STRINGA_TextVal, textstring, (IPTR *)&text);

                        len = strlen(text);
                        if (len > 0)
                        {
                            retval = VecPooledCloneString(text, NULL, ld->ld_IntReq->ir_MemPool, AslBase);
                            if (retval) strcpy(retval, text);
                        }

                    } /* if (doit) */

                    RemoveGadget(win, (struct Gadget *)okbutton);
                    RemoveGadget(win, (struct Gadget *)cancelbutton);
                    RemoveGadget(win, (struct Gadget *)textstring);

                    CloseWindow(win);

                } /* if (win) */

                DisposeObject(textstring);

            } /* if (textstring) */

        } /* if (okbutton && cancelbutton) */

        if (okbutton) DisposeObject(okbutton);
        if (cancelbutton) DisposeObject(cancelbutton);

    } /**/

    return retval;

}

/*****************************************************************************************/
