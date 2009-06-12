#ifndef SONIXCAMTOOL_H
#define SONIXCAMTOOL_H

/* sonixcam vendor specific requests */

#define CMDID_READ_REG           0x00
#define CMDID_WRITE_REG          0x08

#define SXREG_CHIPID             0x00 /* chip ID (either 0x10/0x11/0x12) */
#define SXREG_CTRL               0x01 /* 0x04 enables, 0x01 resets (probably) */
#define SXREG_SOMECTRL           0x02 /* 0x44 */
#define SXREG_I2C_CTRL           0x08 /* I2C Length in bits [6:4], bit 7 is 2WIRE */
#define SXREG_I2C_ADDRESS        0x09 /* I2C Target Address */
#define SXREG_I2C_REG            0x0a /* I2C register index */
#define SXREG_I2C_DATA0          0x0b /* I2C value byte 0 */
#define SXREG_I2C_DATA1          0x0c /* I2C value byte 1 */
#define SXREG_I2C_DATA2          0x0d /* I2C value byte 2 */
#define SXREG_I2C_DATA3          0x0e /* I2C value byte 3 */
#define SXREG_I2C_TERM           0x0f /* I2C termination 0x17 */

#define SXREG_WINDOW_LEFT        0x12 /* left start offset of window */
#define SXREG_WINDOW_TOP         0x13 /* top start offset of window */
#define SXREG_XXX                0x14
#define SXREG_WINDOW_WIDTH       0x15 /* width/16 of window */
#define SXREG_WINDOW_HEIGHT      0x16 /* height/16 of window */
#define SXREG_VID_CTRL_1         0x17 /* some control (0x28 on init), 0x01 low quality compression */
#define SXREG_VID_CTRL_2         0x18 /* some control, 0x80 enables compression, 0x10 = 1/2, 0x20 = 1/4 */
#define SXREG_VID_CTRL_3         0x19 /* some control, 0x20 enables compression, 0x50 disables */

#define BRIDGE_SN9C102           0x0102
#define BRIDGE_SN9C103           0x0103
#define BRIDGE_SN9C105           0x0105
#define BRIDGE_SN9C120           0x0120

struct SCImageHeader
{
    ULONG scih_ImgSize;   /* Image data size in bytes */
    UWORD scih_ImgWidth;  /* Image width in pixels */
    UWORD scih_ImgHeight; /* Image height in pixels */
};

#define BUF_EMPTY  0 /* Buffer may be reused */
#define BUF_READY  1 /* Buffer is filled */
#define BUF_BUSY   2 /* bayer in progress */

struct NepClassSonixcam
{
    struct Node         nch_Node;         /* Node linkage */
    struct PsdDevice   *nch_Device;       /* Up linkage */
    struct PsdInterface *nch_Interface;   /* Up linkage */
    struct PsdPipe     *nch_EP0Pipe;      /* Endpoint 0 pipe */
    struct PsdEndpoint *nch_IsoEP;        /* Endpoint 2 */
    struct PsdRTIsoHandler *nch_RTIso;    /* RT Iso */
    struct Task        *nch_Task;         /* This Task */
    struct MsgPort     *nch_TaskMsgPort;  /* Message Port of Subtask */
    struct Hook         nch_ReleaseHook;  /* Hook for release function */

    UWORD               nch_BridgeID;     /* Bridge ID */

    IPTR                nch_IfNum;        /* Interface number (usually 0, but who knows... */
    LONG                nch_ImgDoneSig;   /* Image Done signal number */

    struct Hook         nch_InReqHook;    /* Hook for input */
    struct Hook         nch_InDoneHook;   /* Hook for input */

    UWORD               nch_I2CAddr;      /* Sensor I2C Address */
    UWORD               nch_ImgCaps;      /* Image capabilities */
    UBYTE              *nch_RawBuf[3];    /* Triple buffering for iso data */
    ULONG               nch_RawBufSize;   /* Size of each allocated buffer */

    ULONG               nch_HeaderSize;   /* Size of header according to bridge */

    UWORD               nch_NextBufNum;   /* Number of next buffer to view */
    UWORD               nch_LastDoneNum;  /* Number of last ready buffer */
    UWORD               nch_BufState[3];  /* Buffer States */

    UWORD               nch_IsoBufNum;    /* Buffer currently used by iso transfer */
    UBYTE              *nch_CurrIsoBuf;   /* Current ISO buffer */
    ULONG               nch_IsoBufPos;    /* Offset in Iso Buffer */

    UBYTE              *nch_ImgBuf;       /* RGB unbayered image */
    ULONG               nch_ImgBufSize;   /* Size of RGB image */

    UBYTE               nch_Reg[256];     /* Shadow of registers */
    ULONG               nch_FrameCnt;
};

#endif /* SONIXCAMTOOL_H */
