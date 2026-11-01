/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    printer.device unit 1 with a preferences file that names device unit 0:
    closing it must release the lock of unit 1, so another task can open
    unit 1 afterwards, and PRD_SETJOBINFO must be accepted. Unit 2 gets
    the same file with the paper size as older Prefs/Printer saved it,
    already shifted into its intuition/preferences.h position; the driver
    must take both. Existing prefs files of the two units are put back.
*/

#include <exec/io.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <devices/printer.h>
#include <devices/prtbase.h>
#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/printertxt.h>
#include <prefs/printergfx.h>
#include <intuition/preferences.h>
#include <utility/tagitem.h>
#include <aros/macros.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <string.h>

#define UNIT       1
#define OLDUNIT    2

static const char *prefs_file[] = { NULL, "ENV:SYS/printer1.prefs", "ENV:SYS/printer2.prefs" };
static const char *saved_file[] = { NULL, "T:unitlock-printer1.prefs", "T:unitlock-printer2.prefs" };
static BOOL had_file[3];

static struct Task *parent;
static volatile LONG child_result;
static volatile BOOL child_done;

static LONG open_unit(LONG unit, struct IOStdReq **iop, struct MsgPort **portp)
{
    *portp = CreateMsgPort();
    *iop = *portp ? (struct IOStdReq *)CreateIORequest(*portp, sizeof(struct IODRPReq)) : NULL;
    if (!*iop)
        return -1;
    return OpenDevice((CONST_STRPTR)"printer.device", unit, (struct IORequest *)*iop, 0);
}

static void close_unit(struct IOStdReq *io, struct MsgPort *port, BOOL opened)
{
    if (opened)
        CloseDevice((struct IORequest *)io);
    if (io)
        DeleteIORequest((struct IORequest *)io);
    if (port)
        DeleteMsgPort(port);
}

static LONG child(void)
{
    struct IOStdReq *io;
    struct MsgPort *port;
    LONG err = open_unit(UNIT, &io, &port);

    close_unit(io, port, err == 0);
    child_result = err;
    child_done = TRUE;
    Signal(parent, SIGBREAKF_CTRL_E);
    return 0;
}

static BOOL write_chunk(struct IFFHandle *iff, LONG id, APTR data, ULONG size)
{
    if (PushChunk(iff, ID_PREF, id, size))
        return FALSE;
    if (WriteChunkBytes(iff, data, size) != (LONG)size)
        return FALSE;
    return PopChunk(iff) == 0;
}

static BOOL write_prefs(LONG unitnum, UWORD papersize)
{
    struct PrefHeader hdr = { PHV_CURRENT, 0, 0 };
    struct PrinterTxtPrefs txt = { { 0 } };
    struct PrinterUnitPrefs unit = { { 0 } };
    struct PrinterDeviceUnitPrefs devunit = { { 0 } };
    struct PrinterGfxPrefs gfx = { { 0 } };
    struct IFFHandle *iff;
    BOOL ok = FALSE;

    strcpy((char *)txt.pt_Driver, "PostScript");
    txt.pt_Port = PP_SERIAL;
    txt.pt_PaperType = PT_SINGLE;
    txt.pt_PaperSize = papersize;
    txt.pt_PaperLength = 66;
    txt.pt_Pitch = PP_PICA;
    txt.pt_Spacing = PS_SIX_LPI;
    txt.pt_RightMargin = 80;
    txt.pt_Quality = PQ_DRAFT;
    /* no device name: the serial port, as pt_Port says */
    devunit.pd_UnitNum = 0;                     /* not the unit being opened */
    strcpy((char *)devunit.pd_UnitName, "unitlock");

    /* the file is big-endian, as Prefs/Printer writes it */
    txt.pt_PaperType = AROS_WORD2BE(txt.pt_PaperType);
    txt.pt_PaperSize = AROS_WORD2BE(txt.pt_PaperSize);
    txt.pt_PaperLength = AROS_WORD2BE(txt.pt_PaperLength);
    txt.pt_Pitch = AROS_WORD2BE(txt.pt_Pitch);
    txt.pt_Spacing = AROS_WORD2BE(txt.pt_Spacing);
    txt.pt_RightMargin = AROS_WORD2BE(txt.pt_RightMargin);
    txt.pt_Quality = AROS_WORD2BE(txt.pt_Quality);
    devunit.pd_UnitNum = AROS_LONG2BE(devunit.pd_UnitNum);

    if (!(iff = AllocIFF()))
        return FALSE;
    if ((iff->iff_Stream = (IPTR)Open((CONST_STRPTR)prefs_file[unitnum], MODE_NEWFILE)))
    {
        InitIFFasDOS(iff);
        if (!OpenIFF(iff, IFFF_WRITE))
        {
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

/* copy src to dst; FALSE if src does not exist */
static BOOL copy_file(const char *src, const char *dst)
{
    BPTR in, out;
    UBYTE buf[1024];
    LONG n;

    if (!(in = Open((CONST_STRPTR)src, MODE_OLDFILE)))
        return FALSE;
    if ((out = Open((CONST_STRPTR)dst, MODE_NEWFILE)))
    {
        while ((n = Read(in, buf, sizeof(buf))) > 0)
            Write(out, buf, n);
        Close(out);
    }
    Close(in);
    return TRUE;
}

static void save_prefs(LONG unit)
{
    had_file[unit] = copy_file(prefs_file[unit], saved_file[unit]);
}

static void restore_prefs(LONG unit)
{
    if (had_file[unit])
    {
        copy_file(saved_file[unit], prefs_file[unit]);
        DeleteFile((CONST_STRPTR)saved_file[unit]);
    }
    else
        DeleteFile((CONST_STRPTR)prefs_file[unit]);
}

int main(void)
{
    struct IOStdReq *io = NULL;
    struct MsgPort *port = NULL;
    LONG err, failures = 0, waited;

    save_prefs(UNIT);
    save_prefs(OLDUNIT);
    if (!write_prefs(UNIT, PS_EURO_A4) || !write_prefs(OLDUNIT, EURO_A4))
    {
        Printf("FAIL: cannot write the preferences files\n");
        restore_prefs(UNIT);
        restore_prefs(OLDUNIT);
        return RETURN_FAIL;
    }

    err = open_unit(UNIT, &io, &port);
    if (err)
    {
        Printf("FAIL: cannot open printer.device unit %ld (%ld)\n", (IPTR)UNIT, (IPTR)err);
        failures++;
    }
    else
        Printf("PASS: opened printer.device unit %ld\n", (IPTR)UNIT);
    close_unit(io, port, err == 0);

    /* the paper size as the AROS editor saved it before: A4 as 0x90 */
    err = open_unit(OLDUNIT, &io, &port);
    if (err)
    {
        Printf("FAIL: cannot open unit %ld with the old paper size encoding (%ld)\n", (IPTR)OLDUNIT, (IPTR)err);
        failures++;
    }
    else
        Printf("PASS: opened unit %ld with the old paper size encoding\n", (IPTR)OLDUNIT);
    close_unit(io, port, err == 0);

    /* another task opens the unit after the first closed it */
    parent = FindTask(NULL);
    SetSignal(0, SIGBREAKF_CTRL_E);
    if (CreateNewProcTags(NP_Entry, (IPTR)child, NP_Name, (IPTR)"unitlock child", TAG_DONE))
    {
        for (waited = 0; waited < 100 && !child_done; waited++)
        {
            if (SetSignal(0, SIGBREAKF_CTRL_E) & SIGBREAKF_CTRL_E)
                break;
            Delay(5);
        }
        if (child_done && child_result == 0)
            Printf("PASS: a second task could open unit %ld after it was closed\n", (IPTR)UNIT);
        else if (!child_done)
        {
            Printf("FAIL: a second task is still blocked opening unit %ld after 10 seconds\n", (IPTR)UNIT);
            failures++;
        }
        else
        {
            Printf("FAIL: the second task could not open unit %ld (%ld)\n", (IPTR)UNIT, (IPTR)child_result);
            failures++;
        }
    }
    else
    {
        Printf("FAIL: cannot start the second task\n");
        failures++;
    }

#ifdef PRD_SETJOBINFO
    if (child_done && open_unit(UNIT, &io, &port) == 0)
    {
        struct TagItem tags[] = {
            { PJIA_Application, (IPTR)"unitlock" },
            { PJIA_Document,    (IPTR)"unit lock test" },
            { TAG_DONE, 0 }
        };
        io->io_Command = PRD_SETJOBINFO;
        io->io_Data = tags;
        io->io_Length = 0;
        io->io_Flags = 0;
        DoIO((struct IORequest *)io);
        if (io->io_Error == 0)
            Printf("PASS: PRD_SETJOBINFO accepted\n");
        else
        {
            Printf("FAIL: PRD_SETJOBINFO error %ld\n", (IPTR)io->io_Error);
            failures++;
        }
        close_unit(io, port, TRUE);
    }
#endif

    restore_prefs(UNIT);
    restore_prefs(OLDUNIT);
    Printf("UNITLOCK %s\n", (IPTR)(failures ? "FAIL" : "PASS"));
    return failures ? RETURN_FAIL : RETURN_OK;
}
