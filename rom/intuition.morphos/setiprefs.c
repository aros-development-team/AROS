/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/graphics.h>
#include <prefs/palette.h>
#include <intuition/pointerclass.h>
#include <devices/input.h>
#include <exec/io.h>
#include "intuition_intern.h"
#include "menus.h"
#include "inputhandler_support.h"
#ifdef __MORPHOS__
#include <cybergraphx/cybergraphics.h>
#include <ppcinline/cybergraphics.h>
#endif
#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

#ifdef IP_ICONTROL
#undef IP_ICONTROL
#endif
#define IP_ICONTROL 4

#define MODENOTAVAILABLE
#if 0
struct InputPrefs
{
    char           ip_Keymap[16];
    UWORD          ip_PointerTicks;
    struct timeval ip_DoubleClick;
    struct timeval ip_KeyRptDelay;
    struct timeval ip_KeyRptSpeed;
    WORD           ip_MouseAccel;
};
#endif
struct IOldFontPrefs
{
    struct TextAttr fp_TextAttr;
    UBYTE           fp_Name[32];
    ULONG           fp_NotUsed;
    WORD            fp_Type;
};

struct IFontPrefs
{
    struct TextAttr fp_TextAttr;
    UBYTE           fp_Name[32];
    ULONG           fp_xxx;
    BOOL            fp_ScrFont;
};

struct IPointerColorPrefs
{
    UWORD Num;
    UWORD Red;
    UWORD Green;
    UWORD Blue;
    UWORD NotZero;
    UWORD Pad[3];
};

struct IOldPointerPrefs
{
    struct BitMap *BitMap;
    WORD  XOffset;
    WORD  YOffset;
    UWORD WordWidth;
    UWORD XResolution;
    UWORD YResolution;
    UWORD Type;
};

struct IPointerPrefs
{
    struct BitMap *BitMap;
    WORD  XOffset;
    WORD  YOffset;
    UWORD BytesPerRow;
    UWORD Size;
    UWORD YSize;
    UWORD Which;
    ULONG Zero;
};

struct IOldPenPrefs
{
    UWORD Count;
    UWORD Type;
    ULONG Pad;
    UWORD PenTable[NUMDRIPENS+1];
};

struct IOldOverScanPrefs
{
    ULONG DisplayID;
    Point ViewPos;
    Point Text;
    struct Rectangle Standard;
};

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

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
    Depending on the operation

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
    29-10-95    digulla automatically created from
                intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    ULONG Result=TRUE;
    ULONG lock = LockIBase(0);

    DEBUG_SETIPREFS(bug("SetIPrefs: data %p length %lu type %lu\n", data, length, type));

    switch (type)
    {
    case IP_SCREENMODE:
#ifndef USEGETIPREFS
        {
#ifdef MODENOTAVAILABLE
            ULONG modeid;
            long  width,height;
            ULONG RecalcID = FALSE;
#endif

            DEBUG_SETIPREFS(bug("SetIPrefs: IP_SCREENMODE\n"));
            if (length > sizeof(struct IScreenModePrefs))
                length = sizeof(struct IScreenModePrefs);
            CopyMem(data, &GetPrivIBase(IntuitionBase)->ScreenModePrefs, length);

#ifdef MODENOTAVAILABLE
            modeid = GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_DisplayID;
            width  = (WORD)GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Width;
            height = (WORD)GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Height;

            if (ModeNotAvailable(modeid))
            {
                RecalcID = TRUE;
            }


            if ((RecalcID == TRUE) && (CyberGfxBase = OpenLibrary("cybergraphics.library",40)))
            {
                //ULONG ID;

                // mode specified is not available

                GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_Depth = 8;

                DEBUG_SETIPREFS(bug("WB ModeID %08lx not available (w %ld h %ld)\n",modeid,width,height));

          // try to find a 800x600x16bit mode first  
                modeid=BestCModeIDTags( CYBRBIDTG_NominalWidth,  (width == -1) ? 800 : width,
                            CYBRBIDTG_NominalHeight, (height == -1) ? 600 : height,
                            CYBRBIDTG_Depth,         16,
                            TAG_DONE );

        if(modeid == INVALID_ID)
        {
          // find a 640x480x8bit fallback mode if there is no 16bit one

                modeid=BestCModeIDTags( CYBRBIDTG_NominalWidth,  (width == -1) ? 640 : width,
                            CYBRBIDTG_NominalHeight, (height == -1) ? 480 : height,
                            CYBRBIDTG_Depth,         8,
                            TAG_DONE );
        }


                if (modeid != INVALID_ID)
                {
                    DEBUG_SETIPREFS(bug(" Using replacement ID %08lx\n",modeid));

                    GetPrivIBase(IntuitionBase)->ScreenModePrefs.smp_DisplayID = modeid;
                }

                CloseLibrary(CyberGfxBase);
            }
#endif
        }
#endif
        break;

    case IP_OLDICONTROL:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_OLDICONTROL\n"));
        if (length > sizeof(struct IIControlPrefs))
            length = sizeof(struct IIControlPrefs);
        CopyMem(data, &GetPrivIBase(IntuitionBase)->IControlPrefs, length);
        FireMenuMessage(MMCODE_STARTCLOCK,NULL,NULL,IntuitionBase);//test if we need to restart clock
        break;

    case IP_IEXTENSIONS:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_IEXTENSIONS\n"));
#ifdef SKINS
        if (length > sizeof(struct IControlExtensions))
            length = sizeof(struct IControlExtensions);
        CopyMem(data, &GetPrivIBase(IntuitionBase)->IControlExtensions, length);
#endif
        break;

    case IP_INPUTEXT:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_INPUTEXT\n"));
#ifdef SKINS
        if (length > sizeof(struct InputPrefsExt))
            length = sizeof(struct InputPrefsExt);

        CopyMem(data, &GetPrivIBase(IntuitionBase)->InputPrefsExt, length);

        {
            struct IOStdReq req;

            memclr(&req,sizeof (struct IOStdReq));

            req.io_Device = GetPrivIBase(IntuitionBase)->InputIO->io_Device;
            req.io_Unit = GetPrivIBase(IntuitionBase)->InputIO->io_Unit;
            req.io_Command = IND_SETMOUSETYPE;
            req.io_Data = &GetPrivIBase(IntuitionBase)->InputPrefsExt.ip_MouseMode;
            DoIO(&req);

        }
#endif
        break;


    case IP_IACTIONS:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_IACTIONS\n"));
#ifdef SKINS
        {
            ULONG mem;

            ObtainSemaphore(&GetPrivIBase(IntuitionBase)->InputHandlerLock);

            mem = GetPrivIBase(IntuitionBase)->NumIControlActions * (sizeof (struct IAction));

            GetPrivIBase(IntuitionBase)->NumIControlActions = 0;

            FreeMem(GetPrivIBase(IntuitionBase)->IControlActions,mem);

            mem = (length/(sizeof (struct IAction)) * (sizeof (struct IAction)));

            if ((GetPrivIBase(IntuitionBase)->IControlActions = AllocMem(mem,MEMF_ANY)))
            {

                GetPrivIBase(IntuitionBase)->NumIControlActions = mem / (sizeof (struct IAction));
                CopyMem(data, GetPrivIBase(IntuitionBase)->IControlActions, mem);
            }

            ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->InputHandlerLock);
        }
#endif
        break;

    case IP_OLDFONT:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_OLDFONT\n"));
        {
            struct IOldFontPrefs *fp = data;
            struct TextFont *font = OpenFont(&fp->fp_TextAttr);

            DEBUG_SETIPREFS(bug("SetIPrefs: Type %d Name <%s> Size %d Font %p\n", fp->fp_Type, fp->fp_Name, fp->fp_TextAttr.ta_YSize, font));

            if (font)
            {
                struct TextFont **fontptr;
                if (fp->fp_Type==0)
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
    case IP_FONT:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_FONT\n"));
        {
            struct IFontPrefs *fp = data;
            struct TextFont *font = OpenFont(&fp->fp_TextAttr);
            struct TextFont **fontptr;

            DEBUG_SETIPREFS(bug("SetIPrefs: Type %d Name <%s> Size %d Font %p\n", fp->fp_ScrFont, fp->fp_Name, fp->fp_TextAttr.ta_YSize, font));

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

    case IP_OLDPOINTER:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_OLDPOINTER\n"));
        {
            struct IOldPointerPrefs *fp = data;
            Object *pointer;
            Object **oldptr;

            DEBUG_SETIPREFS(bug("SetIPrefs: Bitmap 0x%lx XOffset %ld YOffset %ld XResolution %ld YResolution %ld WordWidth %ld Type %ld\n",
                        (ULONG) fp->BitMap,
                        (LONG) fp->XOffset,
                        (LONG) fp->YOffset,
                        (LONG) fp->XResolution,
                        (LONG) fp->YResolution,
                        (LONG) fp->WordWidth,
                        (LONG) fp->Type));

            {
                struct TagItem pointertags[] =
                    {
                        {
                            POINTERA_BitMap , (ULONG) fp->BitMap
                        },
                        {POINTERA_XOffset       , fp->XOffset       },
                        {POINTERA_YOffset       , fp->YOffset       },
                        {POINTERA_XResolution   , fp->XResolution   },
                        {POINTERA_YResolution   , fp->YResolution   },
                        {POINTERA_WordWidth     , fp->WordWidth     },
                        {TAG_DONE                       }
                    };

                pointer = NewObjectA(
                          GetPrivIBase(IntuitionBase)->pointerclass,
                          NULL,
                          pointertags);

            }

            oldptr = fp->Type ?
                 &GetPrivIBase(IntuitionBase)->BusyPointer :
                 &GetPrivIBase(IntuitionBase)->DefaultPointer;

            InstallPointer(IntuitionBase, oldptr, pointer);
            /*
             * Original iprefs checks for a 0 or -1 return
             * otherwise it expects a returned bitmap..sigh
             */
            Result = -1;
        }
        break;
    case IP_POINTER:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_POINTER\n"));
        {
            struct IPointerPrefs *fp = data;
            struct TagItem pointertags[] =
                {
                    {
                        POINTERA_BitMap    , (ULONG) fp->BitMap
                    },
                    {POINTERA_XOffset   , fp->XOffset   },
                    {POINTERA_YOffset   , fp->YOffset   },
                    {TAG_DONE               }
                };

            Object *pointer = NewObjectA(
                          GetPrivIBase(IntuitionBase)->pointerclass,
                          NULL,
                          pointertags);

            Object **oldptr = fp->Which ?
                      &GetPrivIBase(IntuitionBase)->BusyPointer :
                      &GetPrivIBase(IntuitionBase)->DefaultPointer;

            InstallPointer(IntuitionBase, oldptr, pointer);
        }
        break;

    case IP_OLDPENS:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_OLDPENS\n"));
        {
            struct IOldPenPrefs *fp = data;
            UWORD *dataptr;
            int i;
            DEBUG_SETIPREFS(bug("SetIPrefs: Count %ld Type %ld\n",
                        (LONG) fp->Count,
                        (LONG) fp->Type));

            if (fp->Type==0)
            {
                dataptr = &GetPrivIBase(IntuitionBase)->DriPens4[0];
                DEBUG_SETIPREFS(bug("SetIPrefs: Pens4[]\n"));
            }
            else
            {
                dataptr = &GetPrivIBase(IntuitionBase)->DriPens8[0];
                DEBUG_SETIPREFS(bug("SetIPrefs: Pens8[]\n"));
            }
            for (i=0;i<NUMDRIPENS;i++)
            {
                if (fp->PenTable[i]==(UWORD)~0UL)
                {
                    /*
                     * end of the array
                     */
                    DEBUG_SETIPREFS(bug("SetIPrefs: PenTable end at entry %ld\n", (LONG) i));
                    break;
                }
                else
                {
                    DEBUG_SETIPREFS(bug("SetIPrefs: Pens[%ld] %ld\n",
                                (LONG) i,
                                (LONG) fp->PenTable[i]));
                    dataptr[i] = fp->PenTable[i];
                }
            }
        }
        break;

    case IP_OLDOVERSCAN:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_OLDOVERSCAN\n"));
        {
            struct IOldOverScanPrefs *fp = data;
            DEBUG_SETIPREFS(bug("SetIPrefs: DisplayID 0x%lx\n",
                        fp->DisplayID));

            DEBUG_SETIPREFS(bug("SetIPrefs: ViewPos.x %ld ViewPos.y %ld\n",
                        (LONG) fp->ViewPos.x,
                        (LONG) fp->ViewPos.y));

            DEBUG_SETIPREFS(bug("SetIPrefs: Text.x %ld Text.y %ld\n",
                        (LONG) fp->Text.x,
                        (LONG) fp->Text.y));

            DEBUG_SETIPREFS(bug("SetIPrefs: MinX %ld MinY %ld MaxX %ld MaxY %ld\n",
                        (LONG) fp->Standard.MinX,
                        (LONG) fp->Standard.MinY,
                        (LONG) fp->Standard.MaxX,
                        (LONG) fp->Standard.MaxY));
        }
        break;

    case IP_PTRCOLOR:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_PTRCOLOR\n"));
        if (IntuitionBase->ActiveScreen)
        {
            struct IPointerColorPrefs *fp = data;
            if (fp->Num >= 8 && fp->Num <= 10)
            {
                (&GetPrivIBase(IntuitionBase)->ActivePreferences->color17)[fp->Num - 8] =
                    ((fp->Red << 4) & 0xf00) | (fp->Green & 0x0f0) | (fp->Blue >> 4);
                SetPointerColors(IntuitionBase);
            }
        }
        break;

    case IP_OLDPALETTE:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_OLDPALETTE\n"));
        {
            struct ColorSpec *pp = data;
            struct Color32 *p = GetPrivIBase(IntuitionBase)->Colors;

            // COLORTABLEENTRIES == 32

            DEBUG_SETIPREFS(bug("SetIPrefs: Intuition Color32 Table 0x%lx\n", (ULONG) p));

            while (pp->ColorIndex != ~0x0)
            {
                DEBUG_SETIPREFS(bug("SetIPrefs: Index %ld Red 0x%lx Green 0x%lx Blue 0x%lx\n",
                            (LONG) pp->ColorIndex,
                            (ULONG) pp->Red,
                            (ULONG) pp->Green,
                            (ULONG) pp->Blue));
                if (pp->ColorIndex < COLORTABLEENTRIES)
                {
                    struct Preferences *ActivePrefs;

                    p[pp->ColorIndex].red   = (pp->Red<<16)|pp->Red;
                    p[pp->ColorIndex].green = (pp->Green<<16)|pp->Green;
                    p[pp->ColorIndex].blue  = (pp->Blue<<16)|pp->Blue;

                    // check for pointer colors
                    if ((ActivePrefs = GetPrivIBase(IntuitionBase)->ActivePreferences))
                    {
                        if (pp->ColorIndex >= 8 && pp->ColorIndex <= 10)
                        {
                            UWORD *ptrcols=&ActivePrefs->color17;

                            ptrcols[pp->ColorIndex - 8] =
                                ((pp->Red >> 4) & 0xf00) | ((pp->Green >> 8) & 0x0f0) | (pp->Blue >> 12);


                            if (IntuitionBase->ActiveScreen)
                            {
                                SetPointerColors(IntuitionBase);
                            }
                        }
                    }
                    DEBUG_SETIPREFS(bug("SetIPrefs: Set Color32 %ld Red 0x%lx Green 0x%lx Blue 0x%lx\n",
                                (LONG) pp->ColorIndex,
                                p[pp->ColorIndex].red,
                                p[pp->ColorIndex].green,
                                p[pp->ColorIndex].blue));
                }
                else
                {
                    DEBUG_SETIPREFS(bug("SetIPrefs: ColorIndex %ld > TableSize %ld\n",
                                        (LONG) pp->ColorIndex, (LONG) COLORTABLEENTRIES));
                }
                pp++;
            }
        }
        break;

    case IP_PALETTE:
        DEBUG_SETIPREFS(bug("SetIPrefs: IP_PALETTE\n"));
        {
            struct PalettePrefs *pp = data;
            int k;
            struct Color32 *p = GetPrivIBase(IntuitionBase)->Colors;

            CopyMem(pp->pap_4ColorPens, GetPrivIBase(IntuitionBase)->DriPens4, NUMDRIPENS * sizeof(UWORD));
            CopyMem(pp->pap_8ColorPens, GetPrivIBase(IntuitionBase)->DriPens8, NUMDRIPENS * sizeof(UWORD));
            for (k = 0; k < COLORTABLEENTRIES; ++k)
            {
                int n = pp->pap_Colors[k].ColorIndex;
                if (n == -1)
                    break;
                else if (n >= 0 && n < 8)
                {
                    p[n].red   = pp->pap_Colors[k].Red * 0x10001;
                    p[n].green = pp->pap_Colors[k].Green * 0x10001;
                    p[n].blue  = pp->pap_Colors[k].Blue * 0x10001;
                }
                DEBUG_SETIPREFS(bug("SetIPrefs: Set Color32 %ld Red 0x%lx Green 0x%lx Blue 0x%lx\n",
                            (LONG) n,
                            p[n].red,
                            p[n].green,
                            p[n].blue));
            }
        }
        break;

    default:
        DEBUG_SETIPREFS(bug("SetIPrefs: Unknown Prefs Type\n"));
        Result = FALSE;
        break;
    }

    UnlockIBase(lock);

    DEBUG_SETIPREFS(bug("SetIPrefs: Result 0x%lx\n",Result));
    return(Result);
    AROS_LIBFUNC_EXIT
} /* private1 */
