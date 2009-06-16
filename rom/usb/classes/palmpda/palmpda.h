#ifndef PALMPDA_H
#define PALMPDA_H

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <exec/devices.h>

/* vendor and product IDs */

#define HANDSPRING_VENDOR_ID            0x082d
#define HANDSPRING_VISOR_ID             0x0100
#define HANDSPRING_TREO_ID              0x0200
#define HANDSPRING_TREO600_ID           0x0300

#define PALM_VENDOR_ID                  0x0830
#define PALM_M500_ID                    0x0001
#define PALM_M505_ID                    0x0002
#define PALM_M515_ID                    0x0003
#define PALM_I705_ID                    0x0020
#define PALM_M125_ID                    0x0040
#define PALM_M130_ID                    0x0050
#define PALM_TUNGSTEN_T_ID              0x0060
#define PALM_TUNGSTEN_Z_ID              0x0031
#define PALM_ZIRE31_ID                  0x0061
#define PALM_ZIRE_ID                    0x0070
#define PALM_M100_ID                    0x0080

#define SONY_VENDOR_ID                  0x054C
#define SONY_CLIE_3_5_ID                0x0038
#define SONY_CLIE_4_0_ID                0x0066
#define SONY_CLIE_S360_ID               0x0095
#define SONY_CLIE_4_1_ID                0x009A
#define SONY_CLIE_NX60_ID               0x00DA
#define SONY_CLIE_NZ90V_ID              0x00E9
#define SONY_CLIE_UX50_ID               0x0144
#define SONY_CLIE_TJ25_ID               0x0169

#define SAMSUNG_VENDOR_ID               0x04E8
#define SAMSUNG_SCH_I330_ID             0x8001
#define SAMSUNG_SPH_I500_ID             0x6601

#define GARMIN_VENDOR_ID                0x091E
#define GARMIN_IQUE_3600_ID             0x0004

#define ACEECA_VENDOR_ID                0x4766
#define ACEECA_MEZ1000_ID               0x0001

#define KYOCERA_VENDOR_ID               0x0C88
#define KYOCERA_7135_ID                 0x0021

#define FOSSIL_VENDOR_ID                0x0E67
#define FOSSIL_ABACUS_ID                0x0002

#define ZODIAC_VENDOR_ID                0x12ef
#define ZODIAC_ZODIAC_ID                0x0100

/****************************************************************************
 * VISOR_REQUEST_BYTES_AVAILABLE asks the visor for the number of bytes that
 * are available to be transferred to the host for the specified endpoint.
 * Currently this is not used, and always returns 0x0001
 ****************************************************************************/
#define UPR_REQUEST_BYTES_AVAILABLE           0x01

/****************************************************************************
 * VISOR_CLOSE_NOTIFICATION is set to the device to notify it that the host
 * is now closing the pipe. An empty packet is sent in response.
 ****************************************************************************/
#define UPR_CLOSE_NOTIFICATION                0x02

/****************************************************************************
 * VISOR_GET_CONNECTION_INFORMATION is sent by the host during enumeration to
 * get the endpoints used by the connection.
 ****************************************************************************/
#define UPR_GET_CONNECTION_INFORMATION        0x03

/****************************************************************************
 * PALM_GET_EXT_CONNECTION_INFORMATION is sent by the host during enumeration to
 * get some information from the M series devices, that is currently unknown.
 ****************************************************************************/
#define UPR_GET_EXT_CONNECTION_INFORMATION    0x04


struct ConnectInfo
{
    UWORD NumPorts;
    struct
    {
        UBYTE FctID;
        UBYTE Port;
    } Streams[2];
};

/**
 * struct palm_ext_connection_info - return data from a PALM_GET_EXT_CONNECTION_INFORMATION request
 * @num_ports: maximum number of functions/connections in use
 * @endpoint_numbers_different: will be 1 if in and out endpoints numbers are
 *      different, otherwise it is 0.  If value is 1, then
 *      connections.end_point_info is non-zero.  If value is 0, then
 *      connections.port contains the endpoint number, which is the same for in
 *      and out.
 * @port_function_id: contains the creator id of the applicaton that opened
 *      this connection.
 * @port: contains the in/out endpoint number.  Is 0 if in and out endpoint
 *      numbers are different.
 * @end_point_info: high nubbe is in endpoint and low nibble will indicate out
 *      endpoint.  Is 0 if in and out endpoints are the same.
 *
 * The maximum number of connections currently supported is 2
 */

struct ExtConnectInfo
{
    UBYTE NumPorts;
    UBYTE DiffEndPoints;
    UWORD Reserved0;
    struct
    {
        ULONG FctID;
        UBYTE Port;
        UBYTE EPInfo;
        UWORD Reserved0;
    } Streams[2];
};

/* struct visor_connection_info.connection[x].port_function_id defines: */
#define PALM_FUNCTION_GENERIC           0x00
#define PALM_FUNCTION_DEBUGGER          0x01
#define PALM_FUNCTION_HOTSYNC           0x02
#define PALM_FUNCTION_CONSOLE           0x03
#define PALM_FUNCTION_REMOTE_FILE_SYS   0x04

/* NetSync Header structure */
struct NetSyncHeader
{
  UBYTE nsh_Type;                       /* Type of Packet */
  UBYTE nsh_TransID;                    /* current transaction ID */
  ULONG nsh_DataSize;                   /* payload */
};


/* Misc */

#define DEFREADBUFLEN 2048
#define NUMREADPIPES 2

#define ID_ABOUT        0x55555555
#define ID_STORE_CONFIG 0xaaaaaaaa

struct ClsGlobalCfg
{
    ULONG cgc_ChunkID;
    ULONG cgc_Length;
    ULONG cgc_ShellStack;
    char  cgc_ShellCon[128];
    char  cgc_Command[256];
    char  cgc_InhibitTask[64];
};

struct NepSerialBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */

    struct Library     *nh_UtilityBase;   /* utility base */

    struct NepSerDevBase *nh_DevBase;     /* base of device created */
    struct List         nh_Units;         /* List of units available */

    struct Library     *nh_MUIBase;       /* MUI master base */
    struct Library     *nh_PsdBase;       /* Poseidon base */
    struct Library     *nh_IntBase;       /* Intuition base */
    struct Task        *nh_GUITask;       /* GUI Task */

    BOOL                nh_UsingDefaultCfg;
    struct ClsGlobalCfg nh_CurrentCGC;

    Object             *nh_App;
    Object             *nh_MainWindow;
    Object             *nh_ConWindowObj;
    Object             *nh_ShellStackObj;
    Object             *nh_ShellComObj;
    Object             *nh_InhibitTaskObj;
    Object             *nh_UseObj;
    Object             *nh_CloseObj;

    Object             *nh_AboutMI;
    Object             *nh_UseMI;
    Object             *nh_MUIPrefsMI;
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

    struct ConnectInfo  ncp_ConnectInfo;  /* Palm OS3 connection info */
    struct ExtConnectInfo ncp_ExtConnectInfo; /* Palm OS4 connection info */
    struct PsdPipe     *ncp_EP0Pipe;      /* Endpoint 0 pipe */
    UWORD               ncp_HotsyncPort;  /* Number of the hotsync port */
    UWORD               ncp_EPOutNum;     /* Number of the hotsync OUT EP */
    UWORD               ncp_EPInNum;      /* Number of the hotsync IN EP */
    struct PsdEndpoint *ncp_EPOut;        /* OUT Endpoint */
    struct PsdPipeStream *ncp_EPOutStream; /* OUT Endpoint stream */
    struct PsdEndpoint *ncp_EPIn;         /* IN Endpoint */
    struct PsdPipeStream *ncp_EPInStream; /* IN Endpoint stream */
    struct MsgPort     *ncp_DevMsgPort;   /* Message Port for IOParReq */
    UWORD               ncp_UnitProdID;   /* ProductID of unit */
    UWORD               ncp_UnitVendorID; /* VendorID of unit */
    UWORD               ncp_UnitCfgNum;   /* Config of unit */
    UWORD               ncp_UnitIfNum;    /* Interface number */
    UWORD               ncp_UnitAltIfNum; /* Alternate interface number */
    BOOL                ncp_DenyRequests; /* Do not accept further IO requests */
    BOOL                ncp_DevSuspend;   /* suspend things */
    BOOL                ncp_IsConfigured; /* have the parameters been set yet? */

    UWORD               ncp_TransIX;      /* Transfer IX code */

    struct IOExtSer    *ncp_WritePending; /* write IORequest pending */
    struct List         ncp_ReadQueue;    /* List of read requests */
    struct List         ncp_WriteQueue;   /* List of write requests */
};

#endif /* PALMPDA_H */
