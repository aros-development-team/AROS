/*
 *----------------------------------------------------------------------------
 *   rtlbtv2.fwl - Realtek "RTBTCore" (v2) firmware loader
 *----------------------------------------------------------------------------
 *
 * A pluggable BtFirmwareLoader (DEVS:Bluetooth/FWLoaders/) for the newer
 * Realtek controllers whose firmware uses the section-based "RTBTCore" v2
 * container (RTL8852A/B/C, RTL8822C, RTL8851B). Registers itself with
 * bluetooth.library on init; blobs are read from DEVS:Firmware/rtlbt/.
 *
 * The legacy "Realtech" controllers are handled by the separate rtlbtv1.fwl.
 *
 * NOTE: the v2 container parse below (signature + section walk + patch-snippet
 * subsection selection by ECO) follows the documented RTBTCore layout, but has
 * not yet been verified against real v2 hardware. Every offset is bounds-checked
 * so a mis-parse fails cleanly (the controller simply rejects the download and
 * stays in ROM mode) rather than sending anything harmful.
 */

#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <exec/exec.h>
#include <exec/types.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bluetooth.h>

#include "../rtlfwbase.h"
#include "../rtlfwcommon.h"

#include LC_LIBDEFS_FILE

static const STRPTR libname = MOD_NAME_STRING;

/* RTBTCore section opcodes */
#define RTL_V2_OPCODE_PATCH_SNIPPETS  0x01

/* v2-format chips: (lmp_subver, hci_rev) -> firmware basename */
static const struct RtlChip rtl_v2_chips[] =
{
    { 0x8822, 0x000c, "rtl8822cu" },
    { 0x8852, 0x000a, "rtl8852au" },
    { 0x8852, 0x000b, "rtl8852bu" },
    { 0x8852, 0x000c, "rtl8852cu" },
    { 0x8851, 0x000b, "rtl8851bu" },
    { 0, 0, NULL }
};

static ULONG rd32(const UBYTE *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((ULONG) p[3] << 24); }
static UWORD rd16(const UBYTE *p) { return (UWORD)(p[0] | (p[1] << 8)); }

/* /// "rtlV2Match()" */
static ULONG rtlV2Match(struct BtFirmwareLoader *self, struct BtFirmwareContext *ctx)
{
    (void) self;
    if(ctx->fwc_Manufacturer != BT_MANUFACTURER_REALTEK) {
        return(0);
    }
    return(rtlFindChip(rtl_v2_chips, ctx->fwc_LMPSubversion, ctx->fwc_HCIRevision) ? 90 : 0);
}
/* \\\ */

/* /// "rtlV2Load()" */
/* RTBTCore container: [8]sig "RTBTCore" [8]fw_version [4]num_sections, then
   num_sections * { [4]opcode [4]len [len]data }. The patch-snippets section
   (opcode 0x01) holds [2]num [2]resv then subsections { [1]eco [1]prio [2]cb
   [4]len [len]data }; pick the subsection whose eco == ROM version + 1. */
static LONG rtlV2Load(struct BtFirmwareLoader *self, struct BtFirmwareContext *ctx)
{
    struct RtlFwBase *rfw = self->fwl_UserData;
    struct DosLibrary *DOSBase = rfw->rfw_DOSBase;
    const struct RtlChip *chip;
    UBYTE ever = 0;
    UBYTE *fw = NULL, *cfg = NULL, *payload = NULL;
    ULONG fwlen = 0, cfglen = 0;
    ULONG num_sections, i, off, paylen;
    ULONG patchoff = 0, patchlen = 0;
    char path[64];
    LONG res = -1;

    chip = rtlFindChip(rtl_v2_chips, ctx->fwc_LMPSubversion, ctx->fwc_HCIRevision);
    if(!chip) {
        return(-1);
    }
    if(rtlReadRomVersion(ctx, &ever)) {
        return(-1);
    }

    strcpy(path, RTL_FW_DIR);
    strcat(path, chip->rc_Base);
    strcat(path, "_fw.bin");
    if(!(fw = rtlLoadFile(DOSBase, path, &fwlen))) {
        ctx->fwc_Log(ctx, RETURN_FAIL, "firmware '%s' missing - radio will not scan.", path);
        return(-1);
    }
    strcpy(path, RTL_FW_DIR);
    strcat(path, chip->rc_Base);
    strcat(path, "_config.bin");
    cfg = rtlLoadFile(DOSBase, path, &cfglen);       /* optional */

    if((fwlen < 20) || memcmp(fw, "RTBTCore", 8)) {
        ctx->fwc_Log(ctx, RETURN_FAIL, "'%s' is not an RTBTCore (v2) firmware.", chip->rc_Base);
        goto done;
    }
    num_sections = rd32(&fw[16]);
    off = 20;
    for(i = 0; (i < num_sections) && (off + 8 <= fwlen); i++) {
        ULONG opcode = rd32(&fw[off]);
        ULONG slen   = rd32(&fw[off + 4]);
        ULONG sdata  = off + 8;
        ULONG send;

        if((sdata + slen < sdata) || (sdata + slen > fwlen)) {
            break;                                    /* overflow / out of range */
        }
        send = sdata + slen;

        if((opcode == RTL_V2_OPCODE_PATCH_SNIPPETS) && (slen >= 4)) {
            UWORD nsub = rd16(&fw[sdata]);
            ULONG so = sdata + 4;
            UWORD j;
            for(j = 0; (j < nsub) && (so + 8 <= send); j++) {
                UBYTE eco    = fw[so];
                ULONG sublen = rd32(&fw[so + 4]);
                ULONG subdat = so + 8;

                if((subdat + sublen < subdat) || (subdat + sublen > send)) {
                    break;
                }
                /* prefer the exact ECO match; otherwise remember the last one */
                if((eco == (UBYTE)(ever + 1)) || (patchlen == 0)) {
                    patchoff = subdat;
                    patchlen = sublen;
                    if(eco == (UBYTE)(ever + 1)) {
                        j = nsub;                     /* stop: exact match found */
                    }
                }
                so = subdat + sublen;
            }
        }
        off = send;
    }

    if(!patchlen) {
        ctx->fwc_Log(ctx, RETURN_FAIL, "no matching v2 patch snippet for ROM ver %ld.", (ULONG) ever);
        goto done;
    }

    paylen = patchlen + cfglen;
    if(!(payload = AllocVec(paylen, MEMF_ANY))) {
        goto done;
    }
    CopyMem(&fw[patchoff], payload, patchlen);
    if(cfg && cfglen) {
        CopyMem(cfg, &payload[patchlen], cfglen);
    }

    ctx->fwc_Log(ctx, RETURN_OK,
                 "Realtek %s (v2, provisional): ROM ver %ld, patch %ld B + %ld B config.",
                 chip->rc_Base, (ULONG) ever, patchlen, cfglen);
    res = rtlDownload(ctx, payload, paylen);

done:
    if(payload) FreeVec(payload);
    if(fw)      FreeVec(fw);
    if(cfg)     FreeVec(cfg);
    return(res);
}
/* \\\ */

/* /// "libInit()" */
static int GM_UNIQUENAME(libInit)(LIBBASETYPEPTR rfw)
{
    struct Library *BluetoothBase;

    rfw->rfw_DOSBase = (struct DosLibrary *) OpenLibrary("dos.library", 0);
    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1))) {
        if(rfw->rfw_DOSBase) CloseLibrary((struct Library *) rfw->rfw_DOSBase);
        return FALSE;
    }
    rfw->rfw_BluetoothBase = BluetoothBase;

    rfw->rfw_Loader.fwl_Name     = "Realtek (RTBTCore v2)";
    rfw->rfw_Loader.fwl_Match    = rtlV2Match;
    rfw->rfw_Loader.fwl_Load     = rtlV2Load;
    rfw->rfw_Loader.fwl_UserData = rfw;
    btAddFirmwareLoader(&rfw->rfw_Loader);
    return TRUE;
}
/* \\\ */

/* /// "libExpunge()" */
static int GM_UNIQUENAME(libExpunge)(LIBBASETYPEPTR rfw)
{
    struct Library *BluetoothBase = rfw->rfw_BluetoothBase;

    if(BluetoothBase) {
        btRemFirmwareLoader(&rfw->rfw_Loader);
        CloseLibrary(BluetoothBase);
    }
    if(rfw->rfw_DOSBase) {
        CloseLibrary((struct Library *) rfw->rfw_DOSBase);
    }
    return TRUE;
}
/* \\\ */

ADD2INITLIB(GM_UNIQUENAME(libInit), 0)
ADD2EXPUNGELIB(GM_UNIQUENAME(libExpunge), 0)
