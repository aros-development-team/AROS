#ifndef INTUITION_SCREENS_H
#define INTUITION_SCREENS_H

/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Screen handling structures
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef GRAPHICS_CLIP_H
#   include <graphics/clip.h>
#endif
#ifndef GRAPHICS_GFX_H
#   include <graphics/gfx.h>
#endif
#ifndef GRAPHICS_LAYERS_H
#   include <graphics/layers.h>
#endif
#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif
#ifndef GRAPHICS_VIEW_H
#   include <graphics/view.h>
#endif
#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

struct Screen
{
    struct Screen * NextScreen;
    struct Window * FirstWindow;

    WORD LeftEdge;
    WORD TopEdge;
    WORD Width;
    WORD Height;

    WORD MouseY;
    WORD MouseX;

    UWORD   Flags;
    UBYTE * Title;
    UBYTE * DefaultTitle;

    BYTE BarHeight;
    BYTE BarVBorder;
    BYTE BarHBorder;
    BYTE MenuVBorder;
    BYTE MenuHBorder;
    BYTE WBorTop;
    BYTE WBorLeft;
    BYTE WBorRight;
    BYTE WBorBottom;

    struct TextAttr * Font;

    struct ViewPort   ViewPort;
    struct RastPort   RastPort;
    struct BitMap     BitMap_OBSOLETE;    /* OBSOLETE */
    struct Layer_Info LayerInfo;

    struct Gadget * FirstGadget;

    UBYTE DetailPen;
    UBYTE BlockPen;

    UWORD          SaveColor0;
    struct Layer * BarLayer;
    UBYTE        * ExtData;
    UBYTE        * UserData;
};

struct NewScreen
{
    WORD LeftEdge;
    WORD TopEdge;
    WORD Width;
    WORD Height;

    WORD  Depth;
    UBYTE DetailPen;
    UBYTE BlockPen;

    UWORD ViewModes;
    UWORD Type;      /* see below */

    struct TextAttr * Font;
    UBYTE           * DefaultTitle;
    struct Gadget   * Gadgets;
    struct BitMap   * CustomBitMap;
};

struct ExtNewScreen
{
    WORD LeftEdge;
    WORD TopEdge;
    WORD Width;
    WORD Height;

    WORD  Depth;
    UBYTE DetailPen;
    UBYTE BlockPen;

    UWORD ViewModes;
    UWORD Type;      /* see below */

    struct TextAttr * Font;
    UBYTE           * DefaultTitle;
    struct Gadget   * Gadgets;
    struct BitMap   * CustomBitMap;

/* ExtNewScreen specific extension */
    struct TagItem * Extension; /* see below */
};

/* Screen->Flags and (Ext)NewScreen->Type*/
#define WBENCHSCREEN (1<<0)
#define PUBLICSCREEN (1<<1)
#define CUSTOMSCREEN 0x000F
#define SCREENTYPE   0x000F
/* Screen->Flags */
#define SHOWTITLE    (1<<4)
#define BEEPING      (1<<5)
#define CUSTOMBITMAP (1<<6)
#define SCREENBEHIND (1<<7)
#define SCREENQUIET  (1<<8)
#define SCREENHIRES  (1<<9)
#define PENSHARED    (1<<10)
#define NS_EXTENDED  (1<<12)
#define AUTOSCROLL   (1<<14)

/* Height */
#define STDSCREENHEIGHT -1
/* Width */
#define STDSCREENWIDTH  -1

/* ExtNewScreen->Extension (Tags) */
#define SA_Dummy              (TAG_USER + 32 )
#define SA_Left               (SA_Dummy + 1  ) /* [ISG] LONG            Left edge position */
#define SA_Top                (SA_Dummy + 2  ) /* [ISG] LONG            Top edge position  */
#define SA_Width              (SA_Dummy + 3  ) /* [I.G] LONG            Screen width       */
#define SA_Height             (SA_Dummy + 4  ) /* [I.G] LONG            Screen height      */
#define SA_Depth              (SA_Dummy + 5  ) /* [I.G] LONG            Screen depth       */
#define SA_DetailPen          (SA_Dummy + 6  )
#define SA_BlockPen           (SA_Dummy + 7  )
#define SA_Title              (SA_Dummy + 8  )
#define SA_Colors             (SA_Dummy + 9  )
#define SA_ErrorCode          (SA_Dummy + 10 )
#define SA_Font               (SA_Dummy + 11 )
#define SA_SysFont            (SA_Dummy + 12 )
#define SA_Type               (SA_Dummy + 13 )
#define SA_BitMap             (SA_Dummy + 14 )
#define SA_PubName            (SA_Dummy + 15 ) /* [I.G] STRPTR          Public screen name */
#define SA_PubSig             (SA_Dummy + 16 )
#define SA_PubTask            (SA_Dummy + 17 )
#define SA_DisplayID          (SA_Dummy + 18 ) /* [I.G] ULONG           Display mode ID    */
#define SA_DClip              (SA_Dummy + 19 )
#define SA_Overscan           (SA_Dummy + 20 ) /* see below */

#define SA_ShowTitle          (SA_Dummy + 22 )
#define SA_Behind             (SA_Dummy + 23 ) /* [I.G] BOOL                               */
#define SA_Quiet              (SA_Dummy + 24 )
#define SA_AutoScroll         (SA_Dummy + 25 )
#define SA_Pens               (SA_Dummy + 26 )
#define SA_FullPalette        (SA_Dummy + 27 )
#define SA_ColorMapEntries    (SA_Dummy + 28 )
#define SA_Parent             (SA_Dummy + 29 )
#define SA_Draggable          (SA_Dummy + 30 )
#define SA_Exclusive          (SA_Dummy + 31 )
#define SA_SharePens          (SA_Dummy + 32 )
#define SA_BackFill           (SA_Dummy + 33 )
#define SA_Interleaved        (SA_Dummy + 34 )
#define SA_Colors32           (SA_Dummy + 35 )
#define SA_VideoControl       (SA_Dummy + 36 )
#define SA_FrontChild         (SA_Dummy + 37 )
#define SA_BackChild          (SA_Dummy + 38 )
#define SA_LikeWorkbench      (SA_Dummy + 39 )
#define SA_MinimizeISG        (SA_Dummy + 41 )

#define SA_Displayed          (SA_Dummy + 101) /* [..G] BOOL            The screen is currently on display                 */
#define SA_MonitorName        (SA_Dummy + 102) /* [I.G] STRPTR          Monitor object name                                */
#define SA_MonitorObject      (SA_Dummy + 103) /* [..G] Object *        Monitorclass object                                */
#define SA_TopLeftScreen      (SA_Dummy + 112) /* [..G] struct Screen * Pointer to the screen displayed on another monitor */
#define SA_TopMiddleScreen    (SA_Dummy + 113)
#define SA_TopRightScreen     (SA_Dummy + 114)
#define SA_MiddleLeftScreen   (SA_Dummy + 115)
#define SA_MiddleRightScreen  (SA_Dummy + 117)
#define SA_BottomLeftScreen   (SA_Dummy + 118)
#define SA_BottomMiddleScreen (SA_Dummy + 119)
#define SA_BottomRightScreen  (SA_Dummy + 120)
#define SA_StopBlanker        (SA_Dummy + 121) /* [.S.] BOOL            Reserved                                            */
#define SA_ShowPointer        (SA_Dummy + 122) /* [ISG] BOOL            Whether to show mouse pointer on custom screen      */
#define SA_GammaControl       (SA_Dummy + 123) /* [I..] BOOL            Enable custom gamma table for the screen            */
#define SA_GammaRed           (SA_Dummy + 124) /* [ISG] UBYTE *         Gamma table for red color                           */
#define SA_GammaBlue          (SA_Dummy + 125) /* [ISG] UBYTE *         Gamma table for blue color                          */
#define SA_GammaGreen         (SA_Dummy + 126) /* [ISG] UBYTE *         Gamma table for green color                         */
#define SA_3DSupport          (SA_Dummy + 127) /* [I..] BOOL            Make sure that screen has 3D support                */
#define SA_AdaptSize          (SA_Dummy + 128) /* [I..] BOOL            Adapt screen size to display size                   */
#define SA_DisplayWidth       (SA_Dummy + 129) /* [I.G] ULONG           Width of the display mode to search for             */ 
#define SA_DisplayHeight      (SA_Dummy + 130) /* [I.G] ULONG           Height of the display mode to search for            */ 
#define SA_OpacitySupport     (SA_Dummy + 131) /* [..G]                 Reserved                                            */
#define SA_SourceAlphaSupport (SA_Dummy + 132) /* [..G]                 Reserved                                            */
#define SA_PixelFormat        (SA_Dummy + 133) /* [..G] ULONG           Pixel format of the screen                          */
#define SA_ScreenbarTextYPos  (SA_Dummy + 134) /* [..G] LONG            Y position (baseline) of the screen bar text        */
#define SA_ScreenbarTextPen   (SA_Dummy + 135) /* [..G] ULONG           Pen color for the screenbar text                    */
#define SA_ScreenbarTextFont  (SA_Dummy + 136) /* [..G] struct TextFont * Font for the screenbar text                       */
#define SA_ScreenbarSignal    (SA_Dummy + 137) /* [..G] ULONG           Reserved (MorphOS: signal bit for screenbar pluins) */
#define SA_ExactMatchMonitorName (SA_Dummy + 138) /* [I..] BOOL         Strictly obey SA_MonitorName                        */
#define SA_CompositingLayers  (SA_Dummy + 139) /* [I.G] BOOL            Reserved (MorphOS: use composition for layers)      */

/* SA_Overscan */
#define OSCAN_TEXT     1
#define OSCAN_STANDARD 2
#define OSCAN_MAX      3
#define OSCAN_VIDEO    4

                            /* Public Screens */

struct PubScreenNode
{
    struct Node psn_Node;

    struct Screen * psn_Screen;
    UWORD           psn_Flags;        /* see below */
    WORD            psn_Size;
    WORD            psn_VisitorCount;
    struct Task   * psn_SigTask;
    UBYTE           psn_SigBit;
};

/* psn_Flags */
#define PSNF_PRIVATE (1<<0)

/* Maximum length of public screen names. The buffers containing these strings
   must have a length of MAXPUBSCREENNAME+1. */   
#define MAXPUBSCREENNAME 139

#define SHANGHAI     (1<<0)
#define POPPUBSCREEN (1<<1)

                         /* Screen functions */

/* AllocScreenBuffer() */
#define SB_SCREEN_BITMAP 1
#define SB_COPY_BITMAP   2

struct ScreenBuffer
{
    struct BitMap   * sb_BitMap;
    struct DBufInfo * sb_DBufInfo;
};

/* ScreenDepth() */
#define SDEPTH_TOFRONT  0
#define SDEPTH_TOBACK   1
#define SDEPTH_INFAMILY 2

/* ScreenPosition() */
#define SPOS_RELATIVE    0x00
#define SPOS_ABSOLUTE    (1<<0)
#define SPOS_MAKEVISIBLE (1<<1)
#define SPOS_FORCEDRAG   (1<<2)

                            /* Draw Info */

struct DrawInfo
{
    UWORD             dri_Version; /* see below */
    UWORD             dri_NumPens;
    UWORD           * dri_Pens;    /* see below */
    struct TextFont * dri_Font;
    UWORD             dri_Depth;

    struct
    {
        UWORD X;
        UWORD Y;
    } dri_Resolution;

    ULONG dri_Flags; /* see below */

    /* The following fields are present if dri_Version >= 2 */
    struct Image * dri_CheckMark;
    struct Image * dri_AmigaKey;  

    /*
     * The following fields are compatible with AmigaOS v4.
     * Present if dri_Version >= 3.
     */
    struct Screen *dri_Screen;
    APTR           dri_Prefs;

    IPTR           dri_Reserved[3];
};

/* dri_Version */
#define DRI_VERSION 3

/* dri_Flags */
#define DRIF_NEWLOOK (1L<<0)

/* dri_Pens */
#define DETAILPEN        0
#define BLOCKPEN         1
#define TEXTPEN          2
#define SHINEPEN         3
#define SHADOWPEN        4
#define FILLPEN          5
#define FILLTEXTPEN      6
#define BACKGROUNDPEN    7
#define HIGHLIGHTTEXTPEN 8
#define BARDETAILPEN     9
#define BARBLOCKPEN      10
#define BARTRIMPEN       11
#define NUMDRIPENS       12

#define PEN_C0 0xFEFF
#define PEN_C1 0xFEFE
#define PEN_C2 0xFEFD
#define PEN_C3 0xFEFC

/* values for ChangeDecoration ID param */

#define DECORATION_SET     0x8001
#define DECORATION_DEFAULT 0x8000

/* Errors */
#define OSERR_NOMONITOR    1
#define OSERR_NOCHIPS      2
#define OSERR_NOMEM        3
#define OSERR_NOCHIPMEM    4
#define OSERR_PUBNOTUNIQUE 5
#define OSERR_UNKNOWNMODE  6
#define OSERR_TOODEEP      7
#define OSERR_ATTACHFAIL   8
#define OSERR_NOTAVAILABLE 9

#endif /* GRAPHICS_SCREENS_H */
