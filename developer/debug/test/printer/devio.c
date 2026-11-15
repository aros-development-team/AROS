/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    printer.device against an output device of our own, "prtsink.device",
    which finishes every write a moment after it was started, keeps what it
    was sent and fails a write when told to:

    - a unit can be opened more than once;
    - text arrives in the order it was written, 8-bit characters included,
      and whole, though the output device is slow;
    - PRD_PRTCOMMAND is carried out;
    - an error of the output device is the error of the request that caused
      it, is shown through the PRD_SETERRHOOK hook, and is gone afterwards;
    - CMD_STOP holds requests until CMD_START, and AbortIO() takes one out;
    - graphic dumps of planar bitmaps go through the request's colour map,
      Extra Half-Brite and HAM included, whether the bitmap is hand-made
      or one of graphics.library's;
    - a TurboPrint dump of RGB pixels keeps red, green and blue apart.

    The unit's preferences file is put back afterwards.
*/

#include <exec/io.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <exec/devices.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <devices/printer.h>
#include <devices/prtbase.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <graphics/rastport.h>
#include <graphics/modeid.h>
#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/printertxt.h>
#include <prefs/printergfx.h>
#include <intuition/preferences.h>
#include <utility/hooks.h>
#include <aros/macros.h>
#include <aros/asmcall.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/iffparse.h>

#include <string.h>

#include <turboprint.h>

#define UNIT        3
#define SINKNAME    "prtsink.device"
#define CAPTURE     (256 * 1024)

static const char prefs_file[] = "ENV:SYS/printer3.prefs";
static const char saved_file[] = "T:devio-printer3.prefs";

static LONG failures;

static void check(BOOL ok, CONST_STRPTR what)
{
    Printf("%s: %s\n", ok ? (IPTR)"PASS" : (IPTR)"FAIL", (IPTR)what);
    if (!ok)
        failures++;
}

/*** The output device ******************************************************/

static struct MsgPort *volatile sink_port;      /* the sink task's */
static struct Task *sink_task, *parent;
static UBYTE *capture;
static volatile LONG captured;
static volatile LONG fail_code;                 /* the next write fails with this */
static volatile LONG writes;

static AROS_LH3(void, SinkOpen,
    AROS_LHA(struct IORequest *, io, A1),
    AROS_LHA(IPTR, unit, D0),
    AROS_LHA(ULONG, flags, D1),
    struct Device *, base, 1, Sink)
{
    AROS_LIBFUNC_INIT

    base->dd_Library.lib_OpenCnt++;
    io->io_Error = 0;
    io->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

    AROS_LIBFUNC_EXIT
}

static AROS_LH1(BPTR, SinkClose,
    AROS_LHA(struct IORequest *, io, A1),
    struct Device *, base, 2, Sink)
{
    AROS_LIBFUNC_INIT

    base->dd_Library.lib_OpenCnt--;
    return BNULL;

    AROS_LIBFUNC_EXIT
}

static AROS_LH1(BPTR, SinkExpunge,
    AROS_LHA(struct Device *, extra, D0),
    struct Device *, base, 3, Sink)
{
    AROS_LIBFUNC_INIT
    return BNULL;
    AROS_LIBFUNC_EXIT
}

static AROS_LH1(void, SinkBeginIO,
    AROS_LHA(struct IORequest *, io, A1),
    struct Device *, base, 5, Sink)
{
    AROS_LIBFUNC_INIT

    /* never done at once: the caller's buffer has to last */
    io->io_Flags &= ~IOF_QUICK;
    PutMsg(sink_port, &io->io_Message);

    AROS_LIBFUNC_EXIT
}

static AROS_LH1(LONG, SinkAbortIO,
    AROS_LHA(struct IORequest *, io, A1),
    struct Device *, base, 6, Sink)
{
    AROS_LIBFUNC_INIT
    return IOERR_NOCMD;
    AROS_LIBFUNC_EXIT
}

static LONG sink_main(void)
{
    struct MsgPort *port = CreateMsgPort();
    struct IOStdReq *io;

    sink_port = port;
    Signal(parent, SIGBREAKF_CTRL_E);
    if (port == NULL)
        return 0;

    for (;;) {
        ULONG sigs = Wait((1UL << port->mp_SigBit) | SIGBREAKF_CTRL_C);

        while ((io = (struct IOStdReq *)GetMsg(port))) {
            io->io_Error = 0;
            if (io->io_Command == CMD_WRITE) {
                /* the data is looked at late, as a real port would */
                Delay(1);
                writes++;
                if (fail_code) {
                    io->io_Error = fail_code;
                    io->io_Actual = 0;
                    fail_code = 0;
                } else {
                    LONG len = io->io_Length;

                    if (len > CAPTURE - 1 - captured)
                        len = CAPTURE - 1 - captured;
                    CopyMem(io->io_Data, capture + captured, len);
                    captured += len;
                    capture[captured] = 0;
                    io->io_Actual = io->io_Length;
                }
            } else
                io->io_Error = IOERR_NOCMD;
            ReplyMsg(&io->io_Message);
        }
        if (sigs & SIGBREAKF_CTRL_C)
            break;
    }
    DeleteMsgPort(port);
    sink_port = NULL;
    Signal(parent, SIGBREAKF_CTRL_E);
    return 0;
}

static struct Device *sink_create(void)
{
    APTR funcs[] = {
        AROS_SLIB_ENTRY(SinkOpen, Sink, 1),
        AROS_SLIB_ENTRY(SinkClose, Sink, 2),
        AROS_SLIB_ENTRY(SinkExpunge, Sink, 3),
        NULL,
        AROS_SLIB_ENTRY(SinkBeginIO, Sink, 5),
        AROS_SLIB_ENTRY(SinkAbortIO, Sink, 6),
        (APTR)-1,
    };
    struct Device *dev;

    if (!(capture = AllocMem(CAPTURE, MEMF_ANY | MEMF_CLEAR)))
        return NULL;
    parent = FindTask(NULL);
    SetSignal(0, SIGBREAKF_CTRL_E);
    sink_task = (struct Task *)CreateNewProcTags(NP_Entry, (IPTR)sink_main,
                                                 NP_Name, (IPTR)"prtsink", TAG_DONE);
    if (sink_task == NULL)
        return NULL;
    Wait(SIGBREAKF_CTRL_E);
    if (sink_port == NULL)
        return NULL;

    dev = (struct Device *)MakeLibrary(funcs, NULL, NULL, sizeof(struct Device), BNULL);
    if (dev == NULL)
        return NULL;
    dev->dd_Library.lib_Node.ln_Type = NT_DEVICE;
    dev->dd_Library.lib_Node.ln_Name = SINKNAME;
    dev->dd_Library.lib_IdString = SINKNAME;
    dev->dd_Library.lib_Version = 1;
    AddDevice(dev);
    return dev;
}

static void sink_delete(struct Device *dev)
{
    if (dev) {
        Forbid();
        Remove(&dev->dd_Library.lib_Node);
        Permit();
        FreeMem((UBYTE *)dev - dev->dd_Library.lib_NegSize,
                dev->dd_Library.lib_NegSize + dev->dd_Library.lib_PosSize);
    }
    if (sink_port) {
        SetSignal(0, SIGBREAKF_CTRL_E);
        Signal(sink_task, SIGBREAKF_CTRL_C);
        Wait(SIGBREAKF_CTRL_E);
    }
    if (capture)
        FreeMem(capture, CAPTURE);
}

/* where in what the printer got since from, or -1 */
static LONG find(LONG from, CONST_STRPTR text)
{
    char *at;

    if (from < 0 || from > captured)
        return -1;
    at = strstr((char *)capture + from, (const char *)text);
    return at ? (LONG)((UBYTE *)at - capture) : -1;
}

/*** The preferences ********************************************************/

static BOOL write_chunk(struct IFFHandle *iff, LONG id, APTR data, ULONG size)
{
    if (PushChunk(iff, ID_PREF, id, size))
        return FALSE;
    if (WriteChunkBytes(iff, data, size) != (LONG)size)
        return FALSE;
    return PopChunk(iff) == 0;
}

static BOOL write_prefs(void)
{
    struct PrefHeader hdr = { PHV_CURRENT, 0, 0 };
    struct PrinterTxtPrefs txt = { { 0 } };
    struct PrinterUnitPrefs unit = { { 0 } };
    struct PrinterDeviceUnitPrefs devunit = { { 0 } };
    struct PrinterGfxPrefs gfx = { { 0 } };
    struct IFFHandle *iff;
    BOOL ok = FALSE;

    strcpy((char *)txt.pt_Driver, "PostScript");
    txt.pt_Port = PP_PARALLEL;
    txt.pt_PaperType = AROS_WORD2BE(PT_SINGLE);
    txt.pt_PaperSize = AROS_WORD2BE(PS_EURO_A4);
    txt.pt_PaperLength = AROS_WORD2BE(66);
    txt.pt_Pitch = AROS_WORD2BE(PP_PICA);
    txt.pt_Spacing = AROS_WORD2BE(PS_SIX_LPI);
    txt.pt_RightMargin = AROS_WORD2BE(80);
    txt.pt_Quality = AROS_WORD2BE(PQ_DRAFT);
    strcpy((char *)unit.pu_DeviceName, SINKNAME);
    devunit.pd_UnitNum = AROS_LONG2BE(UNIT);
    strcpy((char *)devunit.pd_UnitName, "devio");
    gfx.pg_Shade = AROS_WORD2BE(PS_COLOR);

    if (!(iff = AllocIFF()))
        return FALSE;
    if ((iff->iff_Stream = (IPTR)Open((CONST_STRPTR)prefs_file, MODE_NEWFILE))) {
        InitIFFasDOS(iff);
        if (!OpenIFF(iff, IFFF_WRITE)) {
            ok = !PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN) &&
                write_chunk(iff, ID_PRHD, &hdr, sizeof(hdr)) &&
                write_chunk(iff, ID_PTXT, &txt, sizeof(txt)) &&
                write_chunk(iff, ID_PUNT, &unit, sizeof(unit)) &&
                write_chunk(iff, ID_PDEV, &devunit, sizeof(devunit)) &&
                write_chunk(iff, ID_PGFX, &gfx, sizeof(gfx)) &&
                !PopChunk(iff);
            CloseIFF(iff);
        }
        Close((BPTR)iff->iff_Stream);
    }
    FreeIFF(iff);
    return ok;
}

static BOOL copy_file(const char *src, const char *dst)
{
    BPTR in, out;
    UBYTE buf[1024];
    LONG n;

    if (!(in = Open((CONST_STRPTR)src, MODE_OLDFILE)))
        return FALSE;
    if ((out = Open((CONST_STRPTR)dst, MODE_NEWFILE))) {
        while ((n = Read(in, buf, sizeof(buf))) > 0)
            Write(out, buf, n);
        Close(out);
    }
    Close(in);
    return TRUE;
}

/*** The requests ***********************************************************/

struct Opener {
    struct MsgPort *port;
    union {
        struct IOStdReq std;
        struct IOPrtCmdReq cmd;
        struct IODRPReq drp;
        struct IOPrtErrReq err;
    } *io;
    LONG error;
};

static BOOL opener_open(struct Opener *o)
{
    o->port = CreateMsgPort();
    o->io = o->port ? (APTR)CreateIORequest(o->port, sizeof(*o->io)) : NULL;
    o->error = o->io ? OpenDevice((CONST_STRPTR)"printer.device", UNIT, (struct IORequest *)o->io, 0) : -1;
    return o->error == 0;
}

static void opener_close(struct Opener *o)
{
    if (o->io && o->error == 0)
        CloseDevice((struct IORequest *)o->io);
    if (o->io)
        DeleteIORequest((struct IORequest *)o->io);
    if (o->port)
        DeleteMsgPort(o->port);
    o->io = NULL;
    o->port = NULL;
}

static LONG do_write(struct Opener *o, UWORD command, CONST_STRPTR text)
{
    o->io->std.io_Command = command;
    o->io->std.io_Data = (APTR)text;
    o->io->std.io_Length = strlen((const char *)text);
    o->io->std.io_Flags = 0;
    return DoIO((struct IORequest *)o->io);
}

static LONG do_simple(struct Opener *o, UWORD command)
{
    o->io->std.io_Command = command;
    o->io->std.io_Data = NULL;
    o->io->std.io_Length = 0;
    o->io->std.io_Flags = 0;
    return DoIO((struct IORequest *)o->io);
}

/*** The error hook *********************************************************/

static volatile LONG hook_calls;
static volatile ULONG hook_version;
static volatile BOOL hook_message;

AROS_UFH3(static IPTR, err_hook,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, object, A2),
    AROS_UFHA(struct PrtErrMsg *, pem, A1))
{
    AROS_USERFUNC_INIT

    hook_calls++;
    hook_version = pem->pe_Version;
    hook_message = pem->pe_ES != NULL && pem->pe_ES->es_TextFormat != NULL && pem->pe_ArgList != NULL;
    return 0;

    AROS_USERFUNC_EXIT
}

/*** Graphic dumps **********************************************************/

#define DUMPW   16
#define DUMPH   2

static void set_pen(struct BitMap *bm, LONG x, LONG y, UBYTE pen)
{
    LONG plane;

    for (plane = 0; plane < bm->Depth; plane++) {
        UBYTE *byte = bm->Planes[plane] + y * bm->BytesPerRow + (x >> 3);
        UBYTE bit = 0x80 >> (x & 7);

        if (pen & (1 << plane))
            *byte |= bit;
        else
            *byte &= ~bit;
    }
}

/* A hand-made planar bitmap, as an AmigaOS program would pass: every row
   holds the given pens, then pen 0. */
static LONG dump_pens(struct Opener *o, LONG depth, ULONG modes, struct ColorMap *cm,
    const UBYTE *pens, LONG count)
{
    struct BitMap bm;
    struct RastPort rp;
    LONG plane, x, y, err = -1;
    BOOL ok = TRUE;

    InitBitMap(&bm, depth, DUMPW, DUMPH);
    for (plane = 0; plane < depth; plane++)
        if (!(bm.Planes[plane] = AllocRaster(DUMPW, DUMPH)))
            ok = FALSE;
    if (ok) {
        for (y = 0; y < DUMPH; y++)
            for (x = 0; x < DUMPW; x++)
                set_pen(&bm, x, y, x < count ? pens[x] : 0);
        InitRastPort(&rp);
        rp.BitMap = &bm;

        o->io->drp.io_Command = PRD_DUMPRPORT;
        o->io->drp.io_Flags = 0;
        o->io->drp.io_RastPort = &rp;
        o->io->drp.io_ColorMap = cm;
        o->io->drp.io_Modes = modes;
        o->io->drp.io_SrcX = 0;
        o->io->drp.io_SrcY = 0;
        o->io->drp.io_SrcWidth = DUMPW;
        o->io->drp.io_SrcHeight = DUMPH;
        o->io->drp.io_DestCols = 0;
        o->io->drp.io_DestRows = 0;
        o->io->drp.io_Special = SPECIAL_NOFORMFEED;
        err = DoIO((struct IORequest *)o->io);
    }
    for (plane = 0; plane < depth; plane++)
        if (bm.Planes[plane])
            FreeRaster(bm.Planes[plane], DUMPW, DUMPH);
    return err;
}

static void dump_request(struct Opener *o, UWORD command, struct RastPort *rp,
    struct ColorMap *cm, IPTR modes)
{
    o->io->drp.io_Command = command;
    o->io->drp.io_Flags = 0;
    o->io->drp.io_RastPort = rp;
    o->io->drp.io_ColorMap = cm;
    o->io->drp.io_Modes = modes;
    o->io->drp.io_SrcX = 0;
    o->io->drp.io_SrcY = 0;
    o->io->drp.io_SrcWidth = DUMPW;
    o->io->drp.io_SrcHeight = DUMPH;
    o->io->drp.io_DestCols = 0;
    o->io->drp.io_DestRows = 0;
    o->io->drp.io_Special = SPECIAL_NOFORMFEED;
}

/* The same pens in a bitmap of graphics.library's own */
static LONG dump_system_bitmap(struct Opener *o, struct ColorMap *cm, const UBYTE *pens, LONG count)
{
    struct BitMap *bm = AllocBitMap(DUMPW, DUMPH, 2, BMF_CLEAR, NULL);
    struct RastPort rp;
    LONG x, y, err;

    if (bm == NULL)
        return -1;
    InitRastPort(&rp);
    rp.BitMap = bm;
    for (y = 0; y < DUMPH; y++)
        for (x = 0; x < count; x++) {
            SetAPen(&rp, pens[x]);
            WritePixel(&rp, x, y);
        }
    dump_request(o, PRD_DUMPRPORT, &rp, cm, INVALID_ID);
    err = DoIO((struct IORequest *)o->io);
    FreeBitMap(bm);
    return err;
}

/* TurboPrint's extended dump: the "bitmap" is rows of R, G, B bytes */
static LONG dump_turboprint(struct Opener *o)
{
    static UBYTE pixels[DUMPH][DUMPW * 3];
    struct TPExtIODRP tp = { 1, 1, TPFMT_RGB24 };
    struct BitMap bm;
    struct RastPort rp;
    LONG y;

    memset(pixels, 0xff, sizeof(pixels));
    for (y = 0; y < DUMPH; y++) {
        pixels[y][0] = 0xff; pixels[y][1] = 0x00; pixels[y][2] = 0x80;
        pixels[y][3] = 0x10; pixels[y][4] = 0x20; pixels[y][5] = 0x30;
    }
    memset(&bm, 0, sizeof(bm));
    bm.BytesPerRow = DUMPW * 3;
    bm.Rows = DUMPH;
    bm.Depth = 24;
    bm.Planes[0] = (PLANEPTR)pixels;
    InitRastPort(&rp);
    rp.BitMap = &bm;
    dump_request(o, PRD_TPEXTDUMPRPORT, &rp, NULL, (IPTR)&tp);
    return DoIO((struct IORequest *)o->io);
}

/****************************************************************************/

int main(void)
{
    struct Device *sink;
    struct Opener a = { 0 }, b = { 0 }, c = { 0 };
    struct ColorMap *cm;
    struct Hook hook = { { 0 } };
    BOOL had_prefs;
    LONG err, mark, p1, p2, p3;

    had_prefs = copy_file(prefs_file, saved_file);
    sink = sink_create();
    if (sink == NULL || !write_prefs()) {
        Printf("FAIL: cannot set up the output device and the preferences\n");
        sink_delete(sink);
        return RETURN_FAIL;
    }

    /* Shared units */
    check(opener_open(&a), "first open of the unit");
    check(opener_open(&b), "second open of the same unit");
    if (a.error || b.error)
        goto done;

    /* Text: in order, and all of it though the device is slow */
    mark = captured;
    err = do_write(&a, CMD_WRITE, "abc\351def\n");
    check(err == 0, "CMD_WRITE");
    p1 = find(mark, "abc");
    p2 = find(p1, "def");
    check(p1 >= 0 && p2 > p1, "the text reached the output device before the reply");
    /* the driver prints an 8-bit character through its ped_8BitChars table */
    check(p1 >= 0 && p2 > p1 + 3, "an 8-bit character stays where it was written");

    mark = captured;
    err = do_write(&b, PRD_RAWWRITE, "%rawwrite-marker\n");
    check(err == 0 && find(mark, "%rawwrite-marker") >= 0, "PRD_RAWWRITE by the second opener");

    /* PRD_PRTCOMMAND */
    a.io->cmd.io_Command = PRD_PRTCOMMAND;
    a.io->cmd.io_Flags = 0;
    a.io->cmd.io_PrtCommand = aSGR1;
    a.io->cmd.io_Parm0 = a.io->cmd.io_Parm1 = a.io->cmd.io_Parm2 = a.io->cmd.io_Parm3 = 0;
    err = DoIO((struct IORequest *)a.io);
    check(err != IOERR_NOCMD, "PRD_PRTCOMMAND is a command of the device");
    a.io->cmd.io_Command = PRD_PRTCOMMAND;
    a.io->cmd.io_PrtCommand = aRAW + 100;
    err = DoIO((struct IORequest *)a.io);
    check(err == -1, "PRD_PRTCOMMAND refuses a command there is none of");

    /* An error of the output device */
    hook.h_Entry = (HOOKFUNC)err_hook;
    a.io->err.io_Command = PRD_SETERRHOOK;
    a.io->err.io_Flags = 0;
    a.io->err.io_Hook = &hook;
    check(DoIO((struct IORequest *)a.io) == 0, "PRD_SETERRHOOK");

    fail_code = 4;      /* a device's own error number */
    err = do_write(&a, PRD_RAWWRITE, "lost\n");
    check(err == PDERR_CANCEL, "a failed write is the error of the request that wrote");
    check(a.io->std.io_Actual == 0, "a failed write wrote nothing");
    check(hook_calls == 1 && hook_version == PDHOOK_VERSION && hook_message, "the error hook was called with a message");
    fail_code = IOERR_ABORTED;
    err = do_write(&a, CMD_WRITE, "lost too\n");
    check(err == IOERR_ABORTED, "an exec error of the output device is passed on");
    mark = captured;
    err = do_write(&a, PRD_RAWWRITE, "%after-the-error\n");
    check(err == 0 && find(mark, "%after-the-error") >= 0, "the next write goes through");
    a.io->err.io_Command = PRD_SETERRHOOK;
    a.io->err.io_Hook = PDHOOK_NONE;
    DoIO((struct IORequest *)a.io);

    /* CMD_STOP, CMD_START, AbortIO() */
    if (opener_open(&c)) {
        static const char held1[] = "%held-one\n", held2[] = "%held-two\n";

        check(do_simple(&a, CMD_STOP) == 0, "CMD_STOP");
        mark = captured;
        b.io->std.io_Command = PRD_RAWWRITE;
        b.io->std.io_Data = (APTR)held1;
        b.io->std.io_Length = sizeof(held1) - 1;
        b.io->std.io_Flags = 0;
        SendIO((struct IORequest *)b.io);
        c.io->std.io_Command = PRD_RAWWRITE;
        c.io->std.io_Data = (APTR)held2;
        c.io->std.io_Length = sizeof(held2) - 1;
        c.io->std.io_Flags = 0;
        SendIO((struct IORequest *)c.io);
        Delay(25);
        check(CheckIO((struct IORequest *)b.io) == NULL && CheckIO((struct IORequest *)c.io) == NULL &&
              find(mark, "%held") < 0, "writes wait while the unit is stopped");
        AbortIO((struct IORequest *)c.io);
        err = WaitIO((struct IORequest *)c.io);
        check(err == IOERR_ABORTED && CheckIO((struct IORequest *)b.io) == NULL,
              "AbortIO() takes a waiting request out, and only that one");
        check(do_simple(&a, CMD_START) == 0, "CMD_START");
        err = WaitIO((struct IORequest *)b.io);
        check(err == 0 && find(mark, "%held-one") >= 0 && find(mark, "%held-two") < 0,
              "the request that waited is printed after CMD_START");
        opener_close(&c);
    } else
        check(FALSE, "third open of the unit");

    /* Graphic dumps */
    if ((cm = GetColorMap(64))) {
        /* pen 1, and the pens around it */
        static const UBYTE plain[] = { 1, 2, 3 };
        static const UBYTE ehb[] = { 1, 33 };
        /* pen 1; blue to 15; red to 0; green to 8 */
        static const UBYTE ham[] = { 0x01, 0x1f, 0x20, 0x38 };

        SetRGB32CM(cm, 0, 0xffffffff, 0xffffffff, 0xffffffff);
        SetRGB32CM(cm, 1, 0xaaaaaaaa, 0x33333333, 0x11111111);
        SetRGB32CM(cm, 2, 0x22222222, 0xcccccccc, 0x44444444);
        SetRGB32CM(cm, 3, 0x55555555, 0x66666666, 0xdddddddd);

        mark = captured;
        err = dump_pens(&a, 2, INVALID_ID, cm, plain, sizeof(plain));
        if (err)
            Printf("      error %ld\n", (IPTR)err);
        check(err == 0, "PRD_DUMPRPORT of a planar bitmap");
        check(find(mark, "aa331122cc445566dd") >= 0, "its pens are the colours of the request's colour map");

        mark = captured;
        err = dump_pens(&a, 6, EXTRA_HALFBRITE, cm, ehb, sizeof(ehb));
        check(err == 0 && find(mark, "aa3311551908") >= 0, "Extra Half-Brite pens are half as bright");

        mark = captured;
        err = dump_pens(&a, 6, HAM, cm, ham, sizeof(ham));
        check(err == 0 && find(mark, "aa3311aa33ff0033ff0088ff") >= 0, "HAM pixels hold and modify");

        mark = captured;
        err = dump_system_bitmap(&a, cm, plain, sizeof(plain));
        check(err == 0 && find(mark, "aa331122cc445566dd") >= 0,
              "a bitmap of graphics.library's is printed in the request's colours too");

        FreeColorMap(cm);
    } else
        check(FALSE, "a colour map for the dumps");

    mark = captured;
    err = dump_turboprint(&a);
    if (err)
        Printf("      error %ld\n", (IPTR)err);
    check(err == 0 && find(mark, "ff0080102030") >= 0, "a TurboPrint RGB dump keeps red, green and blue apart");

    /* The job is the unit's: it goes on when one of its openers closes */
    opener_close(&a);
    mark = captured;
    err = do_write(&b, PRD_RAWWRITE, "%second-opener-alone\n");
    check(err == 0 && find(mark, "%second-opener-alone") >= 0, "the unit works on after one of two closes");
    p3 = writes;
    opener_close(&b);
    check(opener_open(&a), "the unit opens again after its last close");
    (void)p3;

done:
    opener_close(&c);
    opener_close(&b);
    opener_close(&a);

    {
        BPTR out = Open((CONST_STRPTR)"T:devio-output.ps", MODE_NEWFILE);

        if (out) {
            Write(out, capture, captured);
            Close(out);
        }
    }
    sink_delete(sink);
    if (had_prefs) {
        copy_file(saved_file, prefs_file);
        DeleteFile((CONST_STRPTR)saved_file);
    } else
        DeleteFile((CONST_STRPTR)prefs_file);

    Printf("%s: %ld failure(s)\n", failures ? (IPTR)"FAILED" : (IPTR)"ALL PASSED", (IPTR)failures);
    return failures ? RETURN_FAIL : RETURN_OK;
}
