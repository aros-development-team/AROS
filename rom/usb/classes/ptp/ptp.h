#ifndef PTP_H
#define PTP_H

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>

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
    ULONG cdc_EnableMTP;
    char  cdc_DOSName[32];
    ULONG cdc_NoPartObj;
};

struct ObjectFmtMapping
{
    UWORD  ofm_ID;
    STRPTR ofm_Suffix;
};

/* Container Types */
#define PCT_COMMAND  1
#define PCT_DATA     2
#define PCT_RESPONSE 3
#define PCT_EVENT    4

struct PTPEvent
{
    ULONG pe_Length;        /* Interrupt Data Length */
    UWORD pe_ContainerType; /* Container Type == Event */
    UWORD pe_EventCode;     /* The PIMA 15740 Event Code */
    ULONG pe_TransID;       /* Transaction ID */
    ULONG pe_EventParam[3]; /* Event Parameters */
};

struct PTPOp
{
    ULONG po_Length;        /* Length of request */
    UWORD po_ContainerType; /* Container Type */
    UWORD po_OpCode;        /* Operation Code */
    //ULONG po_SessionID;     /* Session ID */
    ULONG po_TransID;       /* Transaction ID */
    ULONG po_Param[5];      /* Parameters */

    ULONG po_NumParam;
};

struct PTPResponse
{
    ULONG pr_Length;        /* Length of request */
    UWORD pr_ContainerType; /* Container Type */
    UWORD pr_RespCode;      /* Response Code */
    //ULONG pr_SessionID;     /* Session ID */
    ULONG pr_TransID;       /* Transaction ID */
    ULONG pr_Param[5];      /* Parameters */

    LONG  pr_IOErr;
    ULONG pr_NumParam;

    ULONG pr_DataLength;
};
 

/* PTP Operation Codes */
#define POC_UNDEFINED                             0x1000
#define POC_GETDEVICEINFO                         0x1001
#define POC_OPENSESSION                           0x1002
#define POC_CLOSESESSION                          0x1003
#define POC_GETSTORAGEIDS                         0x1004
#define POC_GETSTORAGEINFO                        0x1005
#define POC_GETNUMOBJECTS                         0x1006
#define POC_GETOBJECTHANDLES                      0x1007
#define POC_GETOBJECTINFO                         0x1008
#define POC_GETOBJECT                             0x1009
#define POC_GETTHUMB                              0x100A
#define POC_DELETEOBJECT                          0x100B
#define POC_SENDOBJECTINFO                        0x100C
#define POC_SENDOBJECT                            0x100D
#define POC_INITIATECAPTURE                       0x100E
#define POC_FORMATSTORE                           0x100F
#define POC_RESETDEVICE                           0x1010
#define POC_SELFTEST                              0x1011
#define POC_SETOBJECTPROTECTION                   0x1012
#define POC_POWERDOWN                             0x1013
#define POC_GETDEVICEPROPDESC                     0x1014
#define POC_GETDEVICEPROPVALUE                    0x1015
#define POC_SETDEVICEPROPVALUE                    0x1016
#define POC_RESETDEVICEPROPVALUE                  0x1017
#define POC_TERMINATEOPENCAPTURE                  0x1018
#define POC_MOVEOBJECT                            0x1019
#define POC_COPYOBJECT                            0x101A
#define POC_GETPARTIALOBJECT                      0x101B
#define POC_INITIATEOPENCAPTURE                   0x101C

/* PTP Result Codes */
#define PRC_UNDEFINED                             0x2000
#define PRC_OK                                    0x2001
#define PRC_GENERALERROR                          0x2002
#define PRC_SESSIONNOTOPEN                        0x2003
#define PRC_INVALIDTRANSACTIONID                  0x2004
#define PRC_OPERATIONNOTSUPPORTED                 0x2005
#define PRC_PARAMETERNOTSUPPORTED                 0x2006
#define PRC_INCOMPLETETRANSFER                    0x2007
#define PRC_INVALIDSTORAGEID                      0x2008
#define PRC_INVALIDOBJECTHANDLE                   0x2009
#define PRC_DEVICEPROPNOTSUPPORTED                0x200A
#define PRC_INVALIDOBJECTFORMATCODE               0x200B
#define PRC_STOREFULL                             0x200C
#define PRC_OBJECTWRITEPROTECTED                  0x200D
#define PRC_STOREREADONLY                         0x200E
#define PRC_ACCESSDENIED                          0x200F
#define PRC_NOTHUMBNAILPRESENT                    0x2010
#define PRC_SELFTESTFAILED                        0x2011
#define PRC_PARTIALDELETION                       0x2012
#define PRC_STORENOTAVAILABLE                     0x2013
#define PRC_SPECIFICATIONBYFORMATUNSUPPORTED      0x2014
#define PRC_NOVALIDOBJECTINFO                     0x2015
#define PRC_INVALIDCODEFORMAT                     0x2016
#define PRC_UNKNOWNVENDORCODE                     0x2017
#define PRC_CAPTUREALREADYTERMINATED              0x2018
#define PRC_DEVICEBUSY                            0x2019
#define PRC_INVALIDPARENTOBJECT                   0x201A
#define PRC_INVALIDDEVICEPROPFORMAT               0x201B
#define PRC_INVALIDDEVICEPROPVALUE                0x201C
#define PRC_INVALIDPARAMETER                      0x201D
#define PRC_SESSIONALREADYOPEN                    0x201E
#define PRC_TRANSACTIONCANCELLED                  0x201F
#define PRC_SPECIFICATIONOFDESTINATIONUNSUPPORTED 0x2020

/* PTP Object Format Codes */
#define POF_UNDEFINED                             0x3000
#define POF_ASSOCIATION                           0x3001
#define POF_SCRIPT                                0x3002
#define POF_EXECUTABLE                            0x3003
#define POF_TEXT                                  0x3004
#define POF_HTML                                  0x3005
#define POF_DPOF                                  0x3006
#define POF_AIFF                                  0x3007
#define POF_WAV                                   0x3008
#define POF_MP3                                   0x3009
#define POF_AVI                                   0x300A
#define POF_MPEG                                  0x300B
#define POF_ASF                                   0x300C
#define POF_UNDEFINED2                            0x3800
#define POF_EXIF_JPEG                             0x3801
#define POF_TIFF_EP                               0x3802
#define POF_FLASHPIX                              0x3803
#define POF_BMP                                   0x3804
#define POF_CIFF                                  0x3805
#define POF_UNDEFINED3                            0x3806
#define POF_GIF                                   0x3807
#define POF_JFIF                                  0x3808
#define POF_PCD                                   0x3809
#define POF_PICT                                  0x380A
#define POF_PNG                                   0x380B
#define POF_UNDEFINED4                            0x380C
#define POF_TIFF                                  0x380D
#define POF_TIFF_IT                               0x380E
#define POF_JP2                                   0x380F
#define POF_JPX                                   0x3810

/* PTP Event Codes */
#define PEC_UNDEFINED                             0x4000
#define PEC_CANCELTRANSACTION                     0x4001
#define PEC_OBJECTADDED                           0x4002
#define PEC_OBJECTREMOVED                         0x4003
#define PEC_STOREADDED                            0x4004
#define PEC_STOREREMOVED                          0x4005
#define PEC_DEVICEPROPCHANGED                     0x4006
#define PEC_OBJECTINFOCHANGED                     0x4007
#define PEC_DEVICEINFOCHANGED                     0x4008
#define PEC_REQUESTOBJECTTRANSFER                 0x4009
#define PEC_STOREFULL                             0x400A
#define PEC_DEVICERESET                           0x400B
#define PEC_STORAGEINFOCHANGED                    0x400C
#define PEC_CAPTURECOMPLETE                       0x400D
#define PEC_UNREPORTEDSTATUS                      0x400E

/* PTP Device Property Codes */
#define PDP_UNDEFINED                             0x5000
#define PDP_BATTERYLEVEL                          0x5001
#define PDP_FUNCTIONALMODE                        0x5002
#define PDP_IMAGESIZE                             0x5003
#define PDP_COMPRESSIONSETTING                    0x5004
#define PDP_WHITEBALANCE                          0x5005
#define PDP_RGBGAIN                               0x5006
#define PDP_FNUMBER                               0x5007
#define PDP_FOCALLENGTH                           0x5008
#define PDP_FOCUSDISTANCE                         0x5009
#define PDP_FOCUSMODE                             0x500A
#define PDP_EXPOSUREMETERINGMODE                  0x500B
#define PDP_FLASHMODE                             0x500C
#define PDP_EXPOSURETIME                          0x500D
#define PDP_EXPOSUREPROGRAMMODE                   0x500E
#define PDP_EXPOSUREINDEX                         0x500F
#define PDP_EXPOSUREBIASCOMPENSATION              0x5010
#define PDP_DATETIME                              0x5011
#define PDP_CAPTUREDELAY                          0x5012
#define PDP_STILLCAPTUREMODE                      0x5013
#define PDP_CONTRAST                              0x5014
#define PDP_SHARPNESS                             0x5015
#define PDP_DIGITALZOOM                           0x5016
#define PDP_EFFECTMODE                            0x5017
#define PDP_BURSTNUMBER                           0x5018
#define PDP_BURSTINTERVAL                         0x5019
#define PDP_TIMELAPSENUMBER                       0x501A
#define PDP_TIMELAPSEINTERVAL                     0x501B
#define PDP_FOCUSMETERINGMODE                     0x501C
#define PDP_UPLOADURL                             0x501D
#define PDP_ARTIST                                0x501E
#define PDP_COPYRIGHTINFO                         0x501F

#if defined(__GNUC__)
# pragma pack()
#endif

#define PTPF_SHARED_LOCK 0x0001 /* Is shared locked */
#define PTPF_EXCL_LOCK   0x0002 /* Is exclusively locked */
#define PTPF_FETCHED     0x4000 /* Directory contents have been fetched */
#define PTPF_NOPURGE     0x8000 /* Don't ever purge this entry */

// quit flags
#define PTPF_DEVNODE 0x0001
#define PTPF_VOLNODE 0x0002
#define PTPF_LOCKS   0x0004
#define PTPF_FHS     0x0008

struct PTPObjectInfo
{
    struct MinNode poi_Node;      /* Linkage */
    struct PTPObjectInfo *poi_Parent; /* Up Linkage */

    UWORD       poi_Flags;        /* State flags */
    UWORD       poi_NameHash;     /* Name Hash */
    UWORD       poi_ReadLocks;    /* Number of read locks on this file */

    ULONG       poi_Handle;       /* ObjectHandle */
    ULONG       poi_ParentHandle; /* Parent Object */
    STRPTR      poi_Name;         /* Filename */

    ULONG       poi_StorageID;    /* Storage ID */
    UWORD       poi_ObjectFmt;    /* Object Format Code */
    UWORD       poi_ProtFlags;    /* Protection Flags */
    ULONG       poi_Size;         /* Object Compressed Size */

    ULONG       poi_ThumbSize;    /* Thumb Compressed Sized */
    UWORD       poi_ThumbFmt;     /* Object Format Code of Thumb */
    UWORD       poi_ThumbWidth;   /* Thumb Pix Width */
    UWORD       poi_ThumbHeight;  /* Thumb Pix Height */
    UWORD       poi_ImageWidth;   /* Image Pix Width */
    UWORD       poi_ImageHeight;  /* Image Pix Height */
    UWORD       poi_ImageDepth;   /* Image Bit Depth */

    //UWORD       poi_AssType;      /* Association Type */
    //ULONG       poi_AssDesc;      /* Association Desc */
    //ULONG       poi_SeqNumber;    /* Sequence Number */

    STRPTR      poi_Keywords;     /* Keywords -> comments field */

    struct DateStamp poi_CapDate; /* Capture Date */
    struct DateStamp poi_ModDate; /* Modifcation Date */

    struct MinList poi_Children;  /* Child objects */
};

struct PTPStorageInfo
{
    struct Node psi_Node;         /* Node linkage */
    ULONG       psi_StorageID;    /* Storage ID */

    UWORD       psi_StorageType;  /* Storage Type */
    UWORD       psi_FSType;       /* Filesystem Type */
    UWORD       psi_AccessCaps;   /* Access Capability */
    ULONG       psi_MaxCapacityH; /* Maximum capacity (HIGH 32-bits) */
    ULONG       psi_MaxCapacityL; /* Maximum capacity (LOW 32-bits) */
    ULONG       psi_FreeH;        /* Free Space in Bytes (HIGH 32-bits) */
    ULONG       psi_FreeL;        /* Free Space in Bytes (LOW 32-bits) */
    ULONG       psi_FreeImgs;     /* Free Space in Images */
    STRPTR      psi_StorageDesc;  /* Storage Description */
    STRPTR      psi_VolumeName;   /* Volume Label */

    struct PTPObjectInfo *psi_ObjectInfo; /* Linked ObjectInfo */
};

// file handle flags
#define PTPF_NEWFILE   0x0001
#define PTPF_OLDLOADED 0x0002
#define PTPF_MODIFIED  0x0004

struct PTPFileHandle
{
    struct Node         pfh_Node;         /* Node linkage */
    struct PTPObjectInfo *pfh_ObjectInfo; /* Object Info */
    UBYTE              *pfh_Buffer;       /* Buffer of whole file */
    ULONG               pfh_BufferLen;    /* Size of buffer */
    ULONG               pfh_SeekPos;      /* Current Seeking Position (LOW 32-bits) */
    UWORD               pfh_Flags;        /* Flags */
};

#define PTPF_SENDOBJ   0x0001 /* Send Object possible */
#define PTPF_PARTIAL   0x0002 /* GetPartialObject possible */
#define PTPF_DELETEOBJ 0x0004 /* DeleteObject available */
#define PTPF_PROTOBJ   0x0008 /* SetObjectProtection available */
#define PTPF_MOVEOBJ   0x0010 /* Move Object */
#define PTPF_FORMAT    0x0020 /* Format Store */

struct NepClassPTP
{
    struct Node         nch_Node;         /* Node linkage */
    struct NepPTPBase  *nch_ClsBase;      /* Up linkage */
    struct Library     *nch_Base;         /* Poseidon base */
    struct Library     *nch_DOSBase;      /* DOS Base */
    struct PsdDevice   *nch_Device;       /* Up linkage */
    struct PsdConfig   *nch_Config;       /* Up linkage */
    struct PsdInterface *nch_Interface;   /* Up linkage */
    struct PsdEndpoint *nch_EPIn;         /* IN Endpoint */
    struct PsdEndpoint *nch_EPOut;        /* OUT Endpoint */
    struct PsdEndpoint *nch_EPInt;        /* INT Endpoint */
    IPTR                nch_EPInPktSize;  /* Size of EP IN packets */
    struct PsdPipe     *nch_EP0Pipe;      /* Endpoint 0 pipe */
    struct PsdPipe     *nch_EPInPipe;     /* Endpoint In pipe */
    struct PsdPipe     *nch_EPOutPipe;    /* Endpoint Out pipe */
    struct PsdPipe     *nch_EPIntPipe;    /* Endpoint Int pipe */
    struct Task        *nch_ReadySigTask; /* Task to send ready signal to */
    LONG                nch_ReadySignal;  /* Signal to send when ready */
    struct Task        *nch_Task;         /* Subtask */
    struct MsgPort     *nch_TaskMsgPort;  /* Message Port for Pipe */
    struct MsgPort     *nch_DOSMsgPort;   /* DOS "task" for DOS Packets */
    IPTR                nch_IfNum;        /* Interface number */

    APTR                nch_MemPool;      /* Private Memory Pool */

    BOOL                nch_ShallExit;    /* shall go down */
    UWORD               nch_ResFlags;     /* Resource flags */

    struct DosList     *nch_DevEntry;     /* DOS List Entry for Device */
    struct DosList     *nch_VolEntry;     /* DOS List Entry for Volume */

    struct List         nch_Storages;     /* ROOT: List of storages */
    struct PTPObjectInfo nch_RootObject;  /* ROOT object */

    struct List         nch_FHs;          /* List of FileHandles */

    struct PTPObjectInfo *nch_LastObject; /* Last object loaded */
    UBYTE              *nch_LastBuffer;   /* Pointer to last loaded object */
    ULONG               nch_LastBufferLen; /* Length of buffer */

    ULONG               nch_SessionID;    /* Session ID */
    ULONG               nch_TransID;      /* Transaction ID */
    UWORD               nch_DevCaps;      /* Device support flags */
    UWORD               nch_NoDestMode;   /* Specification of Destination unsupported */

    STRPTR              nch_DevIDString;  /* Device ID String */
    STRPTR              nch_IfIDString;   /* Interface ID String */

    BOOL                nch_UsingDefaultCfg;
    struct ClsDevCfg   *nch_CDC;

    struct Library     *nch_MUIBase;      /* MUI master base */
    struct Library     *nch_PsdBase;      /* Poseidon base */
    struct Library     *nch_IntBase;      /* Intuition base */
    struct Task        *nch_GUITask;      /* GUI Task */
    struct NepClassPTP *nch_GUIBinding;   /* Window of binding that's open */

    Object             *nch_App;
    Object             *nch_MainWindow;
    Object             *nch_DOSNameObj;
    Object             *nch_EnableMTPObj;
    Object             *nch_NoPartObjObj;

    Object             *nch_UseObj;
    Object             *nch_SetDefaultObj;
    Object             *nch_CloseObj;

    Object             *nch_AboutMI;
    Object             *nch_UseMI;
    Object             *nch_SetDefaultMI;
    Object             *nch_MUIPrefsMI;

    UBYTE               nch_InBuf[512];   /* Buffer for reading in first USB packet */
    UBYTE               nch_OutBuf[1024]; /* Buffer for writing of send object info */
    UBYTE               nch_VolumeName[32]; /* Volume Name */
    UBYTE               nch_LowCharMap[256]; /* International Mapping to lower case */
};

struct NepPTPBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* Utility base */

    struct List         nh_Bindings;      /* List of bindings created */

    struct NepClassPTP  nh_DummyNCH;      /* Dummy NCH for default config */
};

#endif /* PTP_H */
