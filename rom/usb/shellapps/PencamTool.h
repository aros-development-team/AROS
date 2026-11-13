#ifndef PENCAMTOOL_H
#define PENCAMTOOL_H

/* pencam vendor specific requests */

#define CMDID_CLEAR_COMMS_ERROR  0x00    /* -    0     -    0      0       */
#define CMDID_WRITE_CTRLREG      0x01    /* Tx   1-16  -    0-255  0-3     */
#define CMDID_WRITE_SDRAM        0x02    /* Tx   1-16  -    0-1    0       */
#define CMDID_UPLOAD_SDRAM       0x03    /* Tx   8     Yes  0      0       */
#define CMDID_CANCEL_TRANSACTION 0x04    /* -    0     -    0      0       */
#define CMDID_GRAB_IMAGE         0x05    /* Tx   0     -    0-255  0-255   */
#define CMDID_SET_IMAGE_INDEX    0x06    /* -    0     -    0-15   0-255   */
#define CMDID_SET_CAMERA_MODE    0x07    /* Tx   8     -    0-255  0       */
#define CMDID_TEST_CONTROL       0x08    /* Tx   0-8   -    1-4    0-255   */
#define CMDID_START_VIDEO        0x09    /* -    -     Yes  0-3    0       */
#define CMDID_STOP_VIDEO         0x0A    /* -    0     -    0      0       */
#define CMDID_ERASE_FLASH        0x0B    /* -    0     -    0-31   0       */
#define CMDID_PROGRAM_FLASH      0x0C    /* Tx   1-16  -    64-127 0-255   */
#define CMDID_SET_AEC_MODE       0x0D    /* Tx   0     -    0-3    0       */
#define CMDID_SET_CLKDIV         0x0E    /* Tx   0     -    0-15   0       */
#define CMDID_SET_EXPOSURE       0x0F    /* Tx   2     -    0-255  0-255   */
#define CMDID_SET_GAIN           0x10    /* Tx   0     -    0-255  0       */
#define CMDID_SET_GAIN_AND_EXP   0x11    /* Tx   4     -    0-255  0       */
#define CMDID_GET_LAST_ERROR     0x80    /* Rx   2     -    0      0       */
#define CMDID_READ_CTRLREG       0x81    /* Rx   1-16  -    0-255  0-3     */
#define CMDID_READ_SDRAM         0x82    /* Rx   1-16  -    0-1    0       */
#define CMDID_UPLOAD_IMAGE       0x83    /* Rx   16    Yes  0-15   0-255   */
#define CMDID_UPLOAD_THUMBNAIL   0x84    /* Rx   16    Yes  0-15   0-255   */
#define CMDID_GET_CAMERA_INFO    0x85    /* Rx   16    -    0      0       */
#define CMDID_GET_IMAGE_INFO     0x86    /* Rx   16    -    0      0       */
#define CMDID_GET_CAMERA_MODE    0x87    /* Rx   8     -    0      0       */
#define CMDID_PING               0x88    /* Rx   2     -    0-255  0-255   */
#define CMDID_GRAB_UPLOAD        0x89    /* Rx   8     Yes  0-255  0-255   */
#define CMDID_GET_COLDATA_SIZE   0x8A    /* Rx   2     -    0      0       */
#define CMDID_GET_COLDATA        0x8B    /* Rx   0-255 -    0      0       */
#define CMDID_GET_BUTTON_INFO    0x8C    /* Rx   1     -    0      0       */
#define CMDID_GET_USER_IO        0x8D    /* Rx   8     -    0      0       */
#define CMDID_READ_FLASH         0x8E    /* Rx   1-16  -    64-127 0-255   */
#define CMDID_GET_IMAGE_HEADER   0x8F    /* Rx   16    -    0-15   0-255   */
#define CMDID_GET_TABLE_ENTRY    0x90    /* Rx   16    -    0-15   0-255   */
#define CMDID_GET_ENTRY_POINTS   0x91    /* Rx   16    -    0      0       */

#define CAMERR_OK                         0
#define CAMERR_BUSY                       1
#define CAMERR_TIMEOUT                    2
#define CAMERR_V2W_ERROR                  3
#define CAMERR_COMMS_ERROR                4
#define CAMERR_BAD_EXPOSURE               5
#define CAMERR_BAD_INDEX                  6
#define CAMERR_CAMERA_FULL                7
#define CAMERR_BAD_COMMAND                8
#define CAMERR_BAD_PARAM                  9
#define CAMERR_BAD_DATALEN               10
#define CAMERR_TASK_FAILED               11
#define CAMERR_FLASH_PROGRAM_FAILED      12
#define CAMERR_BAD_ADDRESS               13
#define CAMERR_BAD_PAGE                  14
#define CAMERR_EXISTING_IMAGE_SMALLER    15
#define CAMERR_COMMAND_NOT_ALLOWED       16
#define CAMERR_NO_SENSOR_DETECTED        17
#define CAMERR_COLOUR_MATRIX_UNAVAILABLE 18

#define HWCF_SERIAL    0x01 /* !HWCF_USB */
#define HWCF_60HZ      0x02 /* !HWCF_50HZ */
#define HWCF_16MBIT    0x04 /* !HWCF_64MBIT */
#define HWCF_THUMBS    0x08 /* !HWCF_NOTHUMBS */
#define HWCF_VIDEO     0x10 /* !HWCF_NOVIDEO */
#define HWCF_READY     0x20 /* !HWCF_READY */
#define HWCF_MONO      0x40 /* !HWCF_COLOUR */
#define HWCF_MEMAVAIL  0x80 /* !HWCF_NOMEM */

#define ICF_CIF        0x01 /* 352*288 */
#define ICF_VGA        0x02 /* 640x480 */
#define ICF_QCIF       0x04 /* 176*144 */
#define ICF_QVGA       0x08 /* 320*240 */

struct PCImageHeader
{
    ULONG pcih_ImgSize;   /* Image data size in bytes */
    UWORD pcih_ImgWidth;  /* Image width in pixels */
    UWORD pcih_ImgHeight; /* Image height in pixels */
    UWORD pcih_FineExp;   /* Sensor FINE exposure */
    UWORD pcih_CoarseExp; /* Sensor COARSE exposure */
    UBYTE pcih_Gain;      /* Sensor GAIN */
    UBYTE pcih_ClkDiv;    /* Sensor CLKDIV */
    UBYTE pcih_AvgPixVal; /* Average pixel value (g_Ap) */
    UBYTE pcih_Flags;     /* Image flags */
};

struct PCImageInfo
{
    UWORD pcii_ImgIndex;  /* Current image index */
    UWORD pcii_MaxIndex;  /* Max images */
    UWORD pcii_ImgWidth;  /* Image width (in pixels) */
    UWORD pcii_ImgHeight; /* Image height (in pixels) */
    ULONG pcii_ImgSize;   /* Image size (in bytes) */
    UBYTE pcii_TbnWidth;  /* Thumbnail width (in pixels) */
    UBYTE pcii_TbnHeight; /* Thumbnail height (in pixels) */
    UWORD pcii_TbnSize;   /* Thumbnail size (in bytes) */
};

struct NepClassPencam
{
    struct Node         nch_Node;         /* Node linkage */
    struct PsdDevice   *nch_Device;       /* Up linkage */
    struct PsdConfig   *nch_Config;       /* Up linkage */
    struct PsdInterface *nch_Interface;   /* Up linkage */
    struct PsdPipe     *nch_EP0Pipe;      /* Endpoint 0 pipe */
    struct PsdEndpoint *nch_BulkEP;       /* Endpoint 2 */
    struct PsdPipe     *nch_BulkPipe;     /* Endpoint 2 pipe */
    IPTR               nch_BulkPktSize;  /* Size of EP1 packets */
    struct Task        *nch_Task;         /* This Task */
    struct MsgPort     *nch_TaskMsgPort;  /* Message Port of Subtask */
    struct Hook         nch_ReleaseHook;  /* Hook for release function */

    UWORD               nch_FWVers;       /* Firmware version */
    UWORD               nch_FWRev;        /* Firmware revision */
    UWORD               nch_ASICVers;     /* ASIC Version */
    UWORD               nch_ASICRev;      /* ASIC Revision */
    UWORD               nch_HWCaps;       /* Hardware capabilities */
    UWORD               nch_ImgCaps;      /* Image capabilities */
    UBYTE              *nch_RawBuf;       /* Pointer to raw buffer */
    ULONG               nch_RawBufSize;   /* Size of the allocated buffer */
};

#endif /* PENCAMTOOL_H */
