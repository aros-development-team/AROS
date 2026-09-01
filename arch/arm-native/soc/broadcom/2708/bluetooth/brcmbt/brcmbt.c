/*
 *----------------------------------------------------------------------------
 *   brcmbt.fwl - Broadcom Bluetooth firmware (patchram) loader
 *----------------------------------------------------------------------------
 *
 * The log callback is bluetooth.library's, and it formats with vsnprintf --
 * C semantics, not RawDoFmt's. A 32-bit AROS LONG is not a `long` on a
 * 64-bit target, so the format strings below say %d and cast, rather than
 * the %ld an Amiga-style formatter would want.
 *
 * A pluggable BtFirmwareLoader for Broadcom controllers, which is what the
 * Raspberry Pi 3's onboard BCM43438 is. Modelled on AROS's Realtek loaders in
 * rom/bluetooth/firmware; the stack calls fwl_Match/fwl_Load during a
 * controller's bring-up, and blobs are read from disk only.
 *
 * Why this exists: a BCM43438 runs from ROM until it is sent a patchram image.
 * It answers HCI_Reset and reports its version without one -- which is how the
 * bring-up gets as far as it does today -- but reports a placeholder
 * BD_ADDR of AA:AA:AA:AA:AA:AA, and pairing needs a real address.
 *
 * An alternative would be to convert the .hcd to a C source
 * at build time (convert_hcd.py from BTstack) and compiling it into the
 * binary. This does it the way AROS wants instead: the .hcd stays a file on
 * the card, and swapping controllers is swapping a file rather than a rebuild.
 */

#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <exec/exec.h>
#include <exec/types.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bluetooth.h>

#include "brcmbt.h"

#include LC_LIBDEFS_FILE

/* /// "brcmFwPath()" */
/*
 * Which file to load.
 *
 * Linux names these by chip: BCM43430A1.hcd is the Pi 3 and Pi Zero W,
 * BCM4345C0.hcd the Pi 3B+ and Pi 4. LMP subversion identifies the part, and
 * the mapping below covers what this port can run on; anything else falls
 * back to the generic name so an unknown board can still be given a blob by
 * hand.
 */
static CONST_STRPTR brcmFwName(struct BtFirmwareContext *ctx)
{
    switch (ctx->fwc_LMPSubversion)
    {
    case 0x2209:  return "BCM43430A1.hcd";   /* BCM43430A1 - Pi 3, Zero W */
    case 0x6119:  return "BCM4345C0.hcd";    /* BCM4345C0  - Pi 3B+, Pi 4 */
    case 0x230f:  return "BCM4345C5.hcd";    /* BCM4345C5  - Pi 4B rev, CM4 */
    default:      return "BCM.hcd";
    }
}
/* \\\ */

/* /// "brcmMatch()" */
/*
 * Identity only -- the contract says fwl_Match must not issue HCI.
 *
 * Manufacturer 15 is Broadcom in the SIG company list. Cypress parts report
 * the same id, which is correct: they are the same silicon lineage and take
 * the same patchram format.
 */
static ULONG brcmMatch(struct BtFirmwareLoader *self,
                       struct BtFirmwareContext *ctx)
{
    (void)self;

    if (ctx->fwc_Manufacturer != BT_MANUFACTURER_BROADCOM)
        return 0;

    /* Priority 10: above a generic loader, below anything that recognises a
     * specific part more narrowly than "Broadcom". */
    return 10;
}
/* \\\ */

/* /// "brcmReadFile()" */
static APTR brcmReadFile(struct DosLibrary *DOSBase, CONST_STRPTR path,
                         LONG *sizep)
{
    BPTR fh;
    LONG size;
    APTR buf = NULL;

    if (!(fh = Open((STRPTR)path, MODE_OLDFILE)))
        return NULL;

    if (Seek(fh, 0, OFFSET_END) >= 0)
    {
        size = Seek(fh, 0, OFFSET_BEGINNING);
        if (size > 0 && (buf = AllocVec(size, MEMF_ANY)))
        {
            if (Read(fh, buf, size) == size)
                *sizep = size;
            else
            {
                FreeVec(buf);
                buf = NULL;
            }
        }
    }
    Close(fh);
    return buf;
}
/* \\\ */

/* /// "brcmWaitRestart()" */
/*
 * Launch_RAM was the last record, and the controller restarts on it.
 *
 * The caller resumes the bring-up the moment this returns -- hwtask.c runs
 * bDoFirmware() and then bBringupStep() with nothing in between -- so
 * returning early sends the next HCI command to a chip that is not there.
 * That is not hypothetical: with a fixed 50 ms wait, the step after the
 * firmware timed out on every boot, the bring-up failed, and the radio never
 * reached BHS_READY -- which presents as a Bluetooth prefs with no radio in
 * it, because btEnumerateHardware() only reports controllers in that state.
 *
 * A bigger fixed number would be a better guess and still a guess, so ask the
 * controller instead. HCI_Reset is the smallest command it answers and it is
 * idempotent, so the stack issuing its own immediately afterwards costs
 * nothing. A completed command also proves the H4 stream has resynchronised
 * past the framing errors and zero bytes that a restarting UART peer leaves
 * on the line.
 *
 * Probing is not free -- an unanswered command costs the stack's own command
 * timeout -- so settle first and probe a bounded number of times.
 */
static LONG brcmWaitRestart(struct BtFirmwareContext *ctx)
{
    ULONG attempt;

    for (attempt = 0; attempt < BRCM_RESTART_PROBES; attempt++)
    {
        UBYTE status = 0;

        Delay(BRCM_RESTART_SETTLE_MS * TICKS_PER_SECOND / 1000);

        if (ctx->fwc_HciCommand(ctx, HC_OP_HCI_RESET, NULL, 0,
                                &status, NULL, NULL, 0) == 0 && status == 0)
        {
            return 0;
        }

        ctx->fwc_Log(ctx, RETURN_WARN,
            "brcmbt: controller has not answered %u probe(s) since Launch_RAM",
            (unsigned)(attempt + 1));
    }

    ctx->fwc_Log(ctx, RETURN_ERROR,
        "brcmbt: controller did not come back after Launch_RAM");
    return -1;
}
/* \\\ */

/* /// "brcmLoad()" */
/*
 * The patchram protocol, which is simpler than its reputation:
 *
 *   1. HCI_Download_Minidriver (0xfc2e), then settle;
 *   2. replay every command in the .hcd in order -- they are Write_RAM
 *      records plus a trailing Launch_RAM;
 *   3. the controller restarts on Launch_RAM and needs time before it will
 *      answer again.
 *
 * The file is a flat sequence of [opcode:2][plen:1][params:plen], so parsing
 * it is walking that. Sending each record through fwc_HciCommand gets the
 * stack's own synchronous channel, including its completion handling.
 */
static LONG brcmLoad(struct BtFirmwareLoader *self,
                     struct BtFirmwareContext *ctx)
{
    struct BrcmFwBase *bfw = self->fwl_UserData;
    struct DosLibrary *DOSBase = bfw->bfw_DOSBase;
    CONST_STRPTR name = brcmFwName(ctx);
    UBYTE path[128];
    UBYTE *fw;
    LONG size = 0, offset = 0, records = 0;
    UBYTE status = 0;
    LONG err;

    if (!DOSBase)
        return -1;

    /* BRCM_FW_DIR + name, without needing a sprintf. */
    {
        CONST_STRPTR d = BRCM_FW_DIR;
        ULONG i = 0;

        while (*d && i < sizeof(path) - 1)
            path[i++] = *d++;
        while (*name && i < sizeof(path) - 1)
            path[i++] = *name++;
        path[i] = '\0';
    }

    fw = brcmReadFile(DOSBase, (CONST_STRPTR)path, &size);
    if (!fw)
    {
        /*
         * Not an error. A controller that runs from ROM works without this,
         * with a placeholder address; saying so is more useful than failing
         * the bring-up over a file the user may deliberately not have.
         */
        ctx->fwc_Log(ctx, RETURN_WARN,
            "brcmbt: no %s, continuing without patchram", path);
        return 0;
    }

    ctx->fwc_Log(ctx, RETURN_OK, "brcmbt: %s, %d bytes", path, (int)size);

    err = ctx->fwc_HciCommand(ctx, HC_OP_BRCM_DOWNLOAD_MINIDRV, NULL, 0,
                              &status, NULL, NULL, 0);
    if (err || status)
    {
        ctx->fwc_Log(ctx, RETURN_ERROR,
            "brcmbt: Download_Minidriver failed (err %d, status 0x%02x)",
            (int)err, (unsigned)status);
        FreeVec(fw);
        return -1;
    }

    while (offset + BRCM_HCD_HDR <= size)
    {
        UWORD opcode = fw[offset] | ((UWORD)fw[offset + 1] << 8);
        UBYTE plen   = fw[offset + 2];

        if (offset + BRCM_HCD_HDR + plen > size)
        {
            ctx->fwc_Log(ctx, RETURN_ERROR,
                "brcmbt: truncated record at %d of %d",
                (int)offset, (int)size);
            FreeVec(fw);
            return -1;
        }

        err = ctx->fwc_HciCommand(ctx, opcode, &fw[offset + BRCM_HCD_HDR],
                                  plen, &status, NULL, NULL, 0);
        if (err || status)
        {
            ctx->fwc_Log(ctx, RETURN_ERROR,
                "brcmbt: record %d (opcode 0x%04x) failed"
                " (err %d, status 0x%02x)",
                (int)records, (unsigned)opcode, (int)err, (unsigned)status);
            FreeVec(fw);
            return -1;
        }

        offset += BRCM_HCD_HDR + plen;
        records++;
    }

    FreeVec(fw);
    ctx->fwc_Log(ctx, RETURN_OK,
        "brcmbt: %d records applied; controller restarting", (int)records);

    return brcmWaitRestart(ctx);
}
/* \\\ */

/* /// "libInit()" */
static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR bfw)
{
    struct Library *BluetoothBase;

    bfw->bfw_DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    if (!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        if (bfw->bfw_DOSBase)
            CloseLibrary((struct Library *)bfw->bfw_DOSBase);
        return FALSE;
    }
    bfw->bfw_BluetoothBase = BluetoothBase;

    bfw->bfw_Loader.fwl_Name     = "Broadcom (patchram)";
    bfw->bfw_Loader.fwl_Match    = brcmMatch;
    bfw->bfw_Loader.fwl_Load     = brcmLoad;
    bfw->bfw_Loader.fwl_UserData = bfw;
    btAddFirmwareLoader(&bfw->bfw_Loader);
    return TRUE;
}
/* \\\ */

/* /// "libExpunge()" */
static int GM_UNIQUENAME(libExpunge)(LIBBASETYPEPTR bfw)
{
    struct Library *BluetoothBase = bfw->bfw_BluetoothBase;

    if (BluetoothBase)
    {
        btRemFirmwareLoader(&bfw->bfw_Loader);
        CloseLibrary(BluetoothBase);
    }
    if (bfw->bfw_DOSBase)
        CloseLibrary((struct Library *)bfw->bfw_DOSBase);
    return TRUE;
}
/* \\\ */

ADD2INITLIB(GM_UNIQUENAME(libInit), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(libExpunge), 0)
