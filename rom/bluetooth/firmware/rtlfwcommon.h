#ifndef RTLFWCOMMON_H
#define RTLFWCOMMON_H
/*
 * Helpers shared by the Realtek v1 (rtlv1) and v2 (rtlv2) firmware loaders:
 * reading a blob from disk, reading the controller ROM version, chip-table
 * lookup and the vendor firmware download. Kept as static inlines in a header
 * so each loader stays a self-contained module (no shared link object).
 *
 * These never touch the network - firmware is read from disk only.
 */

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <libraries/bluetooth.h>

#include <string.h>

#include "rtlfwbase.h"

/* (lmp_subver, hci_rev) -> firmware basename; hci_rev 0xffff matches any rev. */
struct RtlChip
{
    UWORD        rc_LMPSubversion;
    UWORD        rc_HCIRevision;
    CONST_STRPTR rc_Base;
};

/* /// "rtlFindChip()" */
static const struct RtlChip * rtlFindChip(const struct RtlChip *table, UWORD subver, UWORD rev)
{
    const struct RtlChip *c;
    const struct RtlChip *any = NULL;

    for(c = table; c->rc_Base; c++) {
        if(c->rc_LMPSubversion == subver) {
            if(c->rc_HCIRevision == rev) {
                return(c);
            }
            if((c->rc_HCIRevision == 0xffff) && !any) {
                any = c;
            }
        }
    }
    return(any);
}
/* \\\ */

/* /// "rtlLoadFile()" */
/* Read a whole file into an AllocVec'd buffer (caller FreeVec's it), or NULL. */
static UBYTE * rtlLoadFile(struct DosLibrary *DOSBase, CONST_STRPTR path, ULONG *lenp)
{
    BPTR fh;
    LONG size = 0;
    UBYTE *buf = NULL;

    *lenp = 0;
    if(!(fh = Open((STRPTR) path, MODE_OLDFILE))) {
        return(NULL);
    }
    if(Seek(fh, 0, OFFSET_END) >= 0) {
        size = Seek(fh, 0, OFFSET_BEGINNING);
    }
    if((size > 0) && (buf = AllocVec(size, MEMF_ANY))) {
        if(Read(fh, buf, size) == size) {
            *lenp = (ULONG) size;
        } else {
            FreeVec(buf);
            buf = NULL;
        }
    }
    Close(fh);
    return(buf);
}
/* \\\ */

/* /// "rtlReadRomVersion()" */
/* HCI vendor Read ROM Version (0xFC6D). Return params are [status][version]. */
static LONG rtlReadRomVersion(struct BtFirmwareContext *ctx, UBYTE *ever)
{
    UBYTE resp[8];
    UWORD rlen = 0;
    UBYTE status = 0xff;
    LONG err;

    *ever = 0;
    err = ctx->fwc_HciCommand(ctx, HC_OP_RTL_READ_ROM_VERSION, NULL, 0,
                              &status, resp, &rlen, sizeof(resp));
    if(err || status) {
        ctx->fwc_Log(ctx, RETURN_FAIL, "read ROM version failed (status 0x%02lx).", (ULONG) status);
        return(-1);
    }
    if(rlen >= 2) {
        *ever = resp[1];
    }
    return(0);
}
/* \\\ */

/* /// "rtlDownload()" */
/* Send the built payload to the controller in RTL_FRAG_LEN fragments via the
   vendor Download command (0xFC20): [index|last<<7][data...]. */
static LONG rtlDownload(struct BtFirmwareContext *ctx, const UBYTE *payload, ULONG len)
{
    UBYTE cmd[1 + RTL_FRAG_LEN];
    ULONG pos = 0;
    UWORD frag = 0;

    while(pos < len) {
        ULONG remain = len - pos;
        UWORD chunk = (remain > RTL_FRAG_LEN) ? RTL_FRAG_LEN : (UWORD) remain;
        BOOL  last  = (pos + chunk >= len);
        UBYTE status = 0xff;
        LONG  err;

        cmd[0] = (UBYTE)((frag & 0x7f) | (last ? 0x80 : 0x00));
        CopyMem((APTR) &payload[pos], &cmd[1], chunk);
        err = ctx->fwc_HciCommand(ctx, HC_OP_RTL_DOWNLOAD, cmd, (UWORD)(1 + chunk),
                                  &status, NULL, NULL, 0);
        if(err || status) {
            ctx->fwc_Log(ctx, RETURN_FAIL, "download failed at fragment %ld (status 0x%02lx).",
                         (ULONG) frag, (ULONG) status);
            return(-1);
        }
        pos += chunk;
        frag++;
    }
    ctx->fwc_Log(ctx, RETURN_OK, "firmware downloaded (%ld bytes, %ld fragments).", len, (ULONG) frag);
    return(0);
}
/* \\\ */

#endif /* RTLFWCOMMON_H */
