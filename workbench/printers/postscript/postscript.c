/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#define DEBUG 0
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

#define PS_NOSCALING    TRUE    /* Do all scaling in PostScript, not in printer.device */
#define PS_RGB          TRUE    /* RGB or CMYK printing */


/* Support binary compatability with AOS */
#ifdef __mc68000
#undef RAWFMTFUNC_STRING
#define RAWFMTFUNC_STRING (VOID (*)())"\x16\xC0\x4E\x75"
#endif

static LONG ps_Init(struct PrinterData *pd);
static VOID ps_Expunge(VOID);
static LONG ps_Open(union printerIO *ior);
static VOID ps_Close(union printerIO *ior);

static LONG ps_Render(SIPTR ct, LONG x, LONG y, LONG status);
static LONG ps_ConvFunc(UBYTE *buf, UBYTE c, LONG crlf_flag);
static LONG ps_DoPreferences(union printerIO *ior, LONG command);
static VOID ps_CallErrHook(union printerIO *ior, struct Hook *hook);
static LONG ps_DoSpecial(UWORD *command, UBYTE output_buffer[],
                         BYTE *current_line_position,
                         BYTE *current_line_spacing,
                         BYTE *crlf_flag, UBYTE params[]);

static CONST_STRPTR PED_Commands[] = {
    "\377",                             /*  0 aRIS   (reset) */
    "\377\377",                         /*  1 aRIN   (initialize) */
    ") aSHOW aIND (",                   /*  2 aIND   (linefeed) */
    ") aSHOW aNEL (",                   /*  3 aNEL   (CR/LF) */
    "\377",                             /*  4 aRI    (reverse LF) */
    ") aSHOW aSGR0 (",                  /*  5 aSGR0  (Courier) */
    ") aSHOW aSGR3 (",                  /*  6 aSGR3  (italics) */
    ") aSHOW aSGR23 (",                 /*  7 aSGR23 (no italics) */
    ") aSHOW aSGR4 (",                  /*  8 aSGR4  (underline) */
    ") aSHOW aSGR24 (",                 /*  9 aSGR24 (no underline) */
    ") aSHOW aSGR1 (",                  /* 10 aSGR1  (boldface) */
    ") aSHOW aSGR22 (",                 /* 11 aSGR22 (no boldface) */
    "\377",                             /* 12 aSFC   (set text color) */
    "\377",                             /* 13 aSBC   (set background color) */
    ") aSHOW aSHORP0 (",                /* 14 aSHORP0 (normal pitch) */
    ") aSHOW aSHORP2 (",                /* 15 aSHORP2 (elite) */
    ") aSHOW aSHORP1 (",                /* 16 aSHORP1 (no elite) */
    ") aSHOW aSHORP4 (",                /* 17 aSHORP4 (condensed) */
    ") aSHOW aSHORP3 (",                /* 18 aSHORP3 (no condensed) */
    ") aSHOW aSHORP6 (",                /* 19 aSHORP6 (enlarge) */
    ") aSHOW aSHORP5 (",                /* 20 aSHORT5 (no enlarge) */
    ") aSHOW aDEN6 (",                  /* 21 aDEN6   (shadow) */ 
    ") aSHOW aDEN5 (",                  /* 22 aDEN5   (no shadow) */
    ") aSHOW aDEN4 (",                  /* 23 aDEN4   (double strike) */
    ") aSHOW aDEN3 (",                  /* 24 aDEN3   (no double strike) */
    ") aSHOW aDEN2 (",                  /* 25 aDEN2   (NLQ) */
    ") aSHOW aDEN1 (",                  /* 26 aDEN1   (no NLQ) */
    ") aSHOW aSUS2 (",                  /* 27 aSUS2   (superscript) */
    ") aSHOW aSUS1 (",                  /* 28 aSUS1   (no superscript) */
    ") aSHOW aSUS4 (",                  /* 29 aSUS4   (subscript) */
    ") aSHOW aSUS3 (",                  /* 30 aSUS3   (no subscript) */
    ") aSHOW aSUS0 (",                  /* 31 aSUS0   (normal) */
    ") aSHOW aPLU (",                   /* 32 aPLU    (partial line up) */
    ") aSHOW aPLD (",                   /* 33 aPLD    (partial line down) */
    ") aSHOW aFNT0 (",                  /* 34 aFNT0   (Courier) */
    ") aSHOW aFNT1 (",                  /* 35 aFNT1   (Helvetica) */
    ") aSHOW aFNT2 (",                  /* 36 aFNT2   (Font 2) */
    ") aSHOW aFNT3 (",                  /* 37 aFNT3   (Font 3) */
    ") aSHOW aFNT4 (",                  /* 38 aFNT4   (Font 4) */
    ") aSHOW aFNT5 (",                  /* 39 aFNT5   (Font 5) */
    ") aSHOW aFNT6 (",                  /* 40 aFNT6   (Font 6) */
    ") aSHOW aFNT7 (",                  /* 41 aFNT7   (Font 7) */
    ") aSHOW aFNT8 (",                  /* 42 aFNT8   (Font 8) */
    ") aSHOW aFNT9 (",                  /* 43 aFNT9   (Font 9) */
    ") aSHOW aFNT10 (",                 /* 44 aFNT10  (Font 10) */
    ") aSHOW aPROP2 (",                 /* 45 aPROP2  (proportional) */
    ") aSHOW aPROP1 (",                 /* 46 aPROP1  (no proportional) */
    ") aSHOW aPROP0 (",                 /* 47 aPROP0  (default proportion) */
    "\377",                             /* 48 aTSS    (set proportional offset) */
    ") aSHOW aJFY5 (",                  /* 49 aJFY5   (left justify) */
    ") aSHOW aJFY7 (",                  /* 50 aJFY7   (right justify) */
    ") aSHOW aJFY6 (",                  /* 51 aJFY6   (full justify) */
    ") aSHOW aJFY0 (",                  /* 52 aJFY0   (no justify) */
    ") aSHOW aJFY3 (",                  /* 53 aJFY3   (letter space) */
    ") aSHOW aJFY1 (",                  /* 54 aJFY1   (word fill) */
    ") aSHOW aVERP0 (",                 /* 55 aVERP0  (1/8" line spacing) */
    ") aSHOW aVERP1 (",                 /* 56 aVERP1  (1/6" line spacing) */
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
    { PRTA_FloydDithering, TRUE },      /* 1 */
    { PRTA_MixBWColor, TRUE },          /* 2 */
    { PRTA_LeftBorder, 0 },             /* 3 */
    { PRTA_TopBorder,  0 },             /* 4 */
    { PRTA_NoScaling, PS_NOSCALING },
    { TAG_END }
};

AROS_PRINTER_TAG(PED, 44, 0,
        .ped_PrinterName = "PostScript",
        .ped_Init = ps_Init,
        .ped_Expunge = ps_Expunge,
        .ped_Open = ps_Open,
        .ped_Close = ps_Close,

        /* Settings for a 'graphics only' printer */
        .ped_PrinterClass = PPC_COLORGFX | PPCF_EXTENDED,
        .ped_MaxColumns = 0,    /* Set during render */
        .ped_ColorClass = (PS_RGB ? PCC_BGR : PCC_YMCB),
        .ped_NumCharSets = 2,
        .ped_NumRows = 1,        /* minimum pixels/row in gfx mode */
        .ped_MaxXDots = 0,       /* Set during render */
        .ped_MaxYDots = 0,       /* Set during render */
        .ped_XDotsInch = 0,      /* Set during render */
        .ped_YDotsInch = 0,      /* Set during render */
        .ped_Commands = (STRPTR *)PED_Commands, /* No ANSI commands */
        .ped_DoSpecial = ps_DoSpecial,
        .ped_Render = ps_Render,
        .ped_TimeoutSecs = 1000, /* For print-to-file timeouts */
        .ped_8BitChars = (STRPTR *)PED_8BitChars,
        .ped_PrintMode = 1,
        .ped_ConvFunc = ps_ConvFunc,
        .ped_TagList = &PED_TagList[0],
        .ped_DoPreferences = ps_DoPreferences,
        .ped_CallErrHook = ps_CallErrHook,
);

struct PrinterData *PD;
static LONG ps_PageID;
static BOOL ps_HeaderSent;
static CONST_STRPTR ps_PaperSize;
static LONG ps_PrintBufLen;
static LONG ps_SpacingLPI;
static LONG ps_FontCPI;
static UWORD ps_PrintShade;

static LONG ps_Init(struct PrinterData *pd)
{
    D(bug("ps_Init: pd=%p\n", pd));
    PD = pd;
    return 0;
}

static VOID ps_Expunge(VOID)
{
    D(bug("ps_Expunge\n"));
    PD = NULL;
}

static struct {
    char buff_a[16];
    char buff_b[16];
    char *buff;
    int len;
} ps_PState = {
    .buff = &ps_PState.buff_a[0]
};

#define PFLUSH() do { \
        PD->pd_PWrite(ps_PState.buff, ps_PState.len); \
        if (ps_PState.buff == &ps_PState.buff_a[0]) \
            ps_PState.buff = &ps_PState.buff_b[0]; \
        else \
            ps_PState.buff = &ps_PState.buff_a[0]; \
        ps_PState.len = 0; \
      } while (0)


static AROS_UFH2(void, ps_PPutC,
        AROS_UFHA(UBYTE, c, D0),
        AROS_UFHA(APTR, dummy, A3))
{
    AROS_USERFUNC_INIT

    /* Ignore the trailing 0 that RawDoFmt() tacks on the end */
    if (c == 0)
        return;

    ps_PState.buff[ps_PState.len++]=c;
    if (ps_PState.len >= 16)
        PFLUSH();

    AROS_USERFUNC_EXIT
}

#define ps_PWrite(fmt, ...) \
    do { \
        IPTR args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
        RawDoFmt(fmt, args, (VOID_FUNC)ps_PPutC, NULL); \
        PFLUSH(); \
    } while (0);

#ifdef __mc68000
#define STRING_FUNC     (VOID (*)())"\x16\xC0\x4E\x75"
#else
#define STRING_FUNC     RAWFMTFUNC_STRING
#endif

#define ps_VWrite(buf, fmt, ...) \
    do { \
        IPTR args[] = { AROS_PP_VARIADIC_CAST2IPTR(__VA_ARGS__) }; \
        RawDoFmt(fmt, args, STRING_FUNC, buf); \
    } while (0);

CONST TEXT ps_PageHeader[] =
        "%s\n"  /* Optional formfeed */
        "%%%%Page: %ld %ld\n"
        "%%%%BeginPageSetup\n"
        "%%%%PageOrientation: %s\n"
        "%%%%PageMedia: %s\n"
        "%%%%PageBoundingBox: %ld %ld %ld %ld\n"
        "%%%%EndPageSetup\n"
        "aFB (\\\n"
        ;

static void ps_VSendPage(char *buff, BOOL formfeed)
{
    ps_VWrite(buff, ps_PageHeader, formfeed ? ") aSHOW aFF" : "",
        ps_PageID, ps_PageID,
        /* Aspect */
        PD->pd_Preferences.PrintAspect ? "Landscape" : "Portrait",
        /* Media */
        ps_PaperSize,
        /* BoundingBox */
        0, 0,
        PED->ped_MaxXDots * 72 / PED->ped_XDotsInch,
        PED->ped_MaxYDots * 72 / PED->ped_YDotsInch
    );

    ps_PageID++;
}

static void ps_SendPage(BOOL formfeed)
{
    ps_PWrite(ps_PageHeader, formfeed ? ") aSHOW aFF" : "",
        ps_PageID, ps_PageID,
        /* Aspect */
        PD->pd_Preferences.PrintAspect ? "Landscape" : "Portrait",
        /* Media */
        ps_PaperSize,
        /* BoundingBox */
        0, 0,
        PED->ped_MaxXDots * 72 / PED->ped_XDotsInch,
        PED->ped_MaxYDots * 72 / PED->ped_YDotsInch
    );

    ps_PageID++;
}

static void ps_SendHeader(void)
{
    const TEXT header[] = 
        "%%!PS-Adobe-2.0\n"
        "%%%%Creator: AROS PostScript %ld.%ld (printer.device)\n"
        "%%%%BoundingBox: %ld %ld %ld %ld\n"
        "%%%%DocumentData: Clean7Bit\n"
        "%%%%LanguageLevel: 2\n"
        "%%%%DocumentMedia: %s %ld %ld 0 () ()\n"
        "%%%%EndComments\n"
        "%%%%BeginProlog\n"
        /* (ypixels * 72 / ydpi) - (72 / 4) [1/4 inch] */
        "/Page_Top %ld 72 mul %ld div 72 4 div sub def\n"
        /* 12 * left-margin-chars */
        "/Page_Left 12 %ld mul def\n"
        "/aFB { gsave Font_CPI setFont Page_Left Page_Top moveto } bind def\n"
        "/aFF  { grestore showpage } bind def\n"
        "/aImageBW {\n"
        "       /aImage_H exch def /aImage_W exch def\n"
        "       aImage_W aImage_H 1\n"
        "       [ aImage_W 0 0 aImage_H neg 0 aImage_H ]\n"
        "       { currentfile aImage_W 7 add 8 div cvi string readhexstring pop} bind\n"
        "       image\n"
        "} bind def\n"
        "/aImageGreyscale {\n"
        "       /aImage_H exch def /aImage_W exch def\n"
        "       aImage_W aImage_H 8\n"
        "       [ aImage_W 0 0 aImage_H neg 0 aImage_H ]\n"
        "       { currentfile aImage_W string readhexstring pop} bind\n"
        "       image\n"
        "} bind def\n"
        "/aImageColorRGB {\n"
        "       /aImage_H exch def /aImage_W exch def\n"
        "       aImage_W aImage_H 8\n"
        "       [ aImage_W 0 0 aImage_H neg 0 aImage_H ]\n"
        "       { currentfile aImage_W 3 mul string readhexstring pop} bind %% RGB\n"
        "       false 3 colorimage %% Single-pass \n"
        "} bind def\n"
        "/aImageColorCMYK {\n"
        "       /aImage_H exch def /aImage_W exch def\n"
        "       aImage_W aImage_H 8\n"
        "       [ aImage_W 0 0 aImage_H -1 mul 0 aImage_H ]\n"
        "       { currentfile aImage_W 4 mul string readhexstring pop} bind %% CMYK\n"
        "       false 4 colorimage %%\n"
        "} bind def\n"
        "\n"
        "/setFont { Font_Name findfont 1200 Font_CPI div scalefont setfont } bind def\n"
        "/Spacing { 72 Spacing_LPI div } bind def\n"
        "/Font_Name { \n"
        "    Font_P {\n"
        "        Font_I { \n"
        "            Font_B { /Helvetica-BoldOblique } { /Courier-Oblique } ifelse\n"
        "        } { \n"
        "            Font_B { /Helvetica-Bold } { /Courier } ifelse\n"
        "        } ifelse\n"
        "    } {\n"
        "        Font_I { \n"
        "            Font_B { /Courier-BoldOblique } { /Courier-Oblique } ifelse\n"
        "        } { \n"
        "            Font_B { /Courier-Bold } { /Courier } ifelse\n"
        "        } ifelse\n"
        "    } ifelse\n"
        " } bind def\n"
        "/Font_CPIDefault %ld def\n"
        "/Spacing_LPIDefault %ld def\n"
        "/Font_CPI Font_CPIDefault def\n"
        "/Font_P false def\n"
        "/Font_U false def\n"
        "/Font_B false def\n"
        "/Font_I false def\n"
        "/Font_Bg [ 1 1 1 ] def\n"
        "/Font_Fg [ 0 0 0 ] def\n"
        "/Spacing_LPI Spacing_LPIDefault def\n"
        "/aSHOW { \n"
        "     gsave Font_Bg aload pop setrgbcolor \n"
        "           0 -2 rmoveto dup stringwidth pop dup 0 rlineto\n"
        "           0 Spacing 2 sub rlineto neg 0 rlineto closepath fill\n"
        "     grestore\n"
        "     Font_U { gsave 0 -2 rmoveto dup stringwidth rlineto 0.3 setlinewidth stroke grestore } if\n"
        "     Font_Fg aload pop setrgbcolor\n"
        "     show } bind def\n"
        "%% Commands\n"
        "/aNEL { currentpoint exch pop Page_Left exch Spacing sub dup 0 lt { pop pop aFF aFB } { moveto } ifelse } bind def\n"
        "/aIND { currentpoint exch pop Page_Left exch moveto } bind def\n"
        "/aSGR0   { /Font_Fg [ 0 0 0 ] def /Font_Bg [ 1 1 1 ] def /Font_U false def /Font_B false def /Font_I false def Font_CPI setFont } bind def\n"
        "/aSGR3   { /Font_I true def setFont } bind def\n"
        "/aSGR23  { /Font_I false def setFont } bind def\n"
        "/aSGR4   { /Font_U true def setFont } bind def\n"
        "/aSGR24  { /Font_U false def setFont } bind def\n"
        "/aSGR1   { /Font_B true def setFont } bind def\n"
        "/aSGR22  { /Font_B false def setFont } bind def\n"
        "/aColor  { [ [ 0 0 0 ] [ 1 0 0 ] [ 0 1 0] [ 1 1 0] [ 0 0 1] [ 0.3 0.3 0.3] [0 1 1] [0.7 0.7 0.7] [ 0 0 0 ] [ 0 0 0 ] ] exch get } bind def\n"
        "/aSFC    { aColor /Font_Fg exch def } bind def\n"
        "/aSBC    { aColor /Font_Bg exch def } bind def\n"
        "/aSHORP0 { /Font_CPI Font_CPIDefault def setFont } bind def\n"
        "/aSHORP1 { /Font_CPI Font_CPIDefault def setFont } bind def\n"
        "/aSHORP2 { /Font_CPI 120 def setFont } bind def\n"
        "/aSHORP3 { /Font_CPI Font_CPIDefault def setFont } bind def\n"
        "/aSHORP4 { /Font_CPI 171 def setFont } bind def\n"
        "/aSHORP5 { /Font_CPI Font_CPIDefault def setFont } bind def\n"
        "/aSHORP6 { /Font_CPI 100 def setFont } bind def\n"
        "/aDEN6   { } bind def\n"
        "/aDEN5   { } bind def\n"
        "/aDEN4   { } bind def\n"
        "/aDEN3   { } bind def\n"
        "/aDEN2   { } bind def\n"
        "/aDEN1   { } bind def\n"
        "/aSUS2   { } bind def\n"
        "/aSUS1   { } bind def\n"
        "/aSUS4   { } bind def\n"
        "/aSUS3   { } bind def\n"
        "/aSUS0   { } bind def\n"
        "/aPLU    { } bind def\n"
        "/aPLD    { } bind def\n"
        "/aFNT0   { } bind def\n"
        "/aFNT1   { } bind def\n"
        "/aFNT2   { } bind def\n"
        "/aFNT3   { } bind def\n"
        "/aFNT4   { } bind def\n"
        "/aFNT5   { } bind def\n"
        "/aFNT6   { } bind def\n"
        "/aFNT7   { } bind def\n"
        "/aFNT8   { } bind def\n"
        "/aFNT9   { } bind def\n"
        "/aFNT10  { } bind def\n"
        "/aPROP2  { /Font_P true def } bind def\n"
        "/aPROP1  { /Font_P false def } bind def\n"
        "/aPROP0  { /Font_P false def } bind def\n"
        "/aJFY5   { } bind def\n"
        "/aJFY7   { } bind def\n"
        "/aJFY6   { } bind def\n"
        "/aJFY0   { } bind def\n"
        "/aJFY3   { } bind def\n"
        "/aJFY1   { } bind def\n"
        "/aVERP0  { /Spacing_LPI 8 def } bind def\n"
        "/aVERP1  { /Spacing_LPI 6 def } bind def\n"
        "%%%%EndProlog\n"
        ;

    if (!ps_HeaderSent) {
        ps_PWrite(header,
                __pmh.pmh_Version,
                __pmh.pmh_Revision,
                /* BoundingBox */
                0, 0,
                PED->ped_MaxXDots * 72 / PED->ped_XDotsInch,
                PED->ped_MaxYDots * 72 / PED->ped_YDotsInch,
                /* DocumentMedia */
                ps_PaperSize,
                PED->ped_MaxXDots * 72 / PED->ped_XDotsInch,
                PED->ped_MaxYDots * 72 / PED->ped_YDotsInch,
                /* Margin - assuming average font width is 1/2 its height*/
                PED->ped_MaxYDots, PED->ped_YDotsInch,
                PD->pd_Preferences.PrintLeftMargin,
                ps_FontCPI,
                ps_SpacingLPI);

        ps_PageID = 1;
        ps_HeaderSent = TRUE;
        ps_SendPage(FALSE);
    }
}

static LONG ps_Open(union printerIO *ior)
{
    D(bug("ps_Open: ior=%p\n", ior));

    ps_HeaderSent = FALSE;

    return 0;
}

static VOID ps_Close(union printerIO *ior)
{
    if (ps_HeaderSent) {
        ps_PWrite(") aSHOW aFF\n"
                  "%%%%Trailer\n"
                  "%%%%EOF\n");
    }
    D(bug("ps_Close: ior=%p\n", ior));
}

VOID color_get(struct ColorMap *cm,
                UBYTE *r,
                UBYTE *g,
                UBYTE *b,
                ULONG index)
{
    UWORD hibits = ((UWORD *)cm->ColorTable)[index];

    ULONG red8   = (hibits & 0x0f00) >> 4;
    ULONG green8 = (hibits & 0x00f0);
    ULONG blue8  = (hibits & 0x000f) << 4;

    if (cm->Type > COLORMAP_TYPE_V1_2) {
        UWORD lobits = ((UWORD *)cm->LowColorBits)[index];

        red8   |= (lobits & 0x0f00) >> 8;
        green8 |= (lobits & 0x00f0) >> 4;
        blue8  |= (lobits & 0x000f);
    }

    *r = red8;
    *g = green8;
    *b = blue8;
}

static LONG ps_RenderInit(struct IODRPReq *io, LONG width, LONG height)
{
    D(bug("ps_RenderInit: Dump raster %ldx%ld pixels, io_RastPort=%p\n", width, height, io->io_RastPort));
    D(bug("\t@%ldx%ld (%ldx%ld) => @%ldx%ld\n", 
           io->io_SrcX, io->io_SrcY, io->io_SrcWidth,
           io->io_SrcHeight, io->io_DestCols, io->io_DestRows));
    LONG alignOffsetX;
    LONG alignOffsetY;
    LONG x, y;

    ps_SendHeader();

    ps_PrintShade = PD->pd_Preferences.PrintShade;
    ps_PrintBufLen = PS_NOSCALING ? io->io_SrcWidth : width;
    PD->pd_PrintBuf = AllocMem(ps_PrintBufLen * 8 + 1, MEMF_ANY);
    if (PD->pd_PrintBuf == NULL)
        return PDERR_BUFFERMEMORY;

    if (PD->pd_Preferences.PrintFlags & PGFF_CENTER_IMAGE) {
        alignOffsetX = (PED->ped_MaxXDots - width) / 2;
        alignOffsetY = (PED->ped_MaxYDots - height) / 2;
    } else {
        alignOffsetX = 0;
        alignOffsetY = PED->ped_MaxYDots - height;
    }

    /* Leave text printing mode */
    ps_PWrite(") aSHOW grestore gsave\n");

    /* Move grapics location to (in dots):
     * x = alignOffsetX
     *        (dots)
     * y = alignOffsetY
     */
    x = alignOffsetX;
    y = alignOffsetY;

    /* Move text location to (in points):
     *  textx = PageLeft
     *  texty = y * 72 / dpiY - Spacing
     */
    ps_PWrite("Page_Left %ld 72 mul %ld div Spacing sub moveto\n",
              y, PED->ped_YDotsInch);

    /* Save the graphics context - we don't want the scaling & translation
     * when we return to text context later
     */
    ps_PWrite("gsave\n");
    ps_PWrite("%ld %ld div %ld %ld div translate\n",
            x * 72, PED->ped_XDotsInch,
            y * 72, PED->ped_YDotsInch);
    ps_PWrite("%ld %ld div %ld %ld div scale\n", width * 72, PED->ped_XDotsInch, height * 72, PED->ped_YDotsInch);

    if (PS_NOSCALING) {
        width = io->io_SrcWidth;
        height = io->io_SrcHeight;
    }

    ps_PWrite("%ld %ld ", width, height);

    switch (ps_PrintShade) {
    case SHADE_BW:
        ps_PWrite("aImageBW\n");
        break;
    case SHADE_GREYSCALE:
        ps_PWrite("aImageGreyscale\n");
        break;
    case SHADE_COLOR:
        if (PS_RGB) {
            ps_PWrite("aImageColorRGB\n");
        } else {
            ps_PWrite("aImageColorCMYK\n");
        }
        break;
    }

    return PDERR_NOERR;
}

static UBYTE tohex(UBYTE val)
{
    val &= 0xf;
    return (val < 10) ? ('0' + val) : ('a' + val - 10);
}

static LONG ps_RenderTransfer(struct PrtInfo *pi, LONG cindex, LONG y)
{
    UBYTE *ptr = PD->pd_PrintBuf;
    union colorEntry *src = pi->pi_ColorInt;
    int x;
    int const rgb[3] = { PCMRED, PCMGREEN, PCMBLUE };
    int const cmyk[4] = { PCMCYAN, PCMMAGENTA, PCMYELLOW, PCMBLACK };
    UBYTE color = 0;
    UBYTE threshold = (PD->pd_Preferences.PrintThreshold & 0xf) * 0x11;

    for (x = 0; x < ps_PrintBufLen; x++, src++) {
        int i;

        switch (ps_PrintShade) {
        case SHADE_BW:
            color <<= 1;
            if (PS_RGB)
                color |= (src->colorByte[PCMWHITE] >= threshold) ? 1 : 0;
            else
                color |= ((255 - src->colorByte[PCMBLACK]) >= threshold) ? 1 : 0;
            if ((x & 7)==7) {
                *(ptr++) = tohex(color>>4);
                *(ptr++) = tohex(color>>0);
            }
            break;
        case SHADE_GREYSCALE:
            if (PS_RGB)
                color = src->colorByte[PCMWHITE];
            else
                color = 255 - src->colorByte[PCMBLACK];
            *(ptr++) = tohex(color>>4);
            *(ptr++) = tohex(color>>0);
            break;
        case SHADE_COLOR:
            if (PS_RGB) {
                /* Single-pass RGB */
                for (i = 0; i < 3; i++) {
                    color = src->colorByte[rgb[i]];
                    *(ptr++) = tohex(color>>4);
                    *(ptr++) = tohex(color>>0);
                }
            } else {
                /* CMYK */
                for (i = 0; i < 4; i++) {
                    color = src->colorByte[cmyk[i]];
                    *(ptr++) = tohex(color>>4);
                    *(ptr++) = tohex(color>>0);
                }
            }
            break;
        }
    }
    if (ps_PrintShade == SHADE_BW && ((x & 7) != 0)) {
        for (; (x & 7) != 0; x++) {
            color <<= 1;
        }
        *(ptr++) = tohex(color>>4);
        *(ptr++) = tohex(color>>0);
    }

    *(ptr++) = '\n';

    return PDERR_NOERR;
}

static LONG ps_RenderFlush(LONG rows)
{
    int len = 0;

    switch (ps_PrintShade) {
    case SHADE_BW:
        len = (ps_PrintBufLen + 7) / 8;
        break;
    case SHADE_GREYSCALE:
        len = ps_PrintBufLen;
        break;
    case SHADE_COLOR:
        if (PS_RGB)
            len = ps_PrintBufLen * 3;
        else
            len = ps_PrintBufLen * 4;
        break;
    }

    PD->pd_PWrite(PD->pd_PrintBuf, len * 2 + 1);
    PD->pd_PBothReady();
    return PDERR_NOERR;
}

static LONG ps_RenderClear(void)
{
    memset(PD->pd_PrintBuf, '0', ps_PrintBufLen * 8);
    return PDERR_NOERR;
}

static LONG ps_RenderPreInit(struct IODRPReq *io, LONG flags)
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
        dpiX = 100;
        dpiY = 100;
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
    case PP_ELITE: ps_FontCPI = 120; break;
    case PP_FINE:  ps_FontCPI = 171; break;
    case PP_PICA:  ps_FontCPI = 100; break;
    default:
        return PDERR_BADDIMENSION;
    }

    switch (PD->pd_Preferences.PrintSpacing) {
    case PS_SIX_LPI:   ps_SpacingLPI = 6; break;
    case PS_EIGHT_LPI: ps_SpacingLPI = 8; break;
    default:
        return PDERR_BADDIMENSION;
    }

    switch (PD->pd_Preferences.PaperSize) {
/* PaperSize (in units of 0.0001 meters) */
    case US_LETTER: ps_PaperSize = "Letter";  break;   /* 8.5"x11" */
    case US_LEGAL:  ps_PaperSize = "Legal";   break;   /* 8.5"x14" */
    case N_TRACTOR: ps_PaperSize = "80-Col";  break;   /* 9.5"x11" */
    case W_TRACTOR: ps_PaperSize = "132-Col"; break;   /* 14.86"x11" */
/* European sizes */
    case EURO_A0:   ps_PaperSize = "A0";      break;  /* A0: 841 x 1189 */
    case EURO_A1:   ps_PaperSize = "A1";      break;  /* A1: 594 x 841  */
    case EURO_A2:   ps_PaperSize = "A2";      break;  /* A2: 420 x 594  */
    case EURO_A3:   ps_PaperSize = "A3";      break;  /* A3: 297 x 420  */
    case EURO_A4:   ps_PaperSize = "A4";      break;  /* A4: 210 x 297  */
    case EURO_A5:   ps_PaperSize = "A5";      break;  /* A5: 148 x 210  */
    case EURO_A6:   ps_PaperSize = "A6";      break;  /* A6: 105 x 148  */
    case EURO_A7:   ps_PaperSize = "A7";      break;  /* A7: 74 x 105   */
    case EURO_A8:   ps_PaperSize = "A8";      break;  /* A8: 52 x 74    */
    case CUSTOM:    ps_PaperSize = "Custom";  break;
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

    PED->ped_MaxColumns = width * ps_FontCPI / 2540;
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

static LONG ps_RenderClose(SIPTR error, ULONG flags)
{
    if (ps_PrintBufLen) {
        /* Restore to text context - the text location
         * has already been moved to after the raster
         */
        ps_PWrite("grestore (\n",);
        FreeMem(PD->pd_PrintBuf, ps_PrintBufLen * 8 + 1);
        PD->pd_PrintBuf=NULL;
        ps_PrintBufLen=0;
    }

    /* Send formfeed */
    if (!(flags & SPECIAL_NOFORMFEED))
        ps_SendPage(TRUE);

    return PDERR_NOERR;
}


static LONG ps_Render(SIPTR ct, LONG x, LONG y, LONG status)
{
    LONG err = PDERR_NOERR;

    switch (status) {
    case PRS_INIT:
        D(bug("PRS_INIT: ct=%p, x=%d, y=%d\n", ct, x, y));
        err = ps_RenderInit((struct IODRPReq *)ct, x, y);
        break;
    case PRS_TRANSFER:
        D(bug("PRS_TRANSFER: ct=%p, x=%d, y=%d\n", ct, x, y));
        err = ps_RenderTransfer((struct PrtInfo *)ct, x, y);
        break;
    case PRS_FLUSH:
        D(bug("PRS_FLUSH: ct=%p, x=%d, y=%d\n", ct, x, y));
        err = ps_RenderFlush(y);
        break;
    case PRS_CLEAR:
        D(bug("PRS_CLEAR: ct=%p, x=%d, y=%d\n", ct, x, y));
        err = ps_RenderClear();
        break;
    case PRS_CLOSE:
        D(bug("PRS_CLOSE: ct=%p, x=0x%0x, y=%d\n", ct, x, y));
        err = ps_RenderClose(ct, x);
        break;
    case PRS_PREINIT:
        D(bug("PRS_PREINIT: ct=%p, x=0x%0x, y=%d\n", ct, x, y));
        err = ps_RenderPreInit((struct IODRPReq *)ct, x);
        break;
    case PRS_CONVERT:
        D(bug("PRS_CONVERT: ct=%p, x=0x%0x, y=%d\n", ct, x, y));
        err = PDERR_NOERR;
        break;
    case PRS_CORRECT:
        D(bug("PRS_CORRECT: ct=%p, x=0x%0x, y=%d\n", ct, x, y));
        err = PDERR_NOERR;
        break;
    default:
        D(bug("PRS_xxxx(%d): ct=%p, x=0x%0x, y=%d\n", status, ct, x, y));
        err = PDERR_CANCEL;
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
static LONG ps_DoSpecial(UWORD *command, UBYTE output_buffer[],
                         BYTE *current_line_position,
                         BYTE *current_line_spacing,
                         BYTE *crlf_flag, UBYTE params[])
{
    LONG ret;
    D(bug("ps_DoSpecial: command=0x%04x, output_buffer=%p, current_line_position=%d, current_line_spacing=%d, crlf_flag=%d, params=%s\n",
                *command, output_buffer,  *current_line_position, *current_line_spacing, *crlf_flag, params));

    switch (*command) {
    case aRIN:
        ps_SendHeader();
        ret = -2;
        break;
    case aRIS:
        PD->pd_PWaitEnabled = '\375';
        ret = -2;
        break;
    case aSFC:
        ps_VWrite(output_buffer, ") aSHOW %ld aSFC (\n", params[0] % 10);
        ret = strlen(output_buffer);
        break;
    case aSBC:
        ps_VWrite(output_buffer, ") aSHOW %ld aSBC (\n", params[0] % 10);
        ret = strlen(output_buffer);
        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

static LONG ps_ConvFunc(UBYTE *buf, UBYTE c, LONG crlf_flag)
{
    D(bug("ps_ConvFunc: %p '%c' %d\n", buf, c, crlf_flag));

    if (c == '\014') {  /* Formfeed */
        ps_VSendPage(buf, TRUE);
        return strlen(buf);
    }

    if (c < 0x1f || c > 0x7f)
        return -1;

    if (c == '(' || c == ')' || c== '\\') {
        *(buf++) = '\\';
        *(buf++) = c;
        *(buf++) = 0;
        return 2;
    }

    if (c >= ' ' && c < 127) {
        *(buf++) = c;
        *(buf++) = 0;
        return 1;
    }

    *(buf++) = '\\';
    *(buf++) = ((c >> 6) & 7) + '0';
    *(buf++) = ((c >> 3) & 7) + '0';
    *(buf++) = ((c >> 0) & 7) + '0';
    *(buf++) = 0;
    return 4;
}

static LONG ps_DoPreferences(union printerIO *ior, LONG command)
{
    D(bug("ps_DoPreferences: ior=%p, command=%d\n"));
    return 0;
}

static VOID ps_CallErrHook(union printerIO *ior, struct Hook *hook)
{
    D(bug("ps_CallErrHook: ior=%p, hook=%p\n", ior, hook));
}
