#ifndef USBAUDIO_H
#define USBAUDIO_H

#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <libraries/gadtools.h>
#include <devices/ahi.h>

#define ID_ABOUT        0x55555555
#define ID_STORE_CONFIG 0xaaaaaaaa

#define AHI_SUB_LIB_VERSION 4
#define AHI_USB_MODE_BASE 0x003b0000

#define AHIDB_NepAudioMode (AHIDB_UserBase+0)

#define UAF_MASTER_VOLUME  0x0001
#define UAF_INPUT_GAIN     0x0002
#define UAF_MONITOR_VOLUME 0x0004
#define UAF_SELECT_INPUT   0x0100
#define UAF_SELECT_OUTPUT  0x0200

struct ClsGlobalCfg
{
    ULONG cgc_ChunkID;
    ULONG cgc_Length;
};

struct NepAudioUnit
{
    struct Node         nau_Node;         /* Node linkage */
    UWORD               nau_Type;         /* Type as in USB audio spec */
    UWORD               nau_UnitID;       /* ID of this unit / input / output / feature */
    UWORD               nau_NumInputs;    /* Number of inputs */
    UWORD               nau_NumOutputs;   /* Number of outputs */
    struct NepAudioUnit *nau_InputUnit[8];/* Sourcing Units (or NULL for input terminals) */
    struct NepAudioUnit *nau_OutputUnit[8]; /* Outputting Units */
    UWORD               nau_OutChannels;  /* Number of outgoing channels */
    UWORD               nau_ChannelCfg;   /* Spacial location */
    UWORD               nau_TermType;     /* Terminal Type */
    UBYTE              *nau_Descriptor;   /* Raw descriptor */
    STRPTR              nau_Name;         /* Name of unit */
    struct NepAudioUnit *nau_RootUnit;    /* Top of the chain */
    struct NepAudioUnit *nau_SinkUnit;    /* USB Streaming unit this goes to */
    struct NepAudioUnit *nau_SourceUnit;  /* USB Streaming unit this comes from */

    // only for root
    struct NepAudioUnit *nau_VolumeUnit;  /* Volume control unit */
    struct NepAudioUnit *nau_MonitorUnit; /* Monitor Volume unit */
    struct NepAudioUnit *nau_SelectorUnit; /* Input and output select unit */
    BOOL                nau_VolumeControl; /* Master volume control */
    BOOL                nau_Monitor;      /* Monitor volume control */
    BOOL                nau_Balance;      /* Balance Panning support */

    UWORD               nau_VolCtrlMask;  /* Feature Mask (master, left, right) */
    UWORD               nau_VolCtrlCount; /* Number of channels */

    WORD                nau_MinVolDb;     /* Min Volume in 8.8 DB */
    WORD                nau_MaxVolDb;     /* Max Volume in 8.8 DB */

    Fixed               nau_MinVolume;    /* Minimum volume for this unit */
    Fixed               nau_MaxVolume;    /* Maximum volume for this unit */
};

struct NepAudioMode
{
    struct Node         nam_Node;         /* Node linkage */
    struct NepAudioSubLibBase *nam_SubLibBase; /* usbaudio.audio AHI driver library */
    struct NepClassAudio *nam_Unit;       /* Up linkage */
    struct Library     *nam_PsdBase;      /* Poseidon base */
    struct NepAudioMode *nam_Sibling;     /* Associated Record/Playback node */
    struct PsdInterface *nam_Interface;   /* Interface for mode */
    struct PsdInterface *nam_ZeroBWIF;    /* Interface for without bandwidth use */
    struct PsdEndpoint *nam_EP;           /* Iso Endpoint */
    struct PsdRTIsoHandler *nam_RTIso;    /* Iso Pipe */
    struct PsdPipe     *nam_EP0Pipe;      /* Endpoint 0 pipe */
    struct MsgPort     *nam_TaskMsgPort;  /* Message Port of Subtask */

    ULONG               nam_IfNum;        /* Interface number */
    IPTR                nam_EPNum;        /* Endpoint number */

    struct NepAudioUnit *nam_RootUnit;    /* Root unit containing control information */
    ULONG               nam_AHIModeID;    /* AHI Mode ID */
    ULONG               nam_SampleType;   /* AHI Sample Type */
    UWORD               nam_AHIFrameSize; /* Number of bytes per AHI Frame */
    UWORD               nam_NumChannels;  /* Number of channels */
    UWORD               nam_FrameSize;    /* Number of bytes per Frame */
    UWORD               nam_Resolution;   /* Number of bits per Sample */
    UWORD               nam_SampleSize;   /* Number of bytes per Sample */
    UWORD               nam_TerminalID;   /* ID of the terminal */
    BOOL                nam_IsInput;      /* Is Input (recording) */
    BOOL                nam_HasVolume;    /* Has Volume control */
    BOOL                nam_HasPanning;   /* Supports panning */
    BOOL                nam_HasFreqCtrl;  /* Has Frequency control */
    BOOL                nam_HasPitchCtrl; /* Has Pitch control */
    UWORD               nam_CurrInput;    /* Current input */
    UWORD               nam_CurrOutput;   /* Current output */

    IPTR                nam_MaxPktSize;   /* Max Pkt Size of Endpoint */
    ULONG               nam_Interval;     /* Interval in frames */
    ULONG               nam_MinFreq;      /* Minimum Frequency */
    ULONG               nam_MaxFreq;      /* Maximum Frequency */
    ULONG               nam_NumFrequencies; /* Number of supported frequencies */
    ULONG               nam_FreqArray[64]; /* Array of supported frequencies */
    ULONG               nam_NumInputs;    /* Number of Inputs */
    STRPTR              nam_InputNames[8]; /* Names for the inputs */
    ULONG               nam_NumOutputs;   /* Number of Outputs */
    STRPTR              nam_OutputNames[8]; /* Names for the outputs */
    struct TagItem      nam_Tags[12];     /* AudioDB Tags */
    struct AHIAudioCtrlDrv *nam_AudioCtrl; /* AHI Audio Ctrl */

    LONG                nam_PlayerTimer;  /* Descending Timer when to call next player */
    Fixed               nam_PlayerInterval; /* Player interval (upper word is holds ms) */
    Fixed               nam_PlayerFrac;   /* Player increment */
    ULONG               nam_PlayerMS;     /* Number of milliseconds to mix before player gets called next time */
    struct Interrupt    nam_PlayerInt;    /* Interrupt to cause when Player needs to be called */

    BOOL                nam_FallbackTimer; /* Timer active? */
    struct MsgPort      nam_TimerMsgPort; /* Msg port for Timer device on device removal */
    struct timerequest *nam_TimerIOReq;   /* Timer Request */

    Fixed               nam_SampleFrac;   /* Number of samples per interval */
    Fixed               nam_BufferCount;  /* Increasing sample per interval counter */
    UWORD               nam_NextBufW;     /* 0/1 Next buffer to overwrite */
    UWORD               nam_NextBufR;     /* 0/1 Next buffer to read from */
    WORD               *nam_AHIBuffer;    /* AHI buffer */
    ULONG               nam_USBBufLen[2]; /* Length of each buffer */
    UBYTE              *nam_USBBuffer[2]; /* USB converted buffer (double buffered) */
    Fixed               nam_USBBufCnt[2]; /* Reloading of buffer count */
    LONG                nam_USBCount;     /* Decreasing buffer count -- if negative, start next buffer */
    struct AHIRecordMessage nam_RecMsg;   /* Record message */
    struct Hook         nam_SamConvHook;  /* Sample conversion hook */
    struct Hook         nam_ReleaseHook;  /* Device Release hook */

    Fixed               nam_MasterVol;    /* Master volume for the unit */
    Fixed               nam_ChannelVol[2]; /* Left and right channel */
    Fixed               nam_ChannelPan[2]; /* Channel panning */

    Fixed               nam_InputGain;    /* Input gain */
    Fixed               nam_MonitorVol;   /* Monitor volume */
};

struct NepAudioBase
{
    struct Library      nh_Library;       /* standard */
    UWORD               nh_Flags;         /* various flags */
    struct Library     *nh_UtilityBase;   /* utility base */
    struct NepAudioSubLibBase *nh_SubLibBase; /* usbaudio.audio AHI driver library */
    struct List         nh_Units;         /* List of bindings created */

    struct Library     *nh_MUIBase;       /* MUI master base */
    struct Library     *nh_PsdBase;       /* Poseidon base */
    struct Library     *nh_IntBase;       /* Intuition base */
    struct Task        *nh_GUITask;       /* GUI Task */

    struct ClsGlobalCfg nh_CurrentCGC;
    BOOL                nh_UsingDefaultCfg;

    Object             *nh_App;
    Object             *nh_MainWindow;
    Object             *nh_UseObj;
    Object             *nh_CloseObj;

    Object             *nh_AboutMI;
    Object             *nh_UseMI;
    Object             *nh_MUIPrefsMI;
};

struct NepClassAudio
{
    struct Node         nch_Node;         /* Node linkage */
    ULONG               nch_UnitNo;       /* Unit number */
    struct NepAudioBase *nch_ClsBase;     /* Up linkage */
    struct Library     *nch_Base;         /* Poseidon base */
    struct Library     *nch_AHIBase;      /* AHI Device base */
    struct PsdDevice   *nch_Device;       /* Up linkage */
    struct PsdConfig   *nch_Config;       /* Up linkage */
    struct PsdInterface *nch_Interface;   /* Up linkage */
    struct PsdPipe     *nch_EP0Pipe;      /* Endpoint 0 pipe */
    struct Task        *nch_ReadySigTask; /* Task to send ready signal to */
    LONG                nch_ReadySignal;  /* Signal to send when ready */
    struct Task        *nch_Task;         /* Subtask */
    struct MsgPort     *nch_TaskMsgPort;  /* Message Port of Subtask */
    IPTR                nch_IfNum;        /* Interface Number */
    struct NepAudioMode *nch_CurrentMode; /* Currently selected mode */

    ULONG               nch_UpdateFlags;  /* Update Flags for Ctrl */

    struct List         nch_AudioModes;   /* List of audio modes */
    struct List         nch_AudioUnits;   /* List of audio units */

    struct Hook         nch_OutReqHook;   /* Hook for output */
    struct Hook         nch_InReqHook;    /* Hook for input */
    struct Hook         nch_InDoneHook;   /* Hook for input */

    UWORD               nch_UnitProdID;   /* ProductID of unit */
    UWORD               nch_UnitVendorID; /* VendorID of unit */
    UWORD               nch_UnitCfgNum;   /* Config of unit */
    UWORD               nch_UnitIfNum;    /* Interface number */
    UWORD               nch_UnitAltIfNum; /* Alternate interface number */
    BOOL                nch_DenyRequests; /* Do not accept further IO requests */

    struct MsgPort     *nch_AHIMsgPort;   /* AHI Device msg Port */
    struct AHIRequest  *nch_AHIReq;       /* AHI IO Request */
};

struct NepAudioSubLibBase
{
    struct Library      nas_Library;      /* standard */
    UWORD               nas_Flags;        /* various flags */
    struct NepAudioBase *nas_ClsBase;     /* Up linkage */
    struct Library     *nas_UtilityBase;  /* utility base */
    BPTR                nas_SegList;      /* device seglist */
};

#endif /* USBAUDIO_H */
