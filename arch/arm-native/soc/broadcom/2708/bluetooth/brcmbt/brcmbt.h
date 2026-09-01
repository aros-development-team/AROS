#ifndef BRCMBT_H
#define BRCMBT_H
/*
 * Libbase for the Broadcom Bluetooth firmware loader.
 *
 * A pluggable BtFirmwareLoader in DEVS:Bluetooth/FWLoaders/, modelled on
 * AROS's Realtek loaders (rom/bluetooth/firmware/rtlv1, rtlv2). It registers
 * one loader with bluetooth.library on init; the stack offers each controller
 * to every registered loader during bring-up (fwl_Match) and lets the best
 * match run (fwl_Load).
 */

#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <libraries/bluetooth.h>

struct BrcmFwBase
{
    struct Library          bfw_Lib;
    struct Library         *bfw_BluetoothBase;   /* for btAdd/RemFirmwareLoader */
    struct DosLibrary      *bfw_DOSBase;         /* for reading the .hcd */
    struct BtFirmwareLoader bfw_Loader;
};

/* Broadcom HCI vendor commands (OGF 0x3f). */
#define BRCM_OP(ocf)                (((UWORD)0x3f << 10) | (ocf))
#define HC_OP_BRCM_DOWNLOAD_MINIDRV BRCM_OP(0x002e)
#define HC_OP_BRCM_WRITE_RAM        BRCM_OP(0x004c)
#define HC_OP_BRCM_LAUNCH_RAM       BRCM_OP(0x004e)

/* Host Control group: HCI_Reset, the smallest command a controller answers. */
#define HC_OP_HCI_RESET             ((UWORD)(((UWORD)0x03 << 10) | 0x0003))

/*
 * Recovering from the restart that Launch_RAM causes. Settle, then ask the
 * controller whether it is back, a bounded number of times -- see
 * brcmWaitRestart().
 */
#define BRCM_RESTART_SETTLE_MS      300
#define BRCM_RESTART_PROBES         3

#define BT_MANUFACTURER_BROADCOM    15           /* Bluetooth SIG company id */
#define BRCM_FW_DIR                 "DEVS:Firmware/brcm/"

/*
 * A .hcd is a flat sequence of HCI commands: 2 bytes opcode (little-endian),
 * 1 byte parameter length, then that many parameter bytes. Sending them in
 * order is the whole of the patchram protocol.
 */
#define BRCM_HCD_HDR                3

#endif /* BRCMBT_H */
