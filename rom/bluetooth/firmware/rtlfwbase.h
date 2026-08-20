#ifndef RTLFWBASE_H
#define RTLFWBASE_H
/*
 * Shared libbase for the Realtek firmware-loader modules (rtlv1 / rtlv2).
 * Each module is a small library in DEVS:Bluetooth/FWLoaders/ that registers a
 * single BtFirmwareLoader with bluetooth.library on init.
 */

#include <exec/libraries.h>
#include <dos/dosextens.h>
#include <libraries/bluetooth.h>

struct RtlFwBase
{
    struct Library          rfw_Lib;
    struct Library         *rfw_BluetoothBase;   /* for btAdd/RemFirmwareLoader */
    struct DosLibrary      *rfw_DOSBase;         /* for reading firmware blobs */
    struct BtFirmwareLoader rfw_Loader;          /* the descriptor we register */
};

/* Realtek HCI vendor commands (OGF 0x3f) used by both formats */
#define RTL_OP(ocf)                 (((UWORD)0x3f << 10) | (ocf))
#define HC_OP_RTL_READ_ROM_VERSION  RTL_OP(0x006d)
#define HC_OP_RTL_DOWNLOAD          RTL_OP(0x0020)

#define RTL_FRAG_LEN                252          /* firmware bytes per download cmd */
#define BT_MANUFACTURER_REALTEK     93           /* Bluetooth SIG company id */
#define RTL_FW_DIR                  "DEVS:Firmware/rtlbt/"

#endif /* RTLFWBASE_H */
