/*
 * Copyright (C) 2026, The AROS Development Team.  All rights reserved.
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#include <aros/printertag.h>

#include <devices/printer.h>
#include <devices/prtgfx.h>
#include <dos/dostags.h>
#include <exec/memory.h>
#include <exec/rawfmt.h>
#include <prefs/printergfx.h>
#include <prefs/printertxt.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "pdfjob.h"

static LONG pdf_Init(struct PrinterData *pd);
static VOID pdf_Expunge(VOID);
static LONG pdf_Open(union printerIO *ior);
static VOID pdf_Close(union printerIO *ior);
static LONG pdf_Render(SIPTR ct, LONG x, LONG y, LONG status);
static LONG pdf_ConvFunc(UBYTE *buf, UBYTE c, LONG crlf_flag);
static LONG pdf_DoSpecial(UWORD *command, UBYTE output_buffer[],
                          BYTE *current_line_position,
                          BYTE *current_line_spacing,
                          BYTE *crlf_flag, UBYTE params[]);

static STRPTR PED_Commands[aRAW + 1];

static struct TagItem PED_TagList[] = {
    { PRTA_8BitGuns,      TRUE },
    { PRTA_FloydDithering, TRUE },
    { PRTA_MixBWColor,    TRUE },
    { PRTA_NoScaling,     TRUE },
    { TAG_DONE }
};

AROS_PRINTER_TAG(PED, 44, 1,
    .ped_PrinterName = "PDF",
    .ped_Init = pdf_Init,
    .ped_Expunge = pdf_Expunge,
    .ped_Open = pdf_Open,
    .ped_Close = pdf_Close,
    .ped_PrinterClass = PPC_COLORGFX | PPCF_EXTENDED,
    .ped_ColorClass = PCC_BGR,
    .ped_NumCharSets = 1,
    .ped_NumRows = 1,
    .ped_Commands = PED_Commands,
    .ped_DoSpecial = pdf_DoSpecial,
    .ped_Render = pdf_Render,
    .ped_TimeoutSecs = 1000,
    .ped_PrintMode = 1,
    .ped_ConvFunc = pdf_ConvFunc,
    .ped_TagList = PED_TagList,
);

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

/* Each unit loads its own copy of the driver, so this state is per unit. */
static struct PrinterData *PD;
static BPTR spool;
static char jobname[80], pdfname[80], command[256];
static LONG fault;
static ULONG total;
static ULONG pagew, pageh, left, right;
static ULONG tx, ty, pitch, spacing, style;
static ULONG imgw, imgh, row, channels;
static UBYTE *raster;
static BOOL inpage;

/* Spooled coordinates are 1/100 point */
#define PTS(v, dpi)     ((v) * 7200 / (dpi))

static void spool_bytes(const void *p, ULONG n)
{
    if (fault)
        return;
    if (n > PDFJOB_MAX_SPOOL - total || (ULONG)FWrite(spool, (CONST_APTR)p, 1, n) != n) {
        fault = PDERR_CANCEL;
        return;
    }
    total += n;
}

static void spool_byte(UBYTE c)
{
    spool_bytes(&c, 1);
}

static void spool_word(ULONG n)
{
    UBYTE b[4] = { n >> 24, n >> 16, n >> 8, n };

    spool_bytes(b, 4);
}

static void set_margins(void)
{
    ULONG first = PD->pd_Preferences.PrintLeftMargin;
    ULONG last = PD->pd_Preferences.PrintRightMargin;

    left = first > 0 ? (first - 1) * pitch : 0;
    right = last > first ? last * pitch : pagew;
    if (right > pagew)
        right = pagew;
    if (left >= right)
        left = 0;
}

static void end_page(void)
{
    if (inpage) {
        spool_byte('E');
        inpage = FALSE;
    }
    tx = left;
    ty = spacing;
}

static void begin_page(void)
{
    if (fault || inpage)
        return;
    if (!spool) {
        spool = Open(jobname, MODE_NEWFILE);
        if (!spool) {
            fault = PDERR_CANCEL;
            return;
        }
        spool_bytes(PDFJOB_MAGIC, 8);
    }
    spool_byte('P');
    spool_word(pagew);
    spool_word(pageh);
    inpage = TRUE;
}

static LONG set_geometry(ULONG flags)
{
    static const ULONG densities[] = { 72, 72, 100, 120, 150, 300, 600, 1200 };
    struct Preferences *prefs = &PD->pd_Preferences;
    ULONG w, h, dpi;

    /* Paper size in 1/10 mm */
    switch (prefs->PaperSize) {
    case US_LETTER: w = 2159; h = 2794;  break;
    case US_LEGAL:  w = 2159; h = 3556;  break;
    case N_TRACTOR: w = 2413; h = 2794;  break;
    case W_TRACTOR: w = 3774; h = 2794;  break;
    case EURO_A0:   w = 8410; h = 11890; break;
    case EURO_A1:   w = 5940; h = 8410;  break;
    case EURO_A2:   w = 4200; h = 5940;  break;
    case EURO_A3:   w = 2970; h = 4200;  break;
    case EURO_A4:   w = 2100; h = 2970;  break;
    case EURO_A5:   w = 1480; h = 2100;  break;
    case EURO_A6:   w = 1050; h = 1480;  break;
    case EURO_A7:   w = 740;  h = 1050;  break;
    case EURO_A8:   w = 520;  h = 740;   break;
    case CUSTOM:
        w = prefs->PrintMaxWidth * 254 / 10;
        h = prefs->PrintMaxHeight * 254 / 10;
        break;
    default:
        return PDERR_BADDIMENSION;
    }
    if (!w || !h || w > 50000 || h > 50000)
        return PDERR_BADDIMENSION;

    if (!(flags & SPECIAL_DENSITYMASK))
        flags |= ((ULONG)prefs->PrintDensity << 8) & SPECIAL_DENSITYMASK;
    dpi = densities[(flags & SPECIAL_DENSITYMASK) / SPECIAL_DENSITY1];
    if (prefs->PrintAspect == ASPECT_VERT) {
        ULONG t = w;
        w = h;
        h = t;
    }
    if (w * dpi / 254 > 65535 || h * dpi / 254 > 65535)
        return PDERR_BADDIMENSION;

    pagew = w * 7200 / 254;
    pageh = h * 7200 / 254;
    PED->ped_XDotsInch = PED->ped_YDotsInch = dpi;
    PED->ped_MaxXDots = w * dpi / 254;
    PED->ped_MaxYDots = h * dpi / 254;
    PED->ped_MaxColumns = pagew / pitch > 255 ? 255 : pagew / pitch;
    set_margins();

    return 0;
}

/* Pica 10, Elite 12 and Fine 17.1 characters per inch; 6 or 8 lines per inch */
#define PITCH_PICA      720
#define PITCH_ELITE     600
#define PITCH_FINE      421

static void set_pitch(ULONG p)
{
    pitch = p;
    PED->ped_MaxColumns = pagew / pitch > 255 ? 255 : pagew / pitch;
    set_margins();
}

static void text_defaults(void)
{
    struct Preferences *prefs = &PD->pd_Preferences;

    spacing = prefs->PrintSpacing == EIGHT_LPI ? 900 : 1200;
    set_pitch(prefs->PrintPitch == ELITE ? PITCH_ELITE : prefs->PrintPitch == FINE ? PITCH_FINE : PITCH_PICA);
}

static LONG pdf_Init(struct PrinterData *pd)
{
    int i;

    PD = pd;
    SysBase = pd->pd_Device.dd_ExecBase;
    DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 39);
    if (!DOSBase)
        return PDERR_CANCEL;

    for (i = 0; i <= aRAW; i++)
        PED_Commands[i] = "\377";
    /* PRS_PREINIT can come before the first open */
    pitch = PITCH_PICA;
    spacing = 1200;

    return 0;
}

static VOID pdf_Expunge(VOID)
{
    if (DOSBase)
        CloseLibrary((struct Library *)DOSBase);
    DOSBase = NULL;
}

static LONG pdf_Open(union printerIO *ior)
{
    ULONG sequence = 0;
    BPTR lock;
    LONG err;

    spool = BNULL;
    fault = 0;
    total = 0;
    inpage = FALSE;
    raster = NULL;
    row = imgw = imgh = 0;
    style = 0;

    do {
        NewRawDoFmt("T:PDF-%p-%lu.job", RAWFMTFUNC_STRING, jobname, PD, sequence);
        NewRawDoFmt("T:PDF-%p-%lu.pdf", RAWFMTFUNC_STRING, pdfname, PD, sequence++);
        lock = Lock(jobname, SHARED_LOCK);
        if (lock)
            UnLock(lock);
    } while (lock && sequence < 10000);
    if (lock)
        return PDERR_CANCEL;

    err = set_geometry(0);
    text_defaults();
    tx = left;
    ty = spacing;

    return err;
}

static void send_pdf(void)
{
    UBYTE *buffer;
    BPTR output;
    LONG n = 0;

    output = Open(pdfname, MODE_OLDFILE);
    buffer = AllocVec(16384, MEMF_ANY);
    if (!output || !buffer)
        fault = PDERR_CANCEL;
    else {
        while ((n = Read(output, buffer, 16384)) > 0) {
            /* pd_PWrite is asynchronous: wait before reusing the buffer */
            if (PD->pd_PWrite(buffer, n) || PD->pd_PBothReady()) {
                fault = PDERR_CANCEL;
                break;
            }
        }
        if (n < 0)
            fault = PDERR_CANCEL;
    }
    FreeVec(buffer);
    if (output)
        Close(output);
}

static VOID pdf_Close(union printerIO *ior)
{
    if (raster) {
        FreeVec(raster);
        raster = NULL;
        fault = PDERR_CANCEL;
    }
    if (!spool)
        return;

    end_page();
    spool_byte('Z');
    if (!Close(spool))
        fault = PDERR_CANCEL;
    spool = BNULL;

    if (!fault) {
        NewRawDoFmt("C:PDFRender %s %s", RAWFMTFUNC_STRING, command, jobname, pdfname);
        if (SystemTags(command, NP_StackSize, 512 * 1024, TAG_DONE) != 0)
            fault = PDERR_CANCEL;
    }
    if (!fault)
        send_pdf();

    if (!fault) {
        DeleteFile(jobname);
        DeleteFile(pdfname);
    } else {
        /* ped_Close cannot return an error: keep the job for recovery */
        BPTR log = Open("T:PDF-errors.log", MODE_READWRITE);

        if (log) {
            Seek(log, 0, OFFSET_END);
            FPrintf(log, "PDF job failed; retained %s\n", jobname);
            Close(log);
        }
    }
}

static LONG pdf_ConvFunc(UBYTE *buf, UBYTE c, LONG crlf_flag)
{
    switch (c) {
    case '\f':
        begin_page();
        end_page();
        return 0;
    case '\r':
        tx = left;
        return 0;
    case '\n':
        ty += spacing;
        if (!crlf_flag)
            tx = left;
        if (ty > pageh) {
            begin_page();
            end_page();
        }
        return 0;
    case '\t':
        tx = (tx / (pitch * 8) + 1) * pitch * 8;
        return 0;
    case '\b':
        if (tx >= left + pitch)
            tx -= pitch;
        return 0;
    }
    if (c < 32 || c == 127)
        return 0;

    if (tx + pitch > right) {
        tx = left;
        ty += spacing;
    }
    if (ty > pageh)
        end_page();
    begin_page();
    {
        ULONG words[4] = { tx, ty, pitch * 5 / 3, style };
        UBYTE record[18];
        int i;

        record[0] = 'T';
        for (i = 0; i < 4; i++) {
            record[1 + 4 * i] = words[i] >> 24;
            record[2 + 4 * i] = words[i] >> 16;
            record[3 + 4 * i] = words[i] >> 8;
            record[4 + 4 * i] = words[i];
        }
        record[17] = c;
        spool_bytes(record, sizeof(record));
    }
    tx += pitch;

    return 0;
}

static LONG pdf_DoSpecial(UWORD *command, UBYTE output_buffer[],
                          BYTE *current_line_position,
                          BYTE *current_line_spacing,
                          BYTE *crlf_flag, UBYTE params[])
{
    switch (*command) {
    case aRIS:
    case aRIN:   style = 0; text_defaults(); break;
    case aSGR0:  style = 0;              break;
    case aIND:   pdf_ConvFunc(NULL, '\n', 1); break;
    case aNEL:   pdf_ConvFunc(NULL, '\n', 0); break;
    case aRI:
        if (ty >= spacing)
            ty -= spacing;
        break;
    case aSGR1:  style |= 1;             break;
    case aSGR22: style &= ~1;            break;
    case aSGR3:  style |= 2;             break;
    case aSGR23: style &= ~2;            break;
    case aSGR4:  style |= 4;             break;
    case aSGR24: style &= ~4;            break;
    case aSHORP0:
    case aSHORP1:
    case aSHORP3: set_pitch(PITCH_PICA);  break;
    case aSHORP2: set_pitch(PITCH_ELITE); break;
    case aSHORP4: set_pitch(PITCH_FINE);  break;
    case aVERP0: spacing = 900;          break;
    case aVERP1: spacing = 1200;         break;
    }

    /* Consumed: nothing goes to the printer */
    return -2;
}

static LONG pdf_Render(SIPTR ct, LONG x, LONG y, LONG status)
{
    struct Preferences *prefs = &PD->pd_Preferences;
    struct IODRPReq *io = (struct IODRPReq *)ct;
    ULONG i, w, h, top, ascent;

    switch (status) {
    case PRS_PREINIT:
        return set_geometry(x);

    case PRS_INIT:
        if (fault)
            return fault;
        if (raster || x <= 0 || y <= 0 || !io->io_SrcWidth || !io->io_SrcHeight)
            return PDERR_BADDIMENSION;
        imgw = io->io_SrcWidth;
        imgh = io->io_SrcHeight;
        row = 0;
        channels = prefs->PrintShade == SHADE_COLOR ? 3 : 1;
        if (imgw > PDFJOB_MAX_IMAGE / channels || imgh > PDFJOB_MAX_IMAGE / (imgw * channels))
            return PDERR_BADDIMENSION;
        raster = AllocVec(imgw * channels, MEMF_ANY);
        if (!raster)
            return PDERR_BUFFERMEMORY;

        /* From the top of the current line, as on paper (ty is its baseline) */
        w = PTS(x, PED->ped_XDotsInch);
        h = PTS(y, PED->ped_YDotsInch);
        ascent = pitch * 5 / 3 * 629 / 1000;
        top = ty > ascent ? ty - ascent : 0;
        if (top + h > pageh) {
            /* something on this page, or the paper moved: the picture goes on the next one */
            if (inpage || ty > spacing) {
                begin_page();
                end_page();
            }
            top = 0;
        }
        begin_page();
        spool_byte('I');
        if (((io->io_Special & SPECIAL_CENTER) || (prefs->PrintFlags & PGFF_CENTER_IMAGE)) && w < pagew)
            spool_word((pagew - w) / 2);
        else
            spool_word(0);
        spool_word(top);
        spool_word(w);
        spool_word(h);
        spool_word(imgw);
        spool_word(imgh);
        spool_word(channels);
        tx = left;
        ty = top + h + spacing;
        return fault;

    case PRS_TRANSFER:
        if (fault)
            return fault;
        if (!raster || row >= imgh) {
            fault = PDERR_CANCEL;
            return fault;
        }
        for (i = 0; i < imgw; i++) {
            union colorEntry *c = &((struct PrtInfo *)ct)->pi_ColorInt[i];

            if (channels == 3) {
                raster[3 * i + 0] = c->colorByte[PCMRED];
                raster[3 * i + 1] = c->colorByte[PCMGREEN];
                raster[3 * i + 2] = c->colorByte[PCMBLUE];
            } else {
                UBYTE v = (77UL * c->colorByte[PCMRED] +
                           150UL * c->colorByte[PCMGREEN] +
                           29UL * c->colorByte[PCMBLUE] + 128) / 256;

                if (prefs->PrintShade == SHADE_BW)
                    v = v >= (prefs->PrintThreshold & 15) * 17 ? 255 : 0;
                raster[i] = v;
            }
        }
        spool_bytes(raster, imgw * channels);
        row++;
        return fault;

    case PRS_CLOSE:
        if (ct)
            fault = ct;
        else if (raster && row != imgh)
            fault = PDERR_CANCEL;
        FreeVec(raster);
        raster = NULL;
        if (!(x & SPECIAL_NOFORMFEED))
            end_page();
        return fault;

    case PRS_FLUSH:
    case PRS_CLEAR:
        return fault;
    }

    return 0;
}
