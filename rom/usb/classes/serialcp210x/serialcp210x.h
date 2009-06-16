#ifndef SERIALCP210X_H
#define SERIALCP210X_H

#include <devices/usb.h>
#include <exec/devices.h>

/* Products supported */

/* Requests */
#define UCPR_SET_STATE          0x00    /* Enable / Disable */
#define UCPR_SET_BAUDRATE       0x01    /* (BAUD_RATE_GEN_FREQ / baudrate) */
#define UCPR_GET_BAUDRATE       0x02    /* (BAUD_RATE_GEN_FREQ / baudrate) */
#define UCPR_SET_BITS           0x03    /* 0x(0)(databits)(parity)(stopbits) */
#define UCPR_GET_BITS           0x04    /* 0x(0)(databits)(parity)(stopbits) */

#define UCPR_SET_BREAK          0x05    /* On / Off */
#define UCPR_GET_BREAK          0x06    /* On / Off */
#define UCPR_SET_CONTROL        0x07    /* Flow control line states */
#define UCPR_GET_CONTROL        0x08    /* Flow control line states */
#define UCPR_SET_MODEMCTL       0x13    /* Modem controls */
#define UCPR_GET_MODEMCTL       0x14    /* Modem controls */
#define UCPR_SET_MISC           0x19
#define UCPR_GET_MISC           0x1a

/* CP2101_UART */
#define UART_ENABLE             0x0001
#define UART_DISABLE            0x0000

/* CP2101_BAUDRATE */
#define BAUD_RATE_GEN_FREQ      0x384000

/* CP2101_BITS */
#define BITS_PARITY_NONE        0x0000
#define BITS_PARITY_ODD         0x0010
#define BITS_PARITY_EVEN        0x0020
#define BITS_PARITY_MARK        0x0030
#define BITS_PARITY_SPACE       0x0040

#define BITS_STOP_1             0x0000
#define BITS_STOP_1_5           0x0001
#define BITS_STOP_2             0x0002

/* CP2101_BREAK */
#define BREAK_ON                0x0000
#define BREAK_OFF               0x0001

/* CP2101_CONTROL */
#define UART_DTR                0x0001
#define UART_RTS                0x0002
#define UART_CTS                0x0010
#define UART_DSR                0x0020
#define UART_RING               0x0040
#define UART_DCD                0x0080
#define UART_WRITE_DTR          0x0100
#define UART_WRITE_RTS          0x0200


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
    UWORD               ncp_EPOutNum;     /* Endpoint OUT number */
    UWORD               ncp_EPInNum;      /* Endpoint IN number */
    struct MsgPort     *ncp_DevMsgPort;   /* Message Port for IOParReq */
    UWORD               ncp_UnitProdID;   /* ProductID of unit */
    UWORD               ncp_UnitVendorID; /* VendorID of unit */
    UWORD               ncp_UnitCfgNum;   /* Config of unit */
    UWORD               ncp_UnitIfNum;    /* Interface number */
    UWORD               ncp_UnitAltIfNum; /* Alternate interface number */
    BOOL                ncp_DenyRequests; /* Do not accept further IO requests */
    BOOL                ncp_DevSuspend;   /* suspend things */

    struct IOExtSer    *ncp_WritePending; /* write IORequest pending */
    struct List         ncp_ReadQueue;    /* List of read requests */
    struct List         ncp_WriteQueue;   /* List of write requests */
};

#endif /* SERIALCP210X_H */
