/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <aros/debug.h>
#include <aros/printertag.h>

#include <clib/alib_protos.h>
#include <devices/printer.h>
#include <devices/prtgfx.h>
#include <prefs/printergfx.h>
#include <prefs/printertxt.h>
#include <exec/rawfmt.h>

#include <proto/exec.h>
#include <proto/graphics.h>

/* Support binary compatability with AOS */
#ifdef __mc68000
#undef RAWFMTFUNC_STRING
#define RAWFMTFUNC_STRING (VOID (*)())"\x16\xC0\x4E\x75"
#endif

static LONG sk_Init(struct PrinterData *pd);
static VOID sk_Expunge(VOID);
static LONG sk_Open(union printerIO *ior);
static VOID sk_Close(union printerIO *ior);

static LONG sk_Render(SIPTR ct, LONG x, LONG y, LONG status);
static LONG sk_ConvFunc(UBYTE *buf, UBYTE c, LONG crlf_flag);
static LONG sk_DoPreferences(union printerIO *ior, LONG command);
static VOID sk_CallErrHook(union printerIO *ior, struct Hook *hook);
static LONG sk_DoSpecial(UWORD *command, UBYTE output_buffer[],
                         BYTE *current_line_position,
                         BYTE *current_line_spacing,
                         BYTE *crlf_flag, UBYTE params[]);

static CONST_STRPTR PED_Commands[] = {
    "\377",                             /*  0 aRIS   (reset) */
    "\377\377",                         /*  1 aRIN   (initialize) */
    "\377",                             /*  2 aIND   (linefeed) */
    "\377",                             /*  3 aNEL   (CR/LF) */
    "\377",                             /*  4 aRI    (reverse LF) */
    "\377",                             /*  5 aSGR0  (Courier) */
    "\377",                             /*  6 aSGR3  (italics) */
    "\377",                             /*  7 aSGR23 (no italics) */
    "\377",                             /*  8 aSGR4  (underline) */
    "\377",                             /*  9 aSGR24 (no underline) */
    "\377",                             /* 10 aSGR1  (boldface) */
    "\377",                             /* 11 aSGR21 (no boldface) */
    "\377",                             /* 12 aSFC   (set text color) */
    "\377",                             /* 13 aSBC   (set background color) */
    "\377",                             /* 14 aSHORP0 (normal pitch) */
    "\377",                             /* 15 aSHORP2 (elite) */
    "\377",                             /* 16 aSHORP1 (no elite) */
    "\377",                             /* 17 aSHORP4 (condensed) */
    "\377",                             /* 18 aSHORP3 (no condensed) */
    "\377",                             /* 19 aSHORP6 (enlarge) */
    "\377",                             /* 20 aSHORT5 (no enlarge) */
    "\377",                             /* 21 aDEN6   (shadow) */ 
    "\377",                             /* 22 aDEN5   (no shadow) */
    "\377",                             /* 23 aDEN4   (double strike) */
    "\377",                             /* 24 aDEN3   (no double strike) */
    "\377",                             /* 25 aDEN2   (NLQ) */
    "\377",                             /* 26 aDEN1   (no NLQ) */
    "\377",                             /* 27 aSUS2   (superscript) */
    "\377",                             /* 28 aSUS1   (no superscript) */
    "\377",                             /* 29 aSUS4   (subscript) */
    "\377",                             /* 30 aSUS3   (no subscript) */
    "\377",                             /* 31 aSUS0   (normal) */
    "\377",                             /* 32 aPLU    (partial line up) */
    "\377",                             /* 33 aPLD    (partial line down) */
    "\377",                             /* 34 aFNT0   (Courier) */
    "\377",                             /* 35 aFNT1   (Helvetica) */
    "\377",                             /* 36 aFNT2   (Font 2) */
    "\377",                             /* 37 aFNT3   (Font 3) */
    "\377",                             /* 38 aFNT4   (Font 4) */
    "\377",                             /* 39 aFNT5   (Font 5) */
    "\377",                             /* 40 aFNT6   (Font 6) */
    "\377",                             /* 41 aFNT7   (Font 7) */
    "\377",                             /* 42 aFNT8   (Font 8) */
    "\377",                             /* 43 aFNT9   (Font 9) */
    "\377",                             /* 44 aFNT10  (Font 10) */
    "\377",                             /* 45 aPROP2  (proportional) */
    "\377",                             /* 46 aPROP1  (no proportional) */
    "\377",                             /* 47 aPROP0  (default proportion) */
    "\377",                             /* 48 aTSS    (set proportional offset) */
    "\377",                             /* 49 aJFY5   (left justify) */
    "\377",                             /* 50 aJFY7   (right justify) */
    "\377",                             /* 51 aJFY6   (full justify) */
    "\377",                             /* 52 aJFY0   (no justify) */
    "\377",                             /* 53 aJFY3   (letter space) */
    "\377",                             /* 54 aJFY1   (word fill) */
    "\377",                             /* 55 aVERP0  (1/8" line spacing) */
    "\377",                             /* 56 aVERP1  (1/6" line spacing) */
    "\377",                             /* 57 aSLPP   (form length) */
    "\377",                             /* 58 aPERF   (skip n perfs) */
    "\377",                             /* 59 aPERF0  (no skip perfs) */
    "\377",                             /* 60 aLMS    (left margin) */
    "\377",                             /* 61 aRMS    (right margin) */
    "\377",                             /* 62 aTMS    (top margin) */
    "\377",                             /* 63 aBMS    (bot margin) */
    "\377",                             /* 64 aSTBM   (top & bottom margin) */
    "\377",                             /* 65 aSLRM   (left & right margin) */
    "\377",                             /* 66 aCAM    (no margins) */
    "\377",                             /* 67 aHTS    (horizontal tabs) */
    "\377",                             /* 68 aVTS    (vertical tabs) */
    "\377",                             /* 69 aTBC0   (clear horizontal tab) */
    "\377",                             /* 70 aTBC3   (clear all horiz. tabs) */
    "\377",                             /* 71 aTBC1   (clear vertical tab) */
    "\377",                             /* 72 aTBC4   (clear all vertical tabs) */
    "\377",                             /* 73 aTBCALL (clear all tabs) */
    "\377",                             /* 74 aTBSALL (default tabs) */
    "\377",                             /* 75 aEXTEND (extended chars) */
    "\377",                             /* 76 aRAW    (next N chars are literal) */
};

static CONST_STRPTR cmdTable[] = {
    "aRIS",     /* 0 */
    "aRIN",     /* 1 */
    "aIND",     /* 2 */
    "aNEL",     /* 3 */
    "aRI",      /* 4 */
    "aSGR0",    /* 5 */
    "aSGR3",    /* 6 */
    "aSGR23",   /* 7 */
    "aSGR4",    /* 8 */
    "aSGR24",   /* 9 */
    "aSGR1",    /* 10 */
    "aSGR21",   /* 11 */
    "aSFC",     /* 12 */
    "aSBC",     /* 13 */
    "aSHORP0",  /* 14 */
    "aSHORP2",  /* 15 */
    "aSHORP1",  /* 16 */
    "aSHORP4",  /* 17 */
    "aSHORP3",  /* 18 */
    "aSHORP6",  /* 19 */
    "aSHORT5",  /* 20 */
    "aDEN6",    /* 21 */
    "aDEN5",    /* 22 */
    "aDEN4",    /* 23 */
    "aDEN3",    /* 24 */
    "aDEN2",    /* 25 */
    "aDEN1",    /* 26 */
    "aSUS2",    /* 27 */
    "aSUS1",    /* 28 */
    "aSUS4",    /* 29 */
    "aSUS3",    /* 30 */
    "aSUS0",    /* 31 */
    "aPLU",     /* 32 */
    "aPLD",     /* 33 */
    "aFNT0",    /* 34 */
    "aFNT1",    /* 35 */
    "aFNT2",    /* 36 */
    "aFNT3",    /* 37 */
    "aFNT4",    /* 38 */
    "aFNT5",    /* 39 */
    "aFNT6",    /* 40 */
    "aFNT7",    /* 41 */
    "aFNT8",    /* 42 */
    "aFNT9",    /* 43 */
    "aFNT10",   /* 44 */
    "aPROP2",   /* 45 */
    "aPROP1",   /* 46 */
    "aPROP0",   /* 47 */
    "aTSS",     /* 48 */
    "aJFY5",    /* 49 */
    "aJFY7",    /* 50 */
    "aJFY6",    /* 51 */
    "aJFY0",    /* 52 */
    "aJFY3",    /* 53 */
    "aJFY1",    /* 54 */
    "aVERP0",   /* 55 */
    "aVERP1",   /* 56 */
    "aSLPP",    /* 57 */
    "aPERF",    /* 58 */
    "aPERF0",   /* 59 */
    "aLMS",     /* 60 */
    "aRMS",     /* 61 */
    "aTMS",     /* 62 */
    "aBMS",     /* 63 */
    "aSTBM",    /* 64 */
    "aSLRM",    /* 65 */
    "aCAM",     /* 66 */
    "aHTS",     /* 67 */
    "aVTS",     /* 68 */
    "aTBC0",    /* 69 */
    "aTBC3",    /* 70 */
    "aTBC1",    /* 71 */
    "aTBC4",    /* 72 */
    "aTBCALL",  /* 73 */
    "aTBSALL",  /* 74 */
    "aEXTEND",  /* 75 */
    "aRAW",     /* 76 */
};

static CONST_STRPTR PED_8BitChars[] = {

          " ", /* SPC (160) */
          "?", /* ! */ 
          "?", /* c */ 
          "?", /* £ */
          "?", /* o */
          "?", /* Y */
          "|", 
          "?", /* S */
          "?", 
          "?", /* Copyright */ 
          "?", /* a */
          "?", /* < */ 
          "?", /* - */
          "?", /* SHY */
          "?", /* R */ 
          "?", /* - */
          "?", /* o (176) */
          "?", /* +- */ 
          "?", /* 2 */
          "?", /* 3 */
          "?", 
          "?", /* u */ 
          "?", /* P */ 
          "?", /* . */
          "?", /* , */ 
          "?", /* 1 */
          "?", /* o */
          "?", /* > */
          "?", /* 1/4 */
          "?", /* 1/2 */
          "?", /* 3/4 */ 
          "?", /* ? */
          "?", /* A' (192) */
          "?", /* A' */ 
          "?", /* A^ */ 
          "?", /* A~ */ 
          "?", /* A: */ 
          "?", /* Ao */ 
          "?", /* AE */ 
          "?", /* C */
          "?", /* E' */ 
          "?", /* E' */ 
          "?", /* E^ */ 
          "?", /* E: */ 
          "?", /* I' */ 
          "?", /* I' */ 
          "?", /* I^ */ 
          "?", /* I: */
          "?", /* D- (208) */ 
          "?", /* N~ */ 
          "?", /* O' */ 
          "?", /* O' */ 
          "?", /* O^ */ 
          "?", /* O~ */ 
          "?", /* O: */ 
          "?", /* x  */
          "?", /* 0  */ 
          "?", /* U' */
          "?", /* U' */
          "?", /* U^ */ 
          "?", /* U: */ 
          "?", /* Y' */ 
          "?", /* p  */ 
          "?", /* B  */
          "?", /* a' (224) */
          "?", /* a' */ 
          "?", /* a^ */ 
          "?", /* a~ */ 
          "?", /* a: */ 
          "?", /* ao */ 
          "?", /* ae */ 
          "?", /* c */
          "?", /* e' */ 
          "?", /* e' */ 
          "?", /* e^ */ 
          "?", /* e: */ 
          "?", /* i' */ 
          "?", /* i' */ 
          "?", /* i^ */ 
          "?", /* i: */
          "?", /* o (240) */ 
          "?", /* n~ */ 
          "?", /* o' */ 
          "?", /* o' */ 
          "?", /* o^ */ 
          "?", /* o~ */ 
          "?", /* o: */ 
          "?", /* /  */
          "?", /* 0  */ 
          "?", /* u' */
          "?", /* u' */
          "?", /* u^ */ 
          "?", /* u: */ 
          "?", /* y' */ 
          "?", /* p  */ 
          "?", /* y: */
};

static struct TagItem PED_TagList[] = {
    { PRTA_8BitGuns, TRUE },            /* 0 */
    { PRTA_MixBWColor, TRUE },          /* 1 */
    { PRTA_LeftBorder, 0 },             /* 2 */
    { PRTA_TopBorder,  0 },             /* 3 */
//    { PRTA_ConvertSource, TRUE },       /* 4 */
    { PRTA_ColorCorrection, TRUE },     /* 5 */
    { TAG_END }
};

AROS_PRINTER_TAG(PED, 44, 0,
        .ped_PrinterName = "Skeleton",
        .ped_Init = sk_Init,
        .ped_Expunge = sk_Expunge,
        .ped_Open = sk_Open,
        .ped_Close = sk_Close,

        /* Settings for a 'graphics only' printer */
        .ped_PrinterClass = PPC_COLORGFX | PPCF_EXTENDED,
        .ped_MaxColumns = 0,    /* Set during render */
        .ped_ColorClass = PCC_YMCB | PCC_MULTI_PASS,
        .ped_NumCharSets = 2,
        .ped_NumRows = 1,        /* minimum pixels/row in gfx mode */
        .ped_MaxXDots = 0,       /* Set during render */
        .ped_MaxYDots = 0,       /* Set during render */
        .ped_XDotsInch = 0,      /* Set during render */
        .ped_YDotsInch = 0,      /* Set during render */
        .ped_Commands = (STRPTR *)PED_Commands, /* No ANSI commands */
        .ped_DoSpecial = sk_DoSpecial,
        .ped_Render = sk_Render,
        .ped_TimeoutSecs = 1000, /* For print-to-file timeouts */
        .ped_8BitChars = (STRPTR *)PED_8BitChars,
        .ped_PrintMode = 1,
        .ped_ConvFunc = sk_ConvFunc,
        .ped_TagList = &PED_TagList[0],
        .ped_DoPreferences = sk_DoPreferences,
        .ped_CallErrHook = sk_CallErrHook,
);

struct PrinterData *PD;
static CONST_STRPTR sk_PaperSize;
static LONG sk_PrintBufLen;
static LONG sk_SpacingLPI;
static LONG sk_FontCPI;

static LONG sk_Init(struct PrinterData *pd)
{
    D(bug("sk_Init: pd=%p\n", pd));
    PD = pd;
    return 0;
}

static VOID sk_Expunge(VOID)
{
    D(bug("sk_Expunge\n"));
    PD = NULL;
}

static struct {
    char buff_a[16];
    char buff_b[16];
    char *buff;
    int len;
} sk_PState = {
    .buff = &sk_PState.buff_a[0]
};

#define PFLUSH() do { \
        PD->pd_PWrite(sk_PState.buff, sk_PState.len); \
        if (sk_PState.buff == &sk_PState.buff_a[0]) \
            sk_PState.buff = &sk_PState.buff_b[0]; \
        else \
            sk_PState.buff = &sk_PState.buff_a[0]; \
        sk_PState.len = 0; \
      } while (0)


static AROS_UFH2(void, sk_PPutC,
        AROS_UFHA(UBYTE, c, D0),
        AROS_UFHA(APTR, dummy, A3))
{
    AROS_USERFUNC_INIT

    /* Ignore the trailing 0 that RawDoFmt() tacks on the end */
    if (c == 0)
        return;

    sk_PState.buff[sk_PState.len++]=c;
    if (sk_PState.len >= 16)
        PFLUSH();

    AROS_USERFUNC_EXIT
}

#define sk_PWrite(fmt, ...) \
    do { \
        IPTR args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
        RawDoFmt(fmt, args, (VOID_FUNC)sk_PPutC, NULL); \
        PFLUSH(); \
    } while (0);

#define sk_VWrite(buf, fmt, ...) \
    do { \
        IPTR args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
        RawDoFmt(fmt, args, RAWFMTFUNC_STRING, buf); \
    } while (0);

static LONG sk_Open(union printerIO *ior)
{
    D(bug("sk_Open: ior=%p\n", ior));

    return 0;
}

static VOID sk_Close(union printerIO *ior)
{
    D(bug("sk_Close: ior=%p\n", ior));
}

static LONG sk_RenderInit(struct IODRPReq *io, LONG width, LONG height)
{
    D(bug("sk_RenderInit: Dump raster %ldx%ld pixels, io_RastPort=%p\n", width, height, io->io_RastPort));
    D(bug("\t@%ldx%ld (%ldx%ld) => @%ldx%ld\n", 
           io->io_SrcX, io->io_SrcY, io->io_SrcWidth,
           io->io_SrcHeight, io->io_DestCols, io->io_DestRows));
    LONG alignOffsetX = 0;
    LONG alignOffsetY = 0;
    LONG x, y;

    sk_PrintBufLen = width;
    PD->pd_PrintBuf = AllocMem(sk_PrintBufLen * 6, MEMF_ANY);
    if (PD->pd_PrintBuf == NULL)
        return PDERR_BUFFERMEMORY;

    if (PD->pd_Preferences.PrintFlags & PGFF_CENTER_IMAGE) {
        alignOffsetX = (PED->ped_MaxXDots - width) / 2;
        alignOffsetY = (PED->ped_MaxYDots - height) / 2;
    }

    sk_PWrite("[IMAGE]\n");

    return PDERR_NOERR;
}

static LONG sk_RenderTransfer(struct PrtInfo *pi, LONG color, LONG y)
{
    UBYTE *ptr = PD->pd_PrintBuf;
    union colorEntry *src = pi->pi_ColorInt;
    int x;

    D(bug("\tSource=%p\n", src));

    sk_PWrite("[Image %ld] ", y);
    for (x = 0; x < pi->pi_width; x++, src++, ptr++) {
        *ptr = "  ..ccooCCOO@@##"[(src->colorByte[PCMBLACK] >> 4) & 0xf];
    }

    return PDERR_NOERR;
}

static LONG sk_RenderFlush(LONG rows)
{
    PD->pd_PWrite(PD->pd_PrintBuf, sk_PrintBufLen);
    PD->pd_PWrite("\n", 1);
    return PDERR_NOERR;
}

static LONG sk_RenderClear(void)
{
    memset(PD->pd_PrintBuf, ' ', sk_PrintBufLen);
    return PDERR_NOERR;
}

static LONG sk_RenderPreInit(struct IODRPReq *io, LONG flags)
{
    ULONG dpiX, dpiY;
    ULONG width, height;

    /* Select DPI */
    switch (flags & SPECIAL_DENSITYMASK) {
    case SPECIAL_DENSITY1:
        dpiX = 72;
        dpiY = 72;
        break;
    case SPECIAL_DENSITY2:
        dpiX = 10;
        dpiY = 10;
        break;
    case SPECIAL_DENSITY3:
        dpiX = 120;
        dpiY = 120;
        break;
    case SPECIAL_DENSITY4:
        dpiX = 150;
        dpiY = 150;
        break;
    case SPECIAL_DENSITY5:
        dpiX = 300;
        dpiY = 300;
        break;
    case SPECIAL_DENSITY6:
        dpiX = 600;
        dpiY = 600;
        break;
    case SPECIAL_DENSITY7:
        dpiX = 1200;
        dpiY = 1200;
        break;
    default:
        dpiX = 72;
        dpiY = 72;
    }

    switch (PD->pd_Preferences.PrintPitch) {
    case PP_ELITE: sk_FontCPI = 120; break;
    case PP_FINE:  sk_FontCPI = 171; break;
    case PP_PICA:  sk_FontCPI = 100; break;
    default:
        return PDERR_BADDIMENSION;
    }

    switch (PD->pd_Preferences.PrintSpacing) {
    case PS_SIX_LPI:   sk_SpacingLPI = 6; break;
    case PS_EIGHT_LPI: sk_SpacingLPI = 8; break;
    default:
        return PDERR_BADDIMENSION;
    }

    switch (PD->pd_Preferences.PaperSize) {
/* PaperSize (in units of 0.0001 meters) */
    case US_LETTER: sk_PaperSize = "Letter";  break;   /* 8.5"x11" */
    case US_LEGAL:  sk_PaperSize = "Legal";   break;   /* 8.5"x14" */
    case N_TRACTOR: sk_PaperSize = "80-Col";  break;   /* 9.5"x11" */
    case W_TRACTOR: sk_PaperSize = "132-Col"; break;   /* 14.86"x11" */
/* European sizes */
    case EURO_A0:   sk_PaperSize = "A0";      break;  /* A0: 841 x 1189 */
    case EURO_A1:   sk_PaperSize = "A1";      break;  /* A1: 594 x 841  */
    case EURO_A2:   sk_PaperSize = "A2";      break;  /* A2: 420 x 594  */
    case EURO_A3:   sk_PaperSize = "A3";      break;  /* A3: 297 x 420  */
    case EURO_A4:   sk_PaperSize = "A4";      break;  /* A4: 210 x 297  */
    case EURO_A5:   sk_PaperSize = "A5";      break;  /* A5: 148 x 210  */
    case EURO_A6:   sk_PaperSize = "A6";      break;  /* A6: 105 x 148  */
    case EURO_A7:   sk_PaperSize = "A7";      break;  /* A7: 74 x 105   */
    case EURO_A8:   sk_PaperSize = "A8";      break;  /* A8: 52 x 74    */
    case CUSTOM:    sk_PaperSize = "Custom";  break;
    default:        return PDERR_BADDIMENSION;
    }

    /* Set up for the page size */
    switch (PD->pd_Preferences.PaperSize) {
/* PaperSize (in units of 0.0001 meters) */
    case US_LETTER: width = 2159; height = 2794; break;   /* 8.5"x11" */
    case US_LEGAL:  width = 2159; height = 3556; break;   /* 8.5"x14" */
    case N_TRACTOR: width = 2413; height = 2794; break;   /* 9.5"x11" */
    case W_TRACTOR: width = 3774; height = 2794; break;   /* 14.86"x11" */
/* European sizes */
    case EURO_A0:   width = 8410; height = 11890; break;  /* A0: 841 x 1189 */
    case EURO_A1:   width = 5940; height =  8410; break;  /* A1: 594 x 841  */
    case EURO_A2:   width = 4200; height =  5940; break;  /* A2: 420 x 594  */
    case EURO_A3:   width = 2970; height =  4200; break;  /* A3: 297 x 420  */
    case EURO_A4:   width = 2100; height =  2970; break;  /* A4: 210 x 297  */
    case EURO_A5:   width = 1480; height =  2100; break;  /* A5: 148 x 210  */
    case EURO_A6:   width = 1050; height =  1480; break;  /* A6: 105 x 148  */
    case EURO_A7:   width =  740; height =  1050; break;  /* A7: 74 x 105   */
    case EURO_A8:   width =  520; height =   740; break;  /* A8: 52 x 74    */
    case CUSTOM:    width  = PD->pd_Preferences.PrintMaxWidth * 254 / 10;
                    height = PD->pd_Preferences.PrintMaxHeight * 254 / 10;
                    break;
    default:        return PDERR_CANCEL;
    }

    PED->ped_MaxColumns = width * sk_FontCPI / 2540;
    PED->ped_XDotsInch = dpiX;
    PED->ped_YDotsInch = dpiY;
    PED->ped_MaxXDots = width * dpiX / 254;
    PED->ped_MaxYDots = height * dpiY / 254;
D(bug("MaxColumns=%d, dpiX=%d, dpiY=%d, MaxXDots=%d, MaxYDots=%d (%d x %d in)\n",
        PED->ped_MaxColumns, PED->ped_XDotsInch, PED->ped_YDotsInch,
        PED->ped_MaxXDots, PED->ped_MaxYDots,
        PED->ped_MaxXDots / dpiX, PED->ped_MaxYDots / dpiY));

    return PDERR_NOERR;
}

static LONG sk_RenderClose(SIPTR error, ULONG flags)
{
    if (error != PDERR_CANCEL) {
        /* Send formfeed */
        if (!(flags & SPECIAL_NOFORMFEED))
            sk_PWrite("[FF]\n");
    }

    sk_PWrite("[Close]\n");

    return PDERR_NOERR;
}

static LONG sk_RenderNextColor(void)
{
    return PDERR_NOERR;
}
    
/* If Tag PRTA_ConvertSource is set, this function is called instead
 * of the printer.device built-in to convert.
 *
 * The size of each entry is either sizeof(union colorEntry), or
 * Tag PRTA_ColorSize (if set)
 *
 * The conversion is done in-place.
 */
static LONG sk_RenderConvert(APTR row, LONG entries, LONG is_pixels)
{
    return PDERR_NOERR;
}

/* If Tag PRTA_ColorCorrection is set, this function is called instead
 * of the printer.device built-in to correct printer-space colors.
 *
 * The size of each entry is either sizeof(union colorEntry), or
 * Tag PRTA_ColorSize (if set)
 *
 * The conversion is done in-place.
 */
static LONG sk_RenderCorrect(APTR row, LONG entries, LONG is_pixels)
{
    return PDERR_NOERR;
}

static LONG sk_Render(SIPTR ct, LONG x, LONG y, LONG status)
{
    LONG err = PDERR_NOERR;

    switch (status) {
    case PRS_INIT:
        D(bug("PRS_INIT: IODRPReq=%p, width=%d, height=%d\n", ct, x, y));
        err = sk_RenderInit((struct IODRPReq *)ct, x, y);
        break;
    case PRS_TRANSFER:
        D(bug("PRS_TRANSFER: PrtInfo=%p, color=%d, row=%d\n", ct, x, y));
        err = sk_RenderTransfer((struct PrtInfo *)ct, x, y);
        break;
    case PRS_FLUSH:
        D(bug("PRS_FLUSH: ct=%p, x=%d, rows=%d\n", ct, x, y));
        err = sk_RenderFlush(y);
        break;
    case PRS_CLEAR:
        D(bug("PRS_CLEAR: ct=%p, x=%d, y=%d\n", ct, x, y));
        err = sk_RenderClear();
        break;
    case PRS_CLOSE:
        D(bug("PRS_CLOSE: error=%d, io_Special=0x%0x, y=%d\n", ct, x, y));
        err = sk_RenderClose(ct, x);
        break;
    case PRS_PREINIT:
        D(bug("PRS_PREINIT: IODRPReq=%p, io_Special=0x%0x, y=%d\n", ct, x, y));
        err = sk_RenderPreInit((struct IODRPReq *)ct, x);
        break;
    case PRS_NEXTCOLOR:
        D(bug("PRS_NEXTCOLOR: ct=%p, x=0x%0x, y=%d\n", ct, x, y));
        err = sk_RenderNextColor();
        break;
    case PRS_UNKNOWN:
        D(bug("PRS_UNKNOWN: ct=%p, x=0x%0x, y=%d\n", ct, x, y));
        err = PDERR_NOERR;
        break;
    case PRS_CONVERT:
        D(bug("PRS_CONVERT: row=%p, entries=%d, type=%s\n", ct, x, y ? "pixels" : "union colorEntry"));
        err = sk_RenderConvert((APTR)ct, x, y);
        break;
    case PRS_CORRECT:
        D(bug("PRS_CORRECT: row=%p, entries=%d, type=%s\n", ct, x, y ? "pixels" : "union colorEntry"));
        err = sk_RenderCorrect((APTR)ct, x, y);
        break;
    default:
        D(bug("PRS_xxxx(%d): ct=%p, x=0x%0x, y=%d\n", status, ct, x, y));
        break;
    }
        
    return err;
}

/* Text output:
 *  > 0 = processed, add N chars
 *  0   = not handled by DoSpecial
 *  -1  = Unsupported command
 *  -2  = Processed, but no additional chars in the buffer
 */
static LONG sk_DoSpecial(UWORD *command, UBYTE output_buffer[],
                         BYTE *current_line_position,
                         BYTE *current_line_spacing,
                         BYTE *crlf_flag, UBYTE params[])
{
    D(bug("sk_DoSpecial: command=0x%04x, output_buffer=%p, current_line_position=%d, current_line_spacing=%d, crlf_flag=%d, params=%s\n",
                *command, output_buffer,  *current_line_position, *current_line_spacing, *crlf_flag, params));

    sk_VWrite(output_buffer, "[%s %ld,%ld,%ld,%ld]", cmdTable[*command], params[0], params[1], params[2], params[3]);

    return strlen(output_buffer);
}

static LONG sk_ConvFunc(UBYTE *buf, UBYTE c, LONG crlf_flag)
{
    D(bug("sk_ConvFunc: %p '%c' %d\n", buf, c, crlf_flag));

    /* NOTE: For compatability with AOS 3.x, do 
     *       not attempt to convert ESC or \377
     *       characters if you want DoSpecial() to work.
     */
    if (c == 0x1b || c == 0xff)
        return -1;

    /* As a demo, we're going to UPPERCASE all characters,
     * and put a '\' in front of the modified character.
     */
    if (c >= 'a' && c <= 'z') {
        *(buf++) = '\\';
        *(buf++) = c;
        return 2;
    }

    return -1;
}

static LONG sk_DoPreferences(union printerIO *ior, LONG command)
{
    D(bug("sk_DoPreferences: ior=%p, command=%d\n"));
    return 0;
}

static VOID sk_CallErrHook(union printerIO *ior, struct Hook *hook)
{
    D(bug("sk_CallErrHook: ior=%p, hook=%p\n", ior, hook));
}
