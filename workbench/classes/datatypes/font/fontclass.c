/*
 * fontclass.c - Main functions for the Font DataType class
 * Copyright © 1995-96 Michael Letowski
 */

//#define DEBUG 0
#include <aros/debug.h>

#include <graphics/displayinfo.h>
#include <graphics/gfx.h>
#include <graphics/modeid.h>
#include <graphics/rastport.h>
#include <graphics/rpattr.h>
#include <graphics/text.h>
#include <cybergraphx/cybergraphics.h>
#include <intuition/classes.h>
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <diskfont/diskfont.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>

#include <stdlib.h>
#include <string.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/diskfont.h>
#include <proto/datatypes.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "classbase.h"
#include "prefs.h"
#include "otag.h"

/* Open superclass */
ADD2LIBS("datatypes/picture.datatype", 0, struct Library *, PictureBase);

/****** font.datatype/font.datatype *****************************************
*
*   NAME
*       font.datatype -- data type for Amiga bitmap and outline fonts.
*
*   FUNCTION
*       The font data type, a sub-class of the picture.datatype, is used
*       to load (and display) Amiga bitmap and outline fonts. All available
*       sizes of a given font are loaded into memory and then rendered into
*       screen, taking user preferences into account.
*       By default this data type will render each font size in one line,
*       in ascending order. Whole characters set for the given font and size
*       will be rendered.
*
*       User preferences are stored in font.prefs file, located in either
*       PROGDIR:Prefs/DataTypes/ or ENV:DataTypes/ directory (searched in
*       that order). A preferences file is an ASCII file parsed with the
*       following ReadArgs() template:
*
*           STRINGS/M,CENTER=CENTRE/S,INV=INVERSE/S,FN=FONTNAME/S,
*           DPI/K,FG=FOREGROUND/K,BG=BACKGROUND/K
*
*       STRINGS are multiple texts that are to be rendered instead of the
*       default character set. Each string is rendered in one line.
*       CENTRE indicates that each line should be centered on screen.
*       INVERSE indicates foreground/background color inversion. Does not
*       work for color fonts.
*       FONTNAME indicates that for each font's size the name and size of
*       that font should be rendered before any user text.
*       DPI describes font's aspect ratio, in XDPI/A/N,YDPI/A/N format.
*       Default values for aspect ratio are DiskFont's defaults.
*       FOREGROUND describes foreground color for two-color fonts. The string
*       is parsed with R=RED/A/N,G=GREEN/A/N,B=BLUE/A/N template to extract
*       red, green and blue components for a given color. Default foreground
*       color is black. This option does not work for color fonts.
*       BACKGROUND describes background color for two-color fonts. The string
*       is parsed with R=RED/A/N,G=GREEN/A/N,B=BLUE/A/N template to extract
*       red, green and blue components for a given color. Default background
*       color is white. This option does not work for color fonts.
*
*   METHODS
*       OM_NEW -- Create a new picture object from a .font file and all
*                 associated size files. The source must be a file.
*
*   NOTES
*       Since version 39.4 a font to be loaded does not need to be in
*       FONTS: path. The data type will temporarily extend FONTS: assign
*       to cover given font.
*       If bitmaps are created for an outline font, the data type will load
*       only those sizes, for which bitmaps exist. Otherwise all DiskFont
*       default sizes will be loaded.
*       For color fonts the color is set to that of biggest size.
*
*   BUGS
*       Loading outline fonts with multiple size requires 'a lot' of memory.
*       OpenDiskFont() uses a lot of stack and this causes problems with
*       small-stack clients like original IPrefs.
*
*   SEE ALSO
*       picture.datatype, diskfont.library, bullet.library.
*
*****************************************************************************
*
*/

/*
 * Private constants
 */
#define IDS_CNT                 4                                               /* Number of ModeIDs */

#define COL_WHITE               0xFF                                            /* 'white' colour */
#define COL_BLACK               0x00                                            /* 'black' colour */

#define DEF_XDPI                72                                              /* Default X-axis DPI */
#define DEF_YDPI                80                                              /* Default Y-axis DPI */

#define FONTSPATH               "FONTS"                                         /* Assign name without ':' */

/* 256 for ASCII set, 2 for terminator */
#define MAX_CHAR                (256 + 2)
/* 32 chars for font name, 6 for size, 2 for terminator */
#define MAX_CHAR2               (32 + 6 + 2)
/* Total number of strings */
#define MAX_STRING              32

/* Get font's name */
#define FontName(f)             ((f)->tf_Message.mn_Node.ln_Name)

/* Convenient cast */
#define CFont(f)                ((struct ColorTextFont *)(f))

typedef int (SFUNC)(void const *, void const *);

/*
 * Private functions prototypes
 */
STATIC PBOOL GetFont(Class *cl, Object *o, struct TagItem *attrs);

/* Font management */
STATIC struct TextFont **OpenFonts(struct Opts *opts,
        struct FontContentsHeader *fch, STRPTR name);
STATIC VOID CloseFonts(struct TextFont **f, ULONG cnt);

/* Rendering */
STATIC PBOOL GetWH(struct RastPort *rp, struct Opts *opt,
        struct TextFont *f, ULONG *w, ULONG *h);
STATIC PBOOL Render(struct RastPort *rp, struct Opts *opt,
        struct TextFont **f, ULONG cnt, ULONG w);
STATIC STRPTR *PrepStrings(struct Opts *opt, 
        struct TextFont *f, ULONG *cnt);
STATIC LONG SortFunc(struct TextFont **tf1, struct TextFont **tf2);

/*
 * Level 2 private functions
 */
STATIC PBOOL GetFont(Class *cl, Object *o, struct TagItem *attrs)
{
    STATIC CONST SIPTR SourceIDs[IDS_CNT] =                                         /* Font flag->mode selector */
    {
        LORES_KEY, HIRES_KEY, LORESLACE_KEY, HIRESLACE_KEY
    };	/* ModeIDs */

    struct RastPort RP;
    struct Opts Opts;

    struct PrefsHandle *PH;
    struct FileInfoBlock *FIB;
    struct FontContentsHeader *FCH;
    struct TextFont **Fonts = NULL, *F;
    struct BitMapHeader *BMHD = NULL;
    struct ColorFontColors *CFC;
    struct ColorRegister *CMap = NULL;
    LONG *CRegs = NULL;

    STRPTR Title, Name = NULL;                                                      /* Font file name & picture title */
    BPTR DirLock, FH = 0;
    ULONG TallWide, ModeID;
    ULONG Width, Height, Depth;
    ULONG I, J, NumFonts;
    UBYTE ForeCol[3], BackCol[3], TCol;
    PBOOL AFlag = FALSE, Result = FALSE;

    D(bug("[font.datatype] %s()\n", __func__));

    /* Read preferences */
    PH = GetFontPrefs(&Opts);

    /* Get default title */
    Title = (STRPTR)GetTagData(DTA_Name, 0, attrs);

    /* Get file handle and BitMapHeader */
    GetDTAttrs(o, DTA_Handle, &FH, PDTA_BitMapHeader, &BMHD, TAG_DONE);
    if ((FH) && (BMHD)) {
        /* Get font file name */
        if ((FIB = AllocDosObject(DOS_FIB,NULL))) {                                 /* Create FileInfoBlock */
            if (ExamineFH(FH, FIB))                                                 /* Examine it */
                Name = FIB->fib_FileName;                                           /* Get name pointer */
            if (!Name)                                                              /* Still no name */
                if (Title)                                                          /* Use title to get name */
                    Name = FilePart(Title);                                         /* Get file part of title */
            if (Name) {
                D(bug("[font.datatype] %s: Name = '%s'\n", __func__, Name));
                /* Examine .font file, load sizes */
                if ((DirLock = ParentOfFH(FH))) {
                    D(bug("[font.datatype] %s: DirLock = 0x%p\n", __func__, DirLock));
                    if ((FCH = NewFC(DirLock, Name))) {
                        AFlag = AssignAdd(FONTSPATH, DirLock);
                        Fonts = OpenFonts(&Opts, FCH, Name);
                        NumFonts = FCH->fch_NumEntries;
                        DisposeFC(FCH);
                    }
                    if (AFlag)
                        RemAssignList(FONTSPATH, DirLock);
                    else
                        UnLock(DirLock);
                }
                if (Fonts) {                                                        /* Check if any fonts loaded */
                    D(bug("[font.datatype] %s: %u Fonts @ 0x%p\n", __func__, NumFonts, Fonts));
                    /* Set colors */
                    for (I = 0; I < 3; I++) {                                       /* For each of RGB triad */
                        if (Opts.opt_ForeFlag)
                            ForeCol[I] = clamp(Opts.opt_ForeCol[I], 0, 255);
                        else
                            ForeCol[I] = COL_BLACK;
                        if (Opts.opt_BackFlag)
                            BackCol[I] = clamp(Opts.opt_BackCol[I], 0, 255);
                        else
                            BackCol[I] = COL_WHITE;
                    }

                    /* Calculate sizes */
                    TallWide = 0;
                    Width = Height = 0;
                    Depth = (Opts.opt_TrueColor) ? 24 : 1;
                    InitRastPort(&RP);                                              /* Set up RastPort - here! */
                    for (I = 0; I < NumFonts; I++)
                        if ((F = Fonts[I])) {                                       /* Opened successfully */
                            if (GetWH(&RP, &Opts, F, &Width, &Height)) {
                                if (ftst(F->tf_Style, FSF_COLORFONT))               /* This is ColorFont */
                                    if (CFont(F)->ctf_Depth > Depth)                /* Deeper? */
                                        Depth = CFont(F)->ctf_Depth;                /* Set new depth */
                                TallWide = (F->tf_Flags & (FPF_TALLDOT | FPF_WIDEDOT)) >> FPB_TALLDOT;
                            } else {
                                D(bug("[font.datatype] %s: failed to get width/height\n", __func__));
                                break;
                            }
                        }

                    /* Set up BitMap header */
                    BMHD->bmh_Width = Width;                                        /* Fill in informations */
                    BMHD->bmh_Height = Height;
                    BMHD->bmh_Depth = Depth;

                    D(bug("[font.datatype] %s: %ux%ux%u\n", __func__, BMHD->bmh_Width, BMHD->bmh_Height, BMHD->bmh_Depth));
                    if (Depth <= 8) {
                        /* Get display mode id */
                        ModeID = BestModeID(BIDTAG_DesiredWidth,    Width,
                                        BIDTAG_DesiredHeight,       Height,
                                        BIDTAG_Depth,               Depth,
                                        BIDTAG_SourceID,            SourceIDs[TallWide & (IDS_CNT-1)],
                                        TAG_DONE);

                        /* Set colors */
                        SetDTAttrs(o, NULL, NULL, PDTA_NumColors, 1 << Depth,TAG_DONE);
                        GetDTAttrs(o, PDTA_ColorRegisters, &CMap, PDTA_CRegs, &CRegs, TAG_DONE);
                        if (CMap && CRegs) {
                            if (Depth == 1) {                                           /* B&W font */
                                if (Opts.opt_Inverse)                                   /* Inversion? */
                                    for (I = 0; I < 3; I++)                             /* For each of RGB triad */
                                        tswap(ForeCol[I], BackCol[I], TCol);            /* Swap foreground/background */
                                for (I = 0; I < 3; I++) {                               /* For each of RGB triad */
                                    ((UBYTE *)&CMap[0])[I]      = BackCol[I];
                                    CRegs[I+0]                  = Color32(BackCol[I]);
                                    ((UBYTE *)&CMap[1])[I]      = ForeCol[I];
                                    CRegs[I+3]                  = Color32(ForeCol[I]);
                                }
                            }
                            else                                                        /* Some color fonts */
                                for (I = 0; I < NumFonts; I++)                          /* For each font */
                                    if ((F = Fonts[I]))                                 /* Is it valid? */
                                        if (ftst(F->tf_Style, FSF_COLORFONT)) {          /* Is it color? */
                                            D(bug("[font.datatype] %s: colour font\n", __func__));
                                            if ((CFC = CFont(F)->ctf_ColorFontColors)) {  /* Color table valid? */
                                                for (J = 0; J < CFC->cfc_Count; J++) {  /* Fore each color in table */
                                                    CMap[J].red =   (CFC->cfc_ColorTable[J] & 0x0F00) >> 4;
                                                    CRegs[J*3+0] =  Color32(CMap[J].red);
                                                    CMap[J].green = (CFC->cfc_ColorTable[J] & 0x00F0);
                                                    CRegs[J*3+1] =  Color32(CMap[J].green);
                                                    CMap[J].blue =  (CFC->cfc_ColorTable[J] & 0x000F) << 4;
                                                    CRegs[J*3+2] =  Color32(CMap[J].blue);
                                                }
                                            } else {
                                                D(bug("[font.datatype] %s: No colours specified\n", __func__));
                                                if (Depth == 8) {
                                                    for (J = 0; J < 256; J++) {
                                                        if (Opts.opt_Inverse) {
                                                            CMap[J].red   = J;
                                                            CMap[J].green = J;
                                                            CMap[J].blue  = J;
                                                        } else {
                                                            CMap[J].red   = 255 - J;
                                                            CMap[J].green = 255 - J;
                                                            CMap[J].blue  = 255 - J;
                                                        }
                                                        CRegs[J*3 + 0] = Color32(CMap[J].red);
                                                        CRegs[J*3 + 1] = Color32(CMap[J].green);
                                                        CRegs[J*3 + 2] = Color32(CMap[J].blue);
                                                    }
                                                }
                                            }
                                        }
                        }

                        /* Prepare bitmap */
                        RP.BitMap = AllocBitMap(Width, Height, Depth, BMF_CLEAR, NULL);
                        if (RP.BitMap) {
                            D(bug("[font.datatype] %s: bitmap allocated @ 0x%p\n", __func__, RP.BitMap));
                            /* Do rendering */
                            if ((Result = Render(&RP, &Opts, Fonts, NumFonts, Width))) {
                                if (Depth > 8) {
                                    IPTR bpr;
                                    APTR mem, bmhandle;
                                    struct TagItem bmTags[] = {
                                        {LBMI_BYTESPERROW, (IPTR)&bpr },
                                        {LBMI_BASEADDRESS, (IPTR)&mem },
                                        {TAG_DONE}
                                    };
                                    SetDTAttrs(o, NULL, NULL,
                                            DTA_ObjName,            Title,
                                            DTA_NominalHoriz,       Width,
                                            DTA_NominalVert,        Height,
                                            PDTA_SourceMode,        PMODE_V43,
                                            TAG_DONE);
                                    if ((bmhandle = LockBitMapTagList(RP.BitMap, bmTags)))
                                    {
                                        //DebugPrintF("bpr: %ld, w: %ld, h: %ld, d: %ld\n",bpr,Width,Height,Depth);
                                        DoSuperMethod(cl, o,
                                                PDTM_WRITEPIXELARRAY, mem, PBPAFMT_RGB,
                                                bpr, 0, 0, Width, Height);
                                        UnLockBitMap(bmhandle);
                                    } else {
                                        D(bug("[font.datatype] %s: failed to lock bitmap\n", __func__));
                                    }
                                } else {
                                    D(bug("[font.datatype] %s: using bitmap\n", __func__));
                                    SetDTAttrs(o, NULL, NULL,
                                            DTA_ObjName,            Title,
                                            DTA_NominalHoriz,       Width,
                                            DTA_NominalVert,        Height,
                                            PDTA_BitMap,            RP.BitMap,
                                            PDTA_ModeID,            ModeID,
                                            TAG_DONE);
                                }
                            } else { /* Could not render */
                                D(bug("[font.datatype] %s: failed to render\n", __func__));
                                FreeBitMap(RP.BitMap);
                            }
                        } else {
                            D(bug("[font.datatype] %s: failed to allocate bitmap\n", __func__));
                        }
                    }
                    /* Cleanup */
                    CloseFonts(Fonts, NumFonts);
                }
            }
            FreeDosObject(DOS_FIB, FIB); /* MUST be freed here, not before! */
        }
    }
    if (PH)
        FreeFontPrefs(PH);
    return(Result);
} /* GetFont */

/*
 * Font management
 */
STATIC struct TextFont **OpenFonts(struct Opts *opts,
                                    struct FontContentsHeader *fch, STRPTR name)
{
    struct TagItem MyTags[]=
    {
        {TA_DeviceDPI,  0},
        {TAG_DONE,      0}
    };

    struct TTextAttr TTA;

    struct TextFont **Fonts;
    struct TFontContents *TFC;
    LONG XDPI, YDPI;
    ULONG I, NumEntries = fch->fch_NumEntries;
    ULONG FCnt = 0;                                                             /* Number of opened fonts */

    D(
        bug("[font.datatype] %s(0x%p, 0x%p, '%s')\n", __func__, opts, fch, name);
        bug("[font.datatype] %s: %u entries\n", __func__, NumEntries);
    )
    if ((Fonts = AllocVec(NumEntries * sizeof(APTR), MEMF_CLEAR))) {
        D(bug("[font.datatype] %s: storage @ 0x%p\n", __func__, Fonts));
        for (I = 0; I < NumEntries; I++) {
            memset(&TTA, 0 , sizeof(struct TTextAttr));
            TFC = &TFontContents(fch)[I];                                       /* Get FontContents */
            TTA.tta_Name = name;                                                /* Copy attrs */
            TTA.tta_YSize = TFC->tfc_YSize;
            TTA.tta_Style = TFC->tfc_Style;
            TTA.tta_Flags = TFC->tfc_Flags | FPF_DISKFONT;
            if (ftst(TFC->tfc_Style, FSF_TAGGED)) {                               /* Tags should be set */
                TTA.tta_Tags = (struct TagItem *)&TFC->tfc_FileName[MAXFONTPATH - (TFC->tfc_TagCount * sizeof(struct TagItem))];
                D(bug("[font.datatype] %s: using font tags @ 0x%p\n", __func__, TTA.tta_Tags));
            } else if (opts->opt_DPIFlag) {                                       /* DPI given - set our own DPI tags */
                XDPI = clamp(opts->opt_XDPI, 1, 65535);
                YDPI = clamp(opts->opt_YDPI, 1, 65535);
                MyTags[0].ti_Data = (XDPI << 16) | YDPI;
                TTA.tta_Tags = MyTags;
                fset(TTA.tta_Style, FSF_TAGGED);
            }
            D(bug("[font.datatype] %s: opening font ..\n", __func__));
            if ((Fonts[I] = OpenDiskFont((struct TextAttr *)&TTA)))
                FCnt++;
        }
        if (FCnt) {                                                             /* font loaded ? */
            qsort(Fonts, NumEntries, sizeof(APTR), (SFUNC *)SortFunc);
            D(bug("[font.datatype] %s: returning 0x%p (%u)\n", __func__, Fonts, FCnt));
            return (Fonts);
        }
        bug("[font.datatype] %s: Failed to open any fonts\n", __func__);
        SetIoErr(ERROR_OBJECT_NOT_FOUND);
        FreeVec(Fonts);
    } else {
        bug("[font.datatype] %s: Failed to allocate storage\n", __func__);
    }
    return(NULL);
} /* OpenFonts */

STATIC VOID CloseFonts(struct TextFont **f, ULONG cnt)
{
    ULONG I;

    D(bug("[font.datatype] %s(0x%p, %u)\n", __func__, f, cnt));

    for (I = 0; I < cnt; I++)
        if (f[I])
            CloseFont(f[I]);
    FreeVec(f);
} /* CloseFonts */

/*
 * Rendering
 */
STATIC PBOOL GetWH(struct RastPort *rp, struct Opts *opt,
                    struct TextFont *f, ULONG *w, ULONG *h)
{
    STRPTR CurStr, *Strs;
    ULONG W, J, SCnt;

    D(bug("[font.datatype] %s()\n", __func__));

    if ((Strs = PrepStrings(opt, f, &SCnt))) {                                /* Allocate and init strings */
        /* Calculate sizes */
        SetFont(rp, f);
        for (J = 0; J < SCnt; J++) {
            CurStr = Strs[J];
            W = TextLength(rp, CurStr, strlen(CurStr));                         /* Calculate len of this line */
            if (W > *w)                                                         /* If larger... */
                *w = W;                                                         /* Make it new width */
            *h += f->tf_YSize;                                                  /* Add to height */
        }
        FreeVec(Strs);                                                          /* Free strings */
    }
    return (Strs != NULL);
} /* GetWH */

STATIC PBOOL Render(struct RastPort *rp, struct Opts *opt,
                    struct TextFont **f, ULONG cnt, ULONG w)
{
    struct TextFont *F;
    STRPTR CurStr, *Strs;
    ULONG SCnt, I, J, X, Y=0;
    PBOOL Rendered = FALSE;

    D(bug("[font.datatype] %s(0x%p, 0x%p, 0x%p, %u, %u)\n", __func__, rp, opt, f, cnt, w));

    for (I = 0; I < cnt; I++)                                                   /* For each font size */
        if ((F = f[I])) {                                                         /* Size opened? */
            D(bug("[font.datatype] %s: TextFont @ 0x%p\n", __func__, F));
            if ((Strs = PrepStrings(opt, F, &SCnt))) {                          /* Allocate and init strings */
                Rendered = TRUE;
                /* Do rendering */
                SetFont(rp, F);                                                 /* Make it a current font */
                for (J = 0; J < SCnt; J++) {
                    CurStr = Strs[J];
                    X = opt->opt_Center ? (w - TextLength(rp, CurStr, strlen(CurStr)))/2 : 0;
                    Move(rp, X, Y + F->tf_Baseline);
                    Text(rp, CurStr, strlen(CurStr));
                    D(bug("[font.datatype] %s: rendered @ %u,%u\n", __func__, X, Y + F->tf_Baseline));
                    D(bug("[font.datatype] %s: '%s'\n", __func__, CurStr));
                    Y += F->tf_YSize;
                }
                FreeVec(Strs);                                                  /* Free strings */
            }
        }
    return (Rendered);
} /* Render */

STATIC STRPTR *PrepStrings(struct Opts *opt, 
                            struct TextFont *f, ULONG *cnt)
{
    STRPTR S1, S2, CurStr;
    STRPTR *Strs, *Ss;
    ULONG I, Cnt, Temp;

    D(bug("[font.datatype] %s(0x%p, 0x%p, 0x%p)\n", __func__, opt, f, cnt));

    /* Calculate number of strings */
    Cnt = 2;
    if ((Ss = opt->opt_Strings))                                                /* Strings given */
        while (CurStr = *Ss++)                                                  /* For each string */
            if (*CurStr)                                                        /* Non-empty string */
                Cnt++;                                                          /* Increase count */

    /* Allocate and fill strings array */
    if ((Strs = AllocVec(Cnt * sizeof(STRPTR) + MAX_CHAR + MAX_CHAR2, MEMF_CLEAR))) {
        S1 = (STRPTR)Strs + Cnt * sizeof(STRPTR);
        S2 = S1 + MAX_CHAR;
        Cnt = 0;
        if ((opt->opt_FontName) && FontName(f)) {                               /* FontName - first */
                SNPrintf(S2, MAX_CHAR - 1, "%s %ld", FontName(f), f->tf_YSize);
                Strs[Cnt++] = S2;                                               /* Init strings array */
        }
        Temp = Cnt;
        if ((Ss = opt->opt_Strings))                                            /* User's strings given */
            while (CurStr = *Ss++)                                              /* While array not filled */
                if (*CurStr)                                                    /* Non-empty string */
                    Strs[Cnt++] = CurStr;                                       /* Fill in array */
        if ((Cnt == 0) || !Ss && (Cnt == Temp)) {                               /* Use charset */
            for (I = f->tf_LoChar; I <= f->tf_HiChar; I++)
                S1[I - f->tf_LoChar] = (I) ? (char)I : ' ';                     /* Make ASCII array */
            Strs[Cnt++] = S1;
        }
        *cnt = Cnt;
    }

    D(bug("[font.datatype] %s: returning 0x%p\n", __func__, Strs));
    return (Strs);
} /* PrepStrings */

STATIC LONG SortFunc(struct TextFont **tf1, struct TextFont **tf2)
{
    D(bug("[font.datatype] %s(0x%p, 0x%p)\n", __func__, tf1, tf2));

    if (*tf1 && *tf2)   return ((LONG)(*tf1)->tf_YSize - (*tf2)->tf_YSize);
    else if (*tf1)      return (-1);
    else if (*tf2)      return (1);
    else                return (0);
} /* SortFunc */

/**************************************************************************************************/

IPTR FONT__OM_NEW(Class *cl, Object *o, Msg msg)
{
    Object *newobj;

    D(bug("[font.datatype] %s()\n", __func__));

    newobj = (Object *)DoSuperMethodA(cl, o, msg);
    if (newobj) {
        if (!GetFont(cl, newobj, ((struct opSet *)msg)->ops_AttrList)) {
            CoerceMethod(cl, newobj, OM_DISPOSE);
            newobj = NULL;
        }
    }

    return (IPTR)newobj;
}