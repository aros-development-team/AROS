#ifndef STIR4200_H
#define STIR4200_H

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>
#include <exec/devices.h>

#if defined(__GNUC__)
# pragma pack(2)
#endif

#define ID_ABOUT        0x55555555
#define ID_STORE_CONFIG 0xaaaaaaaa
#define ID_DEF_CONFIG   0xaaaaaaab

struct ClsDevCfg
{
    ULONG cdc_ChunkID;
    ULONG cdc_Length;
    ULONG cdc_StackAuto;
    ULONG cdc_DefaultUnit;
    ULONG cdc_RXSense;
    ULONG cdc_TXPower;
};

#if defined(__GNUC__)
# pragma pack()
#endif

/* Misc */

#define STIR_IRDA_HEADER  4
#define STIR_MIN_RTT 500
#define STIR_IRDA_RXFRAME_LEN 64
#define STIR_FIFO_SIZE 4096
#define STIR_IDLE_TIMEOUT 10		   /* milliseconds */
#define STIR_IDLE_PERIODS 700		   /* roughly 7 second idle window */
#define STIR_MIN_SPEED_DELAY 75		   /* milliseconds */

#define STIR_MAX_ACTIVE_RX_URBS   1       /* Don't touch !!! */
#define STIR_MAX_RX_URBS  (STIR_MAX_ACTIVE_RX_URBS + 1)

#define USTR_WRITE_MULT   0x00         /* Val = Not used (0x0000), Idx = First register to write, Len = #regs */
#define USTR_READ_MULT    0x01         /* Val = Not used (0x0000), Idx = First register to read,  Len = #regs */
#define USTR_READ_ROM     0x02         /* Val = Not used (0x0000), Idx = Base ROM address (00-ff),  Len <= 64 */
#define USTR_WRITE_REG    0x03         /* Val = LSB contains data, Idx = Reg to write, Len = 0 */

                               /*     7         6          5         4          3         2          1         0   */
#define STREG_FIFODAT     0x00 /*                                     Reserved                                     */
#define STREG_MODE        0x01 /*     FIR    Reserved     SIR       ASK     FASTRXEN   FFRSTEN    FFSPRST PDCLK(8) */
#define STREG_BAUDRATE    0x02 /*                                   PDCLK(7: 0)                                    */
#define STREG_CTRL        0x03 /*  SDMODE    RXSLOW      DLOOP1    TXPWD      RXPWD       TXPWR(1: 0)       SRESET */
#define STREG_SENSITIVITY 0x04 /*          RXDSNS(2: 0)          BSTUFF SPWIDTH          ID(2)     ID(1)     ID(0) */
#define STREG_STATUS      0x05 /*  EOFRAME   FFUNDER     FFOVER    FFDIR      FFCLR    FFEMPTY    FFRXERR FFTXERR  */
#define STREG_FIFOCNT_L   0x06 /*                                    FFCNT(7:0)                                    */
#define STREG_FIFOCNT_H   0x07 /*     0         0          0                         FFCNT(12: 8)                  */
#define STREG_DPLLTUNE    0x08 /*                          DPCNT(5: 0)                               LONGP(1: 0)   */
#define STREG_IRDIG       0x09 /*   RXHIGH   TXLOW     Reserved Reserved Reserved      Reserved Reserved Reserved  */
#define STREG_TEST        0x0f /*  PLLDWN    LOOPIR LOOPUSB TSTENA                       TSTOSC(3: 0)              */

/* STREG_MODE */
#define SMF_FIR           0x80
#define SMF_SIR           0x20
#define SMF_ASK           0x10
#define SMF_FASTRXEN      0x08
#define SMF_FASTRSTEN     0x04
#define SMF_FFSPRST       0x02
#define SMF_PDCLK8        0x01

/* STREG_BAUDRATE */
#define PDCLK_4000000     0x002
#define PDCLK_115200      0x009
#define PDCLK_57600       0x013
#define PDCLK_38400       0x01D
#define PDCLK_19200       0x03B
#define PDCLK_9600        0x077
#define PDCLK_2400        0x1DF

/* STREG_CTRL */
#define SCF_SDMODE        0x80
#define SCF_RXSLOW        0x40
#define SCF_DLOOP1        0x20
#define SCF_TXPWD         0x10
#define SCF_RXPWD         0x08
#define SCF_TXPWR_LOW     0x06
#define SCF_TXPWR_MEDLOW  0x04
#define SCF_TXPWR_MEDHIGH 0x02
#define SCF_TXPWR_HIGH    0x00
#define SCF_SRESET        0x01

/* STREG_SENSITIVITY */
#define SSB_RXDSNS           5
#define SSF_BSTUFF        0x10
#define SSF_SPWIDTH       0x08
#define SSM_ID            0x07

/* STREG_STATUS */
#define SSF_EOF           0x80
#define SSF_FFUNDER       0x40
#define SSF_FFOVER        0x20
#define SSF_FFDIR         0x10
#define SSF_FFCLR         0x08
#define SSF_FFEMPTY       0x04
#define SSF_FFRXERR       0x02
#define SSF_FFTXERR       0x01

/* STREG_IRDIG */
#define SIF_RXHIGH        0x80
#define SIF_TXLOW         0x40

/* STREG_TEST */
#define STF_PLLDOWN       0x80
#define STF_LOOPIR        0x40
#define STF_LOOPUSB       0x20
#define STF_TSTENA        0x10
#define STM_TSTOSC        0x0f

#define DEFBUFFERSIZE 4096

struct NepSTIrDevBase
{
    struct Library      np_Library;       /* standard */
    UWORD               np_Flags;         /* various flags */

    BPTR                np_SegList;       /* device seglist */
    struct NepSTIr4200Base *np_ClsBase;    /* pointer to class base */
    struct Library     *np_UtilityBase;   /* utility base */
};

struct NepClassSTIr4200
{
    struct Unit         ncp_Unit;         /* Unit structure */
    ULONG               ncp_UnitNo;       /* Unit number */
    struct NepSTIr4200Base *ncp_ClsBase;  /* Up linkage */
    struct NepSTIrDevBase *ncp_DevBase;    /* Device base */
    struct Library     *ncp_Base;         /* Poseidon base */
    struct PsdDevice   *ncp_Device;       /* Up linkage */
    struct PsdConfig   *ncp_Config;       /* Up linkage */
    struct PsdInterface *ncp_Interface;   /* Up linkage */
    struct Task        *ncp_ReadySigTask; /* Task to send ready signal to */
    LONG                ncp_ReadySignal;  /* Signal to send when ready */
    struct Task        *ncp_Task;         /* Subtask */
    struct MsgPort     *ncp_TaskMsgPort;  /* Message Port of Subtask */

    struct PsdPipe     *ncp_EP0Pipe;      /* Endpoint 0 pipe */
    struct PsdEndpoint *ncp_EPOut;        /* Endpoint 1 */
    struct PsdPipe     *ncp_EPOutPipe;    /* Endpoint 1 pipe */
    struct PsdEndpoint *ncp_EPIn;         /* Endpoint 2 */
    struct PsdPipe     *ncp_EPInPipe;     /* Endpoint 2 pipe */
    IPTR                ncp_EPInMaxPktSize; /* Endpoint 2 max pkt size */
    struct MsgPort     *ncp_DevMsgPort;   /* Message Port for IOParReq */
    UWORD               ncp_UnitProdID;   /* ProductID of unit */
    UWORD               ncp_UnitVendorID; /* VendorID of unit */
    BOOL                ncp_DenyRequests; /* Do not accept further IO requests */
    UBYTE              *ncp_ReadBuffer;   /* Read Buffer Address */
    UBYTE              *ncp_WriteBuffer;  /* Read Buffer Address */
    BOOL                ncp_AbortRead;    /* if true, abort pending read */
    BOOL                ncp_AbortWrite;   /* if true, abort pending write */

    BOOL                ncp_FrameStarted; /* on receive: has frame started yet? */
    BOOL                ncp_UnescapeFirst; /* pending escape from previous packet */
    BOOL                ncp_FIRMode;      /* Fast Mode or slow mode */
    ULONG               ncp_BaudRate;     /* Last set baudrate */

    ULONG               ncp_RBufRemain;   /* Remaining bytes in read buffer */
    ULONG               ncp_RBufOffset;   /* Offset in readbuffer */

    struct IOIrDAReq   *ncp_ReadPending;  /* read IORequest pending */
    struct IOIrDAReq   *ncp_WritePending; /* write IORequest pending */
    struct List         ncp_ReadQueue;    /* List of read requests */
    struct List         ncp_WriteQueue;   /* List of write requests */

    STRPTR              ncp_DevIDString;  /* Device ID String */

    BOOL                ncp_UsingDefaultCfg;
    struct ClsDevCfg   *ncp_CDC;

    struct Library     *ncp_MUIBase;       /* MUI master base */
    struct Library     *ncp_PsdBase;       /* Poseidon base */
    struct Library     *ncp_IntBase;       /* Intuition base */
    struct Task        *ncp_GUITask;       /* GUI Task */
    struct NepClassHid *ncp_GUIBinding;    /* Window of binding that's open */

    Object             *ncp_App;
    Object             *ncp_MainWindow;

    Object             *ncp_UnitObj;
    Object             *ncp_StackAutoObj;

    Object             *ncp_UseObj;
    Object             *ncp_SetDefaultObj;
    Object             *ncp_CloseObj;

    Object             *ncp_AboutMI;
    Object             *ncp_UseMI;
    Object             *ncp_SetDefaultMI;
    Object             *ncp_MUIPrefsMI;

};

struct NepSTIr4200Base
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* utility base */

    struct NepSTIrDevBase *nh_DevBase;       /* base of device created */
    struct List         nh_Units;         /* List of units available */

    struct NepClassSTIr4200 nh_DummyNCP;     /* Dummy ncp for default config */
};


#endif /* STIR4200_H */
