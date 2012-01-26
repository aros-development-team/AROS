/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#define DEBUG 1
#include <aros/debug.h>
#include <clib/alib_protos.h>
#include <devices/printer.h>
#include <devices/prtgfx.h>

#include <proto/graphics.h>

#include "printers_intern.h"

static struct Library *GfxBase;

static VOID ps_Init(struct PrinterData *pd);
static VOID ps_Expunge(VOID);
static LONG ps_Open(union printerIO *ior);
static VOID ps_Close(union printerIO *ior);

static LONG ps_Render(LONG ct, LONG x, LONG y, LONG status);
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

PRINTER_TAG(PED, 44, 0,
        .ped_PrinterName = "PostScript",
        .ped_Init = ps_Init,
        .ped_Expunge = ps_Expunge,
        .ped_Open = ps_Open,
        .ped_Close = ps_Close,

        /* Settings for a 'graphics only' printer */
        .ped_PrinterClass = PPC_COLORGFX | PPCF_EXTENDED,
        .ped_ColorClass = PCC_YMCB,
        .ped_MaxColumns = 0,    /* Set during render */
        .ped_NumCharSets = 0,
        .ped_NumRows = 1,
        .ped_MaxXDots = 0,       /* Set during render */
        .ped_MaxYDots = 0,       /* Set during render */
        .ped_XDotsInch = 0,      /* Set during render */
        .ped_YDotsInch = 0,      /* Set during render */
        .ped_Commands = NULL,    /* No ANSI commands */
        .ped_DoSpecial = ps_DoSpecial,
        .ped_Render = ps_Render,
        .ped_TimeoutSecs = 10,
        .ped_8BitChars = NULL,
        .ped_PrintMode = 1,
        .ped_ConvFunc = ps_ConvFunc,
        .ped_TagList = &PED_TagList[0],
        .ped_DoPreferences = ps_DoPreferences,
        .ped_CallErrHook = ps_CallErrHook,
);

struct PrinterData *PD;

static VOID ps_Init(struct PrinterData *pd)
{
    D(bug("ps_Init: pd=%p\n", pd));
    PD = pd;
    GfxBase = OpenLibrary("graphics.library", 0);
}

static VOID ps_Expunge(VOID)
{
    D(bug("ps_Expunge\n"));
    PD = NULL;
    CloseLibrary(GfxBase);
}

static LONG ps_Open(union printerIO *ior)
{
    D(bug("ps_Open: ior=%p\n", ior));
    return 0;
}

static VOID ps_Close(union printerIO *ior)
{
    D(bug("ps_Close: ior=%p\n", ior));
}

static LONG ps_DoSpecial(UWORD *command, UBYTE output_buffer[],
                         BYTE *current_line_position,
                         BYTE *current_line_spacing,
                         BYTE *crlf_flag, UBYTE params[])
{
    D(bug("ps_DoSpecial: command=%p, output_buffer=%p, current_line_position=%p, current_line_spacing=%p, crlf_flag=%p, params=%p\n",
                command, output_buffer,  current_line_position, current_line_spacing, crlf_flag, params));
    return 0;
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

static void PWrite(const char *format, ...)
{
    va_list args;
    char state = 0;
    int len = 0;
    UBYTE bval = 0;
    LONG lval = 0;
    char buff[16];

#define PFLUSH() do { \
        PD->pd_PWrite(buff, len); \
        PD->pd_PBothReady(); \
        len = 0; \
      } while (0)

#define PUTC(c) do { \
    buff[len++]=c; \
    if (len >= sizeof(buff)) PFLUSH(); \
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
            case 'D':
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
            case 'X':
                bval = (UBYTE)va_arg(args, ULONG);
                PUTC("0123456789abcdef"[(bval >> 4) & 0xf]);
                PUTC("0123456789abcdef"[(bval >> 0) & 0xf]);
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

static LONG ps_PrintBufLen;

static LONG ps_RenderInit(struct IODRPReq *io, LONG x, LONG y)
{
    D(bug("ps_RenderInit: Dump raster %dx%d pixels, io_RastPort=%p\n", x, y, io->io_RastPort));
    D(bug("\t@%dx%d (%dx%d) => @%dx%d\n", 
           io->io_SrcX, io->io_SrcY, io->io_SrcWidth,
           io->io_SrcHeight, io->io_DestCols, io->io_DestRows));

    ps_PrintBufLen = io->io_SrcWidth;
    PD->pd_PrintBuf = AllocMem(ps_PrintBufLen * 6, MEMF_ANY);
    if (PD->pd_PrintBuf == NULL)
        return PDERR_BUFFERMEMORY;

    /* Write a postscript colorimage */
    PWrite("%%!PS-Adobe-2.0\n");
    PWrite("gsave\n");
    PWrite("%D %D translate\n", PD->pd_Preferences.PrintXOffset * PED->ped_XDotsInch / 10 +
                                (PED->ped_MaxXDots - x) / 2,
                                (PED->ped_MaxYDots - y) / 2);
    PWrite("%D %D scale\n", x, y);
    PWrite("%D %D 8 [%D 0 0 %D 0 %D]\n",
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

static void ps_FinishImage(void)
{
    if (ps_PrintBufLen) {
        PWrite(">}\nfalse 3 colorimage\n");
        FreeMem(PD->pd_PrintBuf, ps_PrintBufLen * 6);
        PD->pd_PrintBuf=NULL;
        ps_PrintBufLen=0;
    }
}

static LONG ps_RenderEject(void)
{
    ps_FinishImage();

    PWrite("grestore\n");

    return PDERR_NOERR;
}

static LONG ps_RenderClose(struct IODRPReq *io, ULONG flags)
{
    LONG err = PDERR_NOERR;

    ps_FinishImage();

    if (!(flags & SPECIAL_NOFORMFEED))
        err = ps_RenderEject();


    return err;
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
    case PRS_EJECT:
        D(bug("PRS_EJECT: ct=%p, x=0x%0x, y=%d\n", ct, x, y));
        err = ps_RenderEject();
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

static LONG ps_ConvFunc(UBYTE *buf, UBYTE c, LONG crlf_flag)
{
    D(bug("ps_ConvFunc: buf=%p, c=%c, crlf_flag=%d\n", buf, c, crlf_flag));
    return c;
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
