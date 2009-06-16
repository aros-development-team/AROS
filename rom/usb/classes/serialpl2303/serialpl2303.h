#ifndef SERIALPL2303_H
#define SERIALPL2303_H

#include <devices/usb.h>
#include <exec/devices.h>

/* Products supported */
#define PL2303_VENDOR_ID         0x067b
#define PL2303_PRODUCT_ID        0x2303
#define PL2303_PRODUCT_ID_RSAQ2  0x04bb
#define PL2303_PRODUCT_ID_DCU11  0x1234
#define PL2303_PRODUCT_ID_PHAROS 0xaaa0
#define PL2303_PRODUCT_ID_RSAQ3  0xaaa2
#define PL2303_PRODUCT_ID_ALDIGA 0x0611

#define ATEN_VENDOR_ID           0x0557
#define ATEN_VENDOR_ID2          0x0547
#define ATEN_PRODUCT_ID          0x2008

#define IODATA_VENDOR_ID         0x04bb
#define IODATA_PRODUCT_ID        0x0a03
#define IODATA_PRODUCT_ID_RSAQ5  0x0a0e

#define ELCOM_VENDOR_ID          0x056e
#define ELCOM_PRODUCT_ID         0x5003
#define ELCOM_PRODUCT_ID_UCSGT   0x5004

#define ITEGNO_VENDOR_ID         0x0eba
#define ITEGNO_PRODUCT_ID        0x1080
#define ITEGNO_PRODUCT_ID_2      0x2080

#define MA620_VENDOR_ID          0x0df7
#define MA620_PRODUCT_ID         0x0620

#define RATOC_VENDOR_ID          0x0584
#define RATOC_PRODUCT_ID         0xb000
#define RATOC_PRODUCT_ID_USB60F  0xb020

#define TRIPP_VENDOR_ID          0x2478
#define TRIPP_PRODUCT_ID         0x2008

#define RADIOSHACK_VENDOR_ID     0x1453
#define RADIOSHACK_PRODUCT_ID    0x4026

#define DCU10_VENDOR_ID          0x0731
#define DCU10_PRODUCT_ID         0x0528

#define SITECOM_VENDOR_ID        0x6189
#define SITECOM_PRODUCT_ID       0x2068

#define ALCATEL_VENDOR_ID        0x11f7
#define ALCATEL_PRODUCT_ID       0x02df

#define SAMSUNG_VENDOR_ID        0x04e8
#define SAMSUNG_PRODUCT_ID       0x8001

#define SIEMENS_VENDOR_ID        0x11f5
#define SIEMENS_PRODUCT_ID_SX1   0x0001
#define SIEMENS_PRODUCT_ID_X65   0x0003
#define SIEMENS_PRODUCT_ID_X75   0x0004
#define SIEMENS_PRODUCT_ID_EF81  0x0005

#define SYNTECH_VENDOR_ID        0x0745
#define SYNTECH_PRODUCT_ID       0x0001

#define NOKIA_CA42_VENDOR_ID     0x078b
#define NOKIA_CA42_PRODUCT_ID    0x1234

#define CA_42_CA42_VENDOR_ID     0x10b5
#define CA_42_CA42_PRODUCT_ID    0xac70

#define SAGEM_VENDOR_ID          0x079b
#define SAGEM_PRODUCT_ID         0x0027

#define LEADTEK_VENDOR_ID        0x0413
#define LEADTEK_9531_PRODUCT_ID  0x2101

#define SPEEDDRAGON_VENDOR_ID    0x0e55
#define SPEEDDRAGON_PRODUCT_ID   0x110b

#define OTI_VENDOR_ID            0x0ea0
#define OTI_PRODUCT_ID           0x6858

#define DATAPILOT_U2_VENDOR_ID   0x0731
#define DATAPILOT_U2_PRODUCT_ID  0x2003

#define BELKIN_VENDOR_ID         0x050d
#define BELKIN_PRODUCT_ID        0x0257

#define ALCOR_VENDOR_ID          0x058F
#define ALCOR_PRODUCT_ID         0x9720

#define WS002IN_VENDOR_ID        0x11f6
#define WS002IN_PRODUCT_ID       0x2001

#define COREGA_VENDOR_ID         0x07aa
#define COREGA_PRODUCT_ID        0x002a

#define HL340_VENDOR_ID          0x4348
#define HL340_PRODUCT_ID         0x5523

#define YCCABLE_VENDOR_ID        0x05ad
#define YCCABLE_PRODUCT_ID       0x0fba

/* Requests */
#define UPLR_WRITE        0x01 /* vendor specific device request */
#define UPLR_READ         0x01 /* vendor specific device request */

#define UPLF_CONTROL_DTR  0x01
#define UPLF_CONTROL_RTS  0x02

#define UART_STATE                      0x08
#define UART_STATE_TRANSIENT_MASK       0x74
#define UART_DCD                        0x01
#define UART_DSR                        0x02
#define UART_BREAK                      0x04
#define UART_RING                       0x08
#define UART_FRAME_ERROR                0x10
#define UART_PARITY_ERROR               0x20
#define UART_OVERRUN_ERROR              0x40
#define UART_CTS                        0x80

/* Misc */

#define DEFREADBUFLEN 2048
#define NUMREADPIPES 16

struct UsbCDCSerialState
{
    struct UsbSetupData uss_Req;
    UWORD               uss_State;
};

struct NepSerialBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* utility base */

    struct NepSerDevBase *nh_DevBase;     /* base of device created */
    struct List         nh_Units;         /* List of units available */
};

struct NepSerDevBase
{
    struct Library      np_Library;       /* standard */
    UWORD               np_Flags;         /* various flags */

    BPTR                np_SegList;       /* device seglist */
    struct NepSerialBase *np_ClsBase;     /* pointer to class base */
    struct Library     *np_UtilityBase;   /* cached utilitybase */
};

struct NepClassSerial
{
    struct Unit         ncp_Unit;         /* Unit structure */
    ULONG               ncp_UnitNo;       /* Unit number */
    struct NepSerDevBase *ncp_DevBase;    /* Device base */
    struct Library     *ncp_Base;         /* Poseidon base */
    struct PsdDevice   *ncp_Device;       /* Up linkage */
    struct PsdConfig   *ncp_Config;       /* Up linkage */
    struct PsdInterface *ncp_Interface;   /* Up linkage */
    struct Task        *ncp_ReadySigTask; /* Task to send ready signal to */
    LONG                ncp_ReadySignal;  /* Signal to send when ready */
    LONG                ncp_AbortSignal;  /* Signal to abort write on */
    struct Task        *ncp_Task;         /* Subtask */
    struct MsgPort     *ncp_TaskMsgPort;  /* Message Port of Subtask */

    struct PsdPipe     *ncp_EP0Pipe;      /* Endpoint 0 pipe */
    struct PsdEndpoint *ncp_EPOut;        /* OUT Endpoint */
    struct PsdPipeStream *ncp_EPOutStream; /* OUT Endpoint stream */
    struct PsdEndpoint *ncp_EPIn;         /* IN Endpoint */
    struct PsdPipeStream *ncp_EPInStream; /* IN Endpoint stream */
    struct PsdEndpoint *ncp_EPInt;        /* NOTIFY Endpoint */
    struct PsdPipe     *ncp_EPIntPipe;    /* NOTIFY Endpoint pipe */
    UWORD               ncp_EPOutNum;     /* Endpoint OUT number */
    UWORD               ncp_EPInNum;      /* Endpoint IN number */
    struct MsgPort     *ncp_DevMsgPort;   /* Message Port for IOParReq */
    BOOL                ncp_HXChipset;    /* HX Chips need special treatment */
    BOOL                ncp_SiemensCrap;  /* Siemens crap chipset */
    UWORD               ncp_UnitProdID;   /* ProductID of unit */
    UWORD               ncp_UnitVendorID; /* VendorID of unit */
    UWORD               ncp_UnitCfgNum;   /* Config of unit */
    UWORD               ncp_UnitIfNum;    /* Interface number */
    UWORD               ncp_UnitAltIfNum; /* Alternate interface number */
    BOOL                ncp_DenyRequests; /* Do not accept further IO requests */
    BOOL                ncp_DevSuspend;   /* suspend things */

    UBYTE               ncp_SerialStateReq[10]; /* Serial State stuff */

    struct IOExtSer    *ncp_WritePending; /* write IORequest pending */
    struct List         ncp_ReadQueue;    /* List of read requests */
    struct List         ncp_WriteQueue;   /* List of write requests */
};

#endif /* SERIALPL2303_H */
