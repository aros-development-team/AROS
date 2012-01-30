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

#include <proto/graphics.h>

static struct Library *GfxBase;

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

static struct TagItem PED_TagList[] = {
    { PRTA_8BitGuns, TRUE },            /* 0 */
    { PRTA_NewColor, TRUE },            /* 1 */
    { PRTA_ColorSize, 3 },              /* 2 */
    { PRTA_NoScaling, TRUE },           /* 3 */
    { PRTA_MixBWColor, FALSE },         /* 4 */
    { PRTA_LeftBorder, 0 },             /* 5 */
    { PRTA_TopBorder,  0 },             /* 6 */
    { TAG_END }
};

static CONST_STRPTR PED_Commands[] = {
    "\377",                             /*  0 aRIS   (reset) */
    "",                                 /*  1 aRIN   (initialize) */
    "\n",                               /*  2 aIND   (linefeed) */
    "\r\n",                             /*  3 aNEL   (CR/LF) */
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

AROS_PRINTER_TAG(PED, 44, 0,
        .ped_PrinterName = "PostScript",
        .ped_Init = ps_Init,
        .ped_Expunge = ps_Expunge,
        .ped_Open = ps_Open,
        .ped_Close = ps_Close,

        /* Settings for a 'graphics only' printer */
        .ped_PrinterClass = PPC_COLORGFX | PPCF_EXTENDED,
        .ped_MaxColumns = 0,    /* Set during render */
        .ped_ColorClass = PCC_BGR,
        .ped_NumCharSets = 2,
        .ped_NumRows = 1,
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

static LONG ps_Init(struct PrinterData *pd)
{
    D(bug("ps_Init: pd=%p\n", pd));
    PD = pd;
    GfxBase = OpenLibrary("graphics.library", 0);
    return 0;
}

static VOID ps_Expunge(VOID)
{
    D(bug("ps_Expunge\n"));
    PD = NULL;
    CloseLibrary(GfxBase);
}

static void PWrite(const char *format, ...)
{
    va_list args;
    char state = 0;
    int len = 0;
    LONG lval = 0;
    static char buff_a[16];
    static char buff_b[16];
    static char *buff = &buff_a[0];

#define PFLUSH() do { \
        PD->pd_PWrite(buff, len); \
        if (buff == &buff_a[0]) \
            buff = &buff_b[0]; \
        else \
            buff = &buff_a[0]; \
        len = 0; \
      } while (0)

#define PUTC(c) do { \
    buff[len++]=c; \
    if (len >= 16) PFLUSH(); \
  } while (0)

    va_start(args, format);

    for (; *format; format++) {
        switch (state) {
        case 0:
            if (*format == '%') {
                state = '%';
            } else {
                PUTC(*format);
            }
            break;
        case '%':
            switch (*format) {
            case 'd':
                lval = va_arg(args, LONG);
                if (lval == 0)
                    PUTC('0');
                else {
                    BYTE out[10];
                    int olen = 0;
                    if (lval < 0) {
                        PUTC('-');
                        lval = -lval;
                    }
                    do {
                        out[olen++] = lval % 10;
                        lval /= 10;
                    } while (lval > 0);
                    while (olen--) {
                        PUTC(out[olen] + '0');
                    }
                }
                break;
            case '%':
                PUTC('%');
                break;
            default:
                PUTC('?');
                break;
            }
            state = 0;
            break;
        default:
            state = 0;
            break;
        }
    }

    va_end(args);

    PFLUSH();
}

static BOOL ps_HeaderSent;

static void ps_SendHeader(void)
{
    const TEXT header[] = 
        "%%!PS-Adobe-2.0\n"
        "/Courier findfont 12 scalefont setfont\n"
        "5 %d moveto gsave\n"
        "(\\\n";

    if (!ps_HeaderSent) {
        PWrite(header, PED->ped_MaxYDots * 72 / PED->ped_YDotsInch - 36);
        ps_HeaderSent = TRUE;
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
    if (ps_HeaderSent)
        PWrite(") show showpage grestore\n");
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

static LONG ps_PrintBufLen;

static LONG ps_RenderInit(struct IODRPReq *io, LONG x, LONG y)
{
    D(bug("ps_RenderInit: Dump raster %dx%d pixels, io_RastPort=%p\n", x, y, io->io_RastPort));
    D(bug("\t@%dx%d (%dx%d) => @%dx%d\n", 
           io->io_SrcX, io->io_SrcY, io->io_SrcWidth,
           io->io_SrcHeight, io->io_DestCols, io->io_DestRows));

    ps_SendHeader();

    ps_PrintBufLen = io->io_SrcWidth;
    PD->pd_PrintBuf = AllocMem(ps_PrintBufLen * 6, MEMF_ANY);
    if (PD->pd_PrintBuf == NULL)
        return PDERR_BUFFERMEMORY;

    /* Write a postscript colorimage */
    PWrite(") show grestore gsave\n");
    PWrite("%d %d translate\n", PD->pd_Preferences.PrintXOffset * PED->ped_XDotsInch / 10 +
                                (PED->ped_MaxXDots - x) / 2,
                                (PED->ped_MaxYDots - y) / 2);
    PWrite("%d %d scale\n", x, y);
    PWrite("%d %d 8 [%d 0 0 %d 0 %d]\n",
            io->io_SrcWidth, io->io_SrcHeight,
            io->io_SrcWidth, -io->io_SrcHeight, io->io_SrcHeight);
    PWrite("{<\n");

    return PDERR_NOERR;
}

static UBYTE tohex(UBYTE val)
{
    val &= 0xf;
    return (val < 10) ? ('0' + val) : ('a' + val - 10);
}

static LONG ps_RenderTransfer(struct PrtInfo *pi, LONG x, LONG y)
{
    UBYTE *ptr = PD->pd_PrintBuf;
    UBYTE *src = (UBYTE *)pi->pi_ColorInt;
    ptr += (2 * x);

    for (x = 0; x < ps_PrintBufLen; x++, src += 3, ptr += 6) {
        int i;
        for (i = 0; i < 3; i++) {
            ptr[i*2+0] = tohex(src[2-i]>>4);
            ptr[i*2+1] = tohex(src[2-i]>>0);
        }
    }

    return PDERR_NOERR;
}

static LONG ps_RenderFlush(LONG rows)
{
    PD->pd_PWrite(PD->pd_PrintBuf, ps_PrintBufLen * 6);
    PD->pd_PWrite("\n", 1);
    return PDERR_NOERR;
}

static LONG ps_RenderClear(void)
{
    memset(PD->pd_PrintBuf, '0', ps_PrintBufLen * 6);
    return PDERR_NOERR;
}

static LONG ps_RenderPreInit(struct IODRPReq *io, LONG flags)
{
    ULONG dpiX = 0, dpiY = 0;
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
    }

    /* Set up for the page size */
    switch (PD->pd_Preferences.PaperSize) {
/* PaperSize (in um) */
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

    PED->ped_MaxColumns = width * 10 / 254;
    PED->ped_XDotsInch = dpiX;
    PED->ped_YDotsInch = dpiY;
    PED->ped_MaxXDots = width * dpiX / 254;
    PED->ped_MaxYDots = height * dpiY / 254;

    return PDERR_NOERR;
}

static LONG ps_RenderClose(struct IODRPReq *io, ULONG flags)
{
    if (ps_PrintBufLen) {
        PWrite(">}\nfalse 3 colorimage\n");
        PWrite("grestore (\n");
        FreeMem(PD->pd_PrintBuf, ps_PrintBufLen * 6);
        PD->pd_PrintBuf=NULL;
        ps_PrintBufLen=0;
    }

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
        err = ps_RenderClose((struct IODRPReq *)ct, x);
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

/* Text output */
static LONG ps_DoSpecial(UWORD *command, UBYTE output_buffer[],
                         BYTE *current_line_position,
                         BYTE *current_line_spacing,
                         BYTE *crlf_flag, UBYTE params[])
{
    D(bug("ps_DoSpecial: command=0x%04x, output_buffer=%p, current_line_position=%d, current_line_spacing=%d, crlf_flag=%d, params=%s\n",
                *command, output_buffer,  *current_line_position, *current_line_spacing, *crlf_flag, params));

    if (*command == aRIN) {
        ps_SendHeader();
    }

    if (*command == aRIS) {
        PD->pd_PWaitEnabled = '\375';
    }

    if (*command == aIND) {
        strcpy(output_buffer, "\\\n) show\n"
                /* Get current point => x=5, y=currentline */
                    "currentpoint exch pop 5 exch moveto\n"
                    "(\\\n");
        return strlen(output_buffer);
    }

    if (*command == aNEL) {
        strcpy(output_buffer, "\\\n) show\n"
                /* Get current point => x=5, y=currentline-11 */
                    "currentpoint exch pop 5 exch 11 sub moveto\n"
                    "(\\\n");
        return strlen(output_buffer);
    }


    return -1;
}

static LONG ps_ConvFunc(UBYTE *buf, UBYTE c, LONG crlf_flag)
{
    D(bug("ps_ConvFunc: %p '%c' %d\n", buf, c, crlf_flag));

    if (c < 0x1f || c > 0x7f)
        return -1;

    if (c == '(' || c == ')') {
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
