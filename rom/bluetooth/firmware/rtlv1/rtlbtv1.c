/*
 *----------------------------------------------------------------------------
 *   rtlbtv1.fwl - Realtek legacy ("Realtech") firmware loader
 *----------------------------------------------------------------------------
 *
 * A pluggable BtFirmwareLoader (DEVS:Bluetooth/FWLoaders/) for the older
 * Realtek controllers whose firmware uses the legacy "Realtech" epatch format
 * (RTL8761A/BU, RTL8723B/D, RTL8821A/C, RTL8822B). The module registers itself
 * with bluetooth.library on init; the stack calls fwl_Match/fwl_Load during a
 * controller's bring-up. Firmware blobs are read from DEVS:Firmware/rtlbt/.
 *
 * The v2 "RTBTCore" controllers (RTL8852x, RTL8822C, RTL8851B) are handled by
 * the separate rtlbtv2.fwl loader.
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

/* legacy-format chips: (lmp_subver, hci_rev) -> firmware basename */
static const struct RtlChip rtl_v1_chips[] =
{
    { 0x8761, 0x000a, "rtl8761a"  },
    { 0x8761, 0x000b, "rtl8761bu" },
    { 0x8723, 0x000b, "rtl8723b"  },
    { 0x8723, 0x000d, "rtl8723d"  },
    { 0x8821, 0x000a, "rtl8821a"  },
    { 0x8821, 0x000c, "rtl8821c"  },
    { 0x8822, 0x000b, "rtl8822b"  },
    { 0, 0, NULL }
};

/* /// "rtlV1Match()" */
static ULONG rtlV1Match(struct BtFirmwareLoader *self, struct BtFirmwareContext *ctx)
{
    (void) self;
    if(ctx->fwc_Manufacturer != BT_MANUFACTURER_REALTEK) {
        return(0);
    }
    return(rtlFindChip(rtl_v1_chips, ctx->fwc_LMPSubversion, ctx->fwc_HCIRevision) ? 100 : 0);
}
/* \\\ */

/* /// "rtlV1Load()" */
/* Legacy "Realtech" epatch: [8]sig [4]fw_version [2]num_patches, then
   num_patches * (u16 chip_id, u16 patch_len, u32 patch_off). Pick the patch
   whose chip_id == ROM version + 1 (else the last), overwrite its final 4 bytes
   with fw_version, append the config blob and download the result. */
static LONG rtlV1Load(struct BtFirmwareLoader *self, struct BtFirmwareContext *ctx)
{
    struct RtlFwBase *rfw = self->fwl_UserData;
    struct DosLibrary *DOSBase = rfw->rfw_DOSBase;
    const struct RtlChip *chip;
    UBYTE ever = 0;
    UBYTE *fw = NULL, *cfg = NULL, *payload = NULL;
    ULONG fwlen = 0, cfglen = 0;
    ULONG numpatches, i, patchoff = 0, patchlen = 0, paylen;
    LONG sel = -1;
    char path[64];
    LONG res = -1;

    chip = rtlFindChip(rtl_v1_chips, ctx->fwc_LMPSubversion, ctx->fwc_HCIRevision);
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

    if((fwlen < 14) || memcmp(fw, "Realtech", 8)) {
        ctx->fwc_Log(ctx, RETURN_FAIL, "'%s' is not a legacy (Realtech) firmware.", chip->rc_Base);
        goto done;
    }
    numpatches = fw[12] | (fw[13] << 8);
    if((numpatches == 0) || (14 + numpatches * 8 > fwlen)) {
        ctx->fwc_Log(ctx, RETURN_FAIL, "firmware patch table invalid (%ld patches).", numpatches);
        goto done;
    }
    for(i = 0; i < numpatches; i++) {
        UWORD id = fw[14 + i*2] | (fw[15 + i*2] << 8);
        if(id == (UWORD)(ever + 1)) {
            sel = (LONG) i;
        }
    }
    if(sel < 0) {
        sel = (LONG)(numpatches - 1);
    }
    patchlen = fw[14 + numpatches*2 + sel*2] | (fw[15 + numpatches*2 + sel*2] << 8);
    patchoff = fw[14 + numpatches*4 + sel*4] | (fw[15 + numpatches*4 + sel*4] << 8)
             | (fw[16 + numpatches*4 + sel*4] << 16) | (fw[17 + numpatches*4 + sel*4] << 24);
    if((patchlen < 4) || (patchoff + patchlen > fwlen)) {
        ctx->fwc_Log(ctx, RETURN_FAIL, "selected firmware patch is out of range.");
        goto done;
    }

    paylen = patchlen + cfglen;
    if(!(payload = AllocVec(paylen, MEMF_ANY))) {
        goto done;
    }
    CopyMem(&fw[patchoff], payload, patchlen);
    CopyMem(&fw[8], &payload[patchlen - 4], 4);      /* fw_version into patch tail */
    if(cfg && cfglen) {
        CopyMem(cfg, &payload[patchlen], cfglen);
    }

    ctx->fwc_Log(ctx, RETURN_OK,
                 "Realtek %s: ROM ver %ld, %ld patch(es), using #%ld (%ld B) + %ld B config.",
                 chip->rc_Base, (ULONG) ever, numpatches, sel, patchlen, cfglen);
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

    rfw->rfw_Loader.fwl_Name     = "Realtek (legacy)";
    rfw->rfw_Loader.fwl_Match    = rtlV1Match;
    rfw->rfw_Loader.fwl_Load     = rtlV1Load;
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
