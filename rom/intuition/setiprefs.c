/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/exec.h>
#include <intuition/iprefs.h>
#include <intuition/pointerclass.h>
#include <prefs/pointer.h>
#include <prefs/palette.h>

#include <proto/intuition.h>
#include <proto/graphics.h>

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH3(ULONG, SetIPrefs,

/*  SYNOPSIS */
        AROS_LHA(APTR , data, A0),
        AROS_LHA(ULONG, length, D0),
        AROS_LHA(ULONG, type, D1),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 96, Intuition)

/*  FUNCTION

    INPUTS

    RESULT
        Depending on the operation.

    NOTES
        This function is currently considered private.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct GfxBase *GfxBase = GetPrivIBase(IntuitionBase)->GfxBase;
    ULONG Result = TRUE;
    ULONG lock = LockIBase(0);

    DEBUG_SETIPREFS(bug("[Intuition] %s: data %p length %lu type %lu\n", __func__, data, length, type));

    switch (type)
    {
        case IPREFS_TYPE_ICONTROL_V37:
            DEBUG_SETIPREFS(bug("[Intuition] %s: IP_ICONTROL_V37\n", __func__));
            if (length > sizeof(struct IIControlPrefs))
                length = sizeof(struct IIControlPrefs);
            CopyMem(data, &GetPrivIBase(IntuitionBase)->IControlPrefs, length);

            DEBUG_SETIPREFS(bug("[Intuition] %s: Drag modes: 0x%04lX\n", __func__, GetPrivIBase(IntuitionBase)->IControlPrefs.ic_VDragModes[0]));

            break;

        case IPREFS_TYPE_SCREENMODE_V37:
        {
            BOOL reopen = FALSE, closed = (GetPrivIBase(IntuitionBase)->WorkBench) ? FALSE : TRUE;

            DEBUG_SETIPREFS(bug("[Intuition] %s: IP_SCREENMODE_V37\n", __func__));
            if (length > sizeof(struct IScreenModePrefs))
                length = sizeof(struct IScreenModePrefs);

            if (!GetPrivIBase(IntuitionBase)->ScreenModePrefs)
                GetPrivIBase(IntuitionBase)->ScreenModePrefs = AllocMem(sizeof(struct IScreenModePrefs), MEMF_ANY);

            if (memcmp(GetPrivIBase(IntuitionBase)->ScreenModePrefs, data,
                       sizeof(struct IScreenModePrefs)) == 0)
                break;

            if (!closed)
            {
                BOOL try = TRUE;

                reopen = TRUE;

                UnlockIBase(lock);

                while (try && !(closed = CloseWorkBench()))
                {
                    struct EasyStruct es =
                    {
                        sizeof(struct EasyStruct),
                        0,
                        "System Request",
                        "Intuition is attempting to reset the screen,\n"
                        "please close all windows except Wanderer's ones.",
                        "Retry|Cancel"
                    };

                    try = EasyRequestArgs(NULL, &es, NULL, NULL) == 1;
                }
            }

            if (closed)
                CopyMem(data, GetPrivIBase(IntuitionBase)->ScreenModePrefs, sizeof(struct IScreenModePrefs));

            if (reopen)
            {
                if (closed)
                {
                    if (!OpenWorkBench())
                    {
                        /* FIXME: handle the error condition if OpenWorkBench() fails */
                        /* What to do if OpenWorkBench() fails? Try until it succeeds?
                           Try for a finite amount of times? Don't try and do nothing 
                           at all? */
                    }
                }
                else
                    Result = FALSE;
                return Result;
            }

            break;
        }

        case IPREFS_TYPE_POINTER_V39:
        DEBUG_SETIPREFS(bug("[Intuition] %s: IP_POINTER_V39\n", __func__));
        {
            struct IPointerPrefs *fp = data;
            struct TagItem pointertags[] = {
                {POINTERA_BitMap , (IPTR)fp->BitMap},
                {POINTERA_XOffset, fp->XOffset     },
                {POINTERA_YOffset, fp->YOffset     },
                {TAG_DONE        , 0               }
            };

            Object *pointer = NewObjectA(
                          GetPrivIBase(IntuitionBase)->pointerclass,
                          NULL,
                          pointertags);

            Object **oldptr = fp->Which ?
                      &GetPrivIBase(IntuitionBase)->BusyPointer :
                      &GetPrivIBase(IntuitionBase)->DefaultPointer;

            InstallPointer(IntuitionBase, fp->Which, oldptr, pointer);
            /* return -1 so that WB3.x C:IPrefs is happy */
            Result = -1;
        }
        break;

        case IPREFS_TYPE_POINTER_V37:
        DEBUG_SETIPREFS(bug("[Intuition] %s: IP_POINTER_V37\n", __func__));
        {
            struct Preferences *ActivePrefs = &GetPrivIBase(IntuitionBase)->ActivePreferences;
            struct IPointerPrefsV37 *fp = data;
            UWORD size = fp->YSize * 2;
            Object *pointer;

            if (size > POINTERSIZE)
                size = POINTERSIZE;
            memset(ActivePrefs->PointerMatrix, 0, POINTERSIZE * sizeof (UWORD));
            CopyMem(fp->data, ActivePrefs->PointerMatrix, size * sizeof (UWORD));
            ActivePrefs->XOffset = fp->XOffset;
            ActivePrefs->YOffset = fp->YOffset;

            pointer = MakePointerFromPrefs(IntuitionBase, ActivePrefs);
            if (pointer)
                 InstallPointer(IntuitionBase, WBP_NORMAL, &GetPrivIBase(IntuitionBase)->DefaultPointer, pointer);
            /* return -1 so that WB2.x C:IPrefs is happy */
            Result = -1;
        }
        break;

        case IPREFS_TYPE_PALETTE_V39:
        case IPREFS_TYPE_PALETTE_V37:
        DEBUG_SETIPREFS(bug("[Intuition] %s: IP_PALETTE_V%d %p %d\n", __func__, type == IPREFS_TYPE_PALETTE_V39 ? 39 : 37, data, length));
        {
            struct ColorSpec *pp = data;
            struct Color32 *p = GetPrivIBase(IntuitionBase)->Colors;
            BOOL update_pointer = FALSE;
            struct Preferences *ActivePrefs = &GetPrivIBase(IntuitionBase)->ActivePreferences;

            DEBUG_SETIPREFS(bug("[Intuition] %s: Intuition Color32 Table 0x%p\n", __func__, p));

            while (pp->ColorIndex != -1)
            {
                WORD idx;
                    
                idx = pp->ColorIndex;
                DEBUG_SETIPREFS(bug("[Intuition] %s: Index %ld R 0x%04lX G 0x%04lX B 0x%04lX\n",
                                    __func__, idx, pp->Red, pp->Green, pp->Blue));
                if (type == IPREFS_TYPE_PALETTE_V37) {
                    /* v37 cursor colors are 17 to 19 */
                    if (idx >= 17)
                        idx = idx - 17 + 8;
                    else if (idx >= 8)
                        idx = -1;
                }
                if (idx >= 0 && idx < COLORTABLEENTRIES)
                {
                    UWORD red, green, blue;
                    if (type == IPREFS_TYPE_PALETTE_V37) {
                        /* 4-bit color components */
                        red = (pp->Red << 4) | pp->Red;
                        green = (pp->Green << 4) | pp->Green;
                        blue = (pp->Blue << 4) | pp->Blue;
                        red = (red << 8) | red;
                        green = (green << 8) | green;
                        blue = (blue << 8) | blue;
                    } else {
                        /* 8-bit color components */
                        red = pp->Red;
                        green = pp->Green;
                        blue = pp->Blue;
                    }
                   
                    p[idx].red   = (red << 16) | red;
                    p[idx].green = (green << 16) | green;
                    p[idx].blue  = (blue << 16) | blue;

                    /* Update oldstyle preferences */
                    if (ActivePrefs)
                    {
                        UWORD *cols = NULL;
                        UWORD baseindex;
                        
                        if (idx < 4) {
                            baseindex = 0;
                            cols = &ActivePrefs->color0;
                        } else if (idx >= 8 && idx <= 10) {
                            baseindex = 8;
                            cols=&ActivePrefs->color17;
                            update_pointer = TRUE;
                        }
                        
                        if (cols)
                            cols[idx - baseindex] = ((red >> 4) & 0xf00) | ((green >> 8) & 0x0f0) | ((blue >> 12));
                    }
                    DEBUG_SETIPREFS(bug("[Intuition] %s: Set Color32 %ld R 0x%08lx G 0x%08lx B 0x%08lx\n",
                                __func__,
                                (LONG) idx,
                                p[idx].red,
                                p[idx].green,
                                p[idx].blue));
                }
                pp++;
            }
            
            if (update_pointer) {
                DEBUG_SETIPREFS(bug("[SetIPrefs] Updating pointer colors\n", __func__));
                SetPointerColors(IntuitionBase);
            }
        }
        break;

        case IPREFS_TYPE_PENS_V39:
        DEBUG_SETIPREFS(bug("[Intuition] %s: IP_PENS_V39\n", __func__));
        {
            struct IOldPenPrefs *fp = data;
            UWORD *dataptr;
            int i;
            DEBUG_SETIPREFS(bug("[Intuition] %s: Count %ld Type %ld\n",
                        __func__,
                        (LONG) fp->Count,
                        (LONG) fp->Type));

            if (fp->Type==0)
            {
                dataptr = &GetPrivIBase(IntuitionBase)->DriPens4[0];
                DEBUG_SETIPREFS(bug("[Intuition] %s: Pens4[]\n", __func__));
            }
            else
            {
                dataptr = &GetPrivIBase(IntuitionBase)->DriPens8[0];
                DEBUG_SETIPREFS(bug("[Intuition] %s: Pens8[]\n", __func__));
            }
            for (i=0;i<NUMDRIPENS;i++)
            {
                if (fp->PenTable[i]==(UWORD)~0UL)
                {
                    /*
                     * end of the array
                     */
                    DEBUG_SETIPREFS(bug("[Intuition] %s: PenTable end at entry %ld\n", __func__, (LONG) i));
                    break;
                }
                else
                {
                    DEBUG_SETIPREFS(bug("[Intuition] %s: Pens[%ld] %ld\n",
                                __func__,
                                (LONG) i,
                                (LONG) fp->PenTable[i]));
                    dataptr[i] = fp->PenTable[i];
                }
            }
        }
        break;


        case IPREFS_TYPE_POINTER_ALPHA:
            DEBUG_SETIPREFS(bug("[Intuition] %s: IP_POINTER_ALPHA\n", __func__));
            GetPrivIBase(IntuitionBase)->PointerAlpha = *(UWORD *)data;
        break;

        case IPREFS_TYPE_OVERSCAN_V37:
            DEBUG_SETIPREFS(bug("[Intuition] %s: IP_OVERSCAN_V37\n", __func__));
        break;

        case IPREFS_TYPE_FONT_V37:
        DEBUG_SETIPREFS(bug("[Intuition] %s: IP_FONT_V37\n", __func__));
        {
            struct IFontPrefs *fp = data;
            struct TextFont *font = OpenFont(&fp->fp_TextAttr);
            struct TextFont **fontptr;

            DEBUG_SETIPREFS(bug("[Intuition] %s: Type %d Name <%s> Size %d Font %p\n", __func__, fp->fp_ScrFont, fp->fp_Name, fp->fp_TextAttr.ta_YSize, font));

            if (font)
            {
                if (fp->fp_ScrFont==0)
                {
                    /*
                     * We can't free graphics defaultfont..it`s shared
                     */
                    fontptr = &GfxBase->DefaultFont;
                }
                else
                {
                    fontptr = &GetPrivIBase(IntuitionBase)->ScreenFont;
                    CloseFont(*fontptr);
                }
                *fontptr = font;
            }
        }
        break;

        default:
            DEBUG_SETIPREFS(bug("[Intuition] %s: Unknown Prefs Type\n", __func__));
            Result = FALSE;
            break;
    }

    UnlockIBase(lock);

    DEBUG_SETIPREFS(bug("[Intuition] %s: Result 0x%lx\n", __func__, Result));
    
    return(Result);
    
    AROS_LIBFUNC_EXIT
} /* private1 */
