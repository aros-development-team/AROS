/*
 *----------------------------------------------------------------------------
 *       bluetooth.library: pluggable controller-firmware loaders
 *----------------------------------------------------------------------------
 *
 * Some radios (notably Realtek RTL8761/8852) answer HCI from a ROM bootloader
 * but keep the RF dead until the host downloads a vendor firmware patch. That
 * support does NOT live in bluetooth.library: it lives in loadable modules in
 * DEVS:Bluetooth/FWLoaders/ that BTStackLoader opens and which register a
 * "struct BtFirmwareLoader" here with btAddFirmwareLoader().
 *
 * During a controller's bring-up the hwtask reaches HCB_FIRMWARE and calls
 * bDoFirmware(): the best-matching loader is offered the controller together
 * with a synchronous HCI channel (bHciDoSync, wrapped as fwc_HciCommand) and a
 * log callback (fwc_Log). Matching is identity-only (manufacturer / LMP
 * subversion / HCI revision) and must not issue HCI; the load reads whatever it
 * needs over fwc_HciCommand. Firmware BLOBs are read from disk only.
 *
 * A controller can also come up before its loader binds (hot-plug races the
 * stack loader). btAddFirmwareLoader() therefore re-checks controllers that are
 * already present and signals the hwtask of any still-unpatched match, so the
 * download happens then. bth_Flags BTHF_FWLOADED marks a controller whose
 * firmware has been handled so it is never downloaded twice.
 */

#include "debug.h"

#include "bluetooth.library.h"
#include "hwtask.h"

#include <proto/exec.h>
#include <proto/dos.h>

#include <libraries/bluetooth.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define DOSBase BluetoothBase->bt_DosBase

/* /// "bFwHciCommand()" */
/* BtFirmwareContext.fwc_HciCommand: route a synchronous HCI command from a
   firmware loader to the hwtask that owns the context. */
static LONG bFwHciCommand(struct BtFirmwareContext *ctx, UWORD opcode, CONST_APTR params, UWORD plen,
                          UBYTE *status, UBYTE *resp, UWORD *resplen, UWORD respmax)
{
    struct BtHWCore *hc = ctx->fwc_Private;

    return bHciDoSync(hc, opcode, params, plen, status, resp, resplen, respmax);
}
/* \\\ */

/* /// "bFwLog()" */
/* BtFirmwareContext.fwc_Log: append a formatted line (tagged with the device)
   to the bluetooth message log. */
static void bFwLog(struct BtFirmwareContext *ctx, LONG level, CONST_STRPTR fmt, ...)
{
    struct BtHWCore *hc = ctx->fwc_Private;
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), (const char *) fmt, ap);
    va_end(ap);

    btAddErrorMsg((UWORD) level, (STRPTR) GM_UNIQUENAME(libname),
                   "%s/%ld: %s", bth->bth_DevName, bth->bth_Unit, buf);
}
/* \\\ */

/* /// "bFillCtxIdentity()" */
static void bFillCtxIdentity(struct BtFirmwareContext *ctx, struct BtHardware *bth)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->fwc_Manufacturer  = bth->bth_ManufacturerID;
    ctx->fwc_HCIVersion    = bth->bth_HCIVersion;
    ctx->fwc_HCIRevision   = bth->bth_HCIRevision;
    ctx->fwc_LMPVersion    = bth->bth_LMPVersion;
    ctx->fwc_LMPSubversion = bth->bth_LMPSubversion;
}
/* \\\ */

/* /// "bFindLoader()" */
/* Highest-priority loader whose fwl_Match accepts this controller, or NULL.
   The caller must hold bt_FirmwareLock. */
static struct BtFirmwareLoader * bFindLoader(struct BtBase *BluetoothBase, struct BtFirmwareContext *ctx)
{
    struct BtFirmwareLoader *loader, *best = NULL;
    ULONG bestpri = 0;

    for(loader = (struct BtFirmwareLoader *) BluetoothBase->bt_FirmwareLoaders.lh_Head;
        loader->fwl_Node.mln_Succ;
        loader = (struct BtFirmwareLoader *) loader->fwl_Node.mln_Succ) {
        ULONG pri = loader->fwl_Match ? loader->fwl_Match(loader, ctx) : 0;
        if(pri > bestpri) {
            bestpri = pri;
            best = loader;
        }
    }
    return(best);
}
/* \\\ */

/* /// "bDoFirmware()" */
/* Offer the controller behind hc to the registered firmware loaders and let the
   best match download its firmware. Runs in the hwtask (so fwc_HciCommand can
   pump events). Idempotent: a controller flagged BTHF_FWLOADED is left alone. */
BOOL bDoFirmware(struct BtHWCore *hc)
{
    BOOL loaded = FALSE;
    struct BtBase *BluetoothBase = hc->hc_Base;
    struct BtHardware *bth = hc->hc_Hardware;
    struct BtFirmwareContext ctx;
    struct BtFirmwareLoader *best;

    if(bth->bth_Flags & BTHF_FWLOADED) {
        return(FALSE);
    }

    bFillCtxIdentity(&ctx, bth);
    ctx.fwc_HciCommand = bFwHciCommand;
    ctx.fwc_Log        = bFwLog;
    ctx.fwc_Private    = hc;

    /* hold the loader list (shared) across the whole load so a concurrent
       btRemFirmwareLoader() cannot free the loader mid-download. */
    ObtainSemaphoreShared(&BluetoothBase->bt_FirmwareLock);
    best = bFindLoader(BluetoothBase, &ctx);
    if(!best) {
        ReleaseSemaphore(&BluetoothBase->bt_FirmwareLock);
        /* No loader handles this controller yet. Most controllers need no
           firmware; leave it eligible so a loader binding later can retry. */
        bth->bth_Flags |= BTHF_FWPENDING;
        return(FALSE);
    }

    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "%s/%ld: firmware loader '%s' selected.",
                   bth->bth_DevName, bth->bth_Unit,
                   best->fwl_Name ? (STRPTR) best->fwl_Name : (STRPTR) "?");

    if(best->fwl_Load) {
        LONG err = best->fwl_Load(best, &ctx);
        if(err) {
            btAddErrorMsg(RETURN_FAIL, (STRPTR) GM_UNIQUENAME(libname),
                           "%s/%ld: firmware load failed (error %ld).",
                           bth->bth_DevName, bth->bth_Unit, err);
        } else {
            loaded = TRUE;
        }
    }
    ReleaseSemaphore(&BluetoothBase->bt_FirmwareLock);

    /* mark handled either way: retrying the same (missing/failing) loader would
       just loop on every future bind. A re-plug makes a fresh controller. */
    bth->bth_Flags &= ~BTHF_FWPENDING;
    bth->bth_Flags |= BTHF_FWLOADED;
    return(loaded);
}
/* \\\ */

/* /// "btAddFirmwareLoader()" */
AROS_LH1(void, btAddFirmwareLoader,
         AROS_LHA(struct BtFirmwareLoader *, loader, A0),
         LIBBASETYPEPTR, BluetoothBase, 86, bt)
{
    AROS_LIBFUNC_INIT
    struct BtHardware *bth;

    KPRINTF(5, ("btAddFirmwareLoader(%s)\n", loader ? loader->fwl_Name : (CONST_STRPTR) "?"));
    if(!loader) {
        return;
    }

    ObtainSemaphore(&BluetoothBase->bt_FirmwareLock);
    AddTail((struct List *) &BluetoothBase->bt_FirmwareLoaders, (struct Node *) &loader->fwl_Node);
    ReleaseSemaphore(&BluetoothBase->bt_FirmwareLock);

    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "firmware loader '%s' registered.",
                   loader->fwl_Name ? (STRPTR) loader->fwl_Name : (STRPTR) "?");

    /* re-check controllers already present: a hot-plugged radio may have come up
       before this loader bound and skipped the (then empty) firmware step. */
    btLockReadBase();
    for(bth = (struct BtHardware *) BluetoothBase->bt_Hardware.lh_Head;
        bth->bth_Node.ln_Succ;
        bth = (struct BtHardware *) bth->bth_Node.ln_Succ) {
        struct BtFirmwareContext ctx;

        if(bth->bth_Flags & BTHF_FWLOADED) {
            continue;                     /* already handled - never re-download */
        }
        if(!bth->bth_Task) {
            continue;                     /* not ready yet; its own bring-up will ask */
        }
        bFillCtxIdentity(&ctx, bth);
        ctx.fwc_HciCommand = NULL;        /* match is identity-only */
        ctx.fwc_Log        = NULL;
        ctx.fwc_Private    = NULL;
        if(loader->fwl_Match && loader->fwl_Match(loader, &ctx)) {
            /* wake the hwtask; it runs bDoFirmware() in its own context */
            Signal(bth->bth_Task, SIGBREAKF_CTRL_F);
        }
    }
    btUnlockBase();
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "btRemFirmwareLoader()" */
AROS_LH1(void, btRemFirmwareLoader,
         AROS_LHA(struct BtFirmwareLoader *, loader, A0),
         LIBBASETYPEPTR, BluetoothBase, 87, bt)
{
    AROS_LIBFUNC_INIT

    KPRINTF(5, ("btRemFirmwareLoader(%s)\n", loader ? loader->fwl_Name : (CONST_STRPTR) "?"));
    if(!loader) {
        return;
    }

    ObtainSemaphore(&BluetoothBase->bt_FirmwareLock);
    Remove((struct Node *) &loader->fwl_Node);
    ReleaseSemaphore(&BluetoothBase->bt_FirmwareLock);

    btAddErrorMsg(RETURN_OK, (STRPTR) GM_UNIQUENAME(libname),
                   "firmware loader '%s' removed.",
                   loader->fwl_Name ? (STRPTR) loader->fwl_Name : (STRPTR) "?");
    AROS_LIBFUNC_EXIT
}
/* \\\ */
