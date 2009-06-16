#ifndef DFU_H
#define DFU_H

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>
#include <devices/usb_dfu.h>

#if defined(__GNUC__)
# pragma pack(2)
#endif

#define ID_ABOUT        0x55555555
#define ID_DOWNLOAD     0xaaaaaaaa
#define ID_UPLOAD       0xaaaaaaab
#define ID_DETACH       0xaaaaaaac


#define STATUS_OK               0x00
#define STATUS_ERR_TARGET       0x01
#define STATUS_ERR_FILE         0x02
#define STATUS_ERR_WRITE        0x03
#define STATUS_ERR_ERASE        0x04
#define STATUS_ERR_CHECK_ERASED 0x05
#define STATUS_ERR_PROG         0x06
#define STATUS_ERR_VERIFY       0x07
#define STATUS_ERR_ADDRESS      0x08
#define STATUS_ERR_NOTDONE      0x09
#define STATUS_ERR_FIRMWARE     0x0A
#define STATUS_ERR_VENDOR       0x0B
#define STATUS_ERR_USBR         0x0C
#define STATUS_ERR_POR          0x0D
#define STATUS_ERR_UNKNOWN      0x0E
#define STATUS_ERR_STALLEDPKT   0x0F

#define STATE_APP_IDLE                 0
#define STATE_APP_DETACH               1
#define STATE_DFU_IDLE                 2
#define STATE_DFU_DNLOAD_SYNC          3
#define STATE_DFU_DNBUSY               4
#define STATE_DFU_DNLOAD_IDLE          5
#define STATE_DFU_MANIFEST_SYNC        6
#define STATE_DFU_MANIFEST             7
#define STATE_DFU_MANIFEST_WAIT_RESET  8
#define STATE_DFU_UPLOAD_IDLE          9
#define STATE_DFU_ERROR               10

#if defined(__GNUC__)
# pragma pack()
#endif

struct NepClassDFU
{
    struct Node         nch_Node;         /* Node linkage */
    struct NepDFUBase  *nch_ClsBase;      /* Up linkage */
    struct Library     *nch_Base;         /* Poseidon base */
    struct PsdDevice   *nch_Device;       /* Up linkage */
    struct PsdDevice   *nch_Hub;          /* Up linkage */
    struct PsdConfig   *nch_Config;       /* Up linkage */
    struct PsdInterface *nch_Interface;   /* Up linkage */
    struct PsdPipe     *nch_EP0Pipe;      /* Endpoint 0 pipe */
    ULONG               nch_HubPort;
    struct Task        *nch_ReadySigTask; /* Task to send ready signal to */
    LONG                nch_ReadySignal;  /* Signal to send when ready */
    struct MsgPort     *nch_TaskMsgPort;  /* Message Port for Pipe */
    ULONG               nch_IfNum;        /* Interface number */

    STRPTR              nch_DevIDString;  /* Device ID String */
    STRPTR              nch_IfIDString;   /* Interface ID String */

    BOOL                nch_WillDetach;   /* automatically detaches */
    BOOL                nch_CanRetrieve;  /* allows firmware downloading */
    BOOL                nch_CanUpgrade;   /* allows firmware upgrading */
    BOOL                nch_NoManifestReset; /* no need to reset after manifestation */
    ULONG               nch_DetachTimeOut; /* Max time (ms) before detach fails */
    ULONG               nch_TransferSize; /* Maximum bytes to write */

    UBYTE              *nch_Buffer;
    BPTR                nch_InOutFile;

    struct UsbDFUStatus nch_DFUStatus;

    struct Library     *nch_MUIBase;      /* MUI master base */
    struct Library     *nch_PsdBase;      /* Poseidon base */
    struct Library     *nch_IntBase;      /* Intuition base */
    struct Library     *nch_DOSBase;      /* DOS base */
    struct Task        *nch_GUITask;      /* GUI Task */
    struct NepClassDFU *nch_GUIBinding;   /* Window of binding that's open */

    Object             *nch_App;
    Object             *nch_MainWindow;
    Object             *nch_FWFileObj;
    Object             *nch_GaugeObj;

    Object             *nch_DetachObj;
    Object             *nch_DownloadObj;
    Object             *nch_UploadObj;
    Object             *nch_CloseObj;

    Object             *nch_AboutMI;
    Object             *nch_MUIPrefsMI;
};

struct NepDFUBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* Utility base */
};

#endif /* DFU_H */
