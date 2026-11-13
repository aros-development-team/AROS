#ifndef MIDI_CAMD_H
#define MIDI_CAMD_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/* Some comments are needed in this file. */
#ifndef EXEC_TYPES_H
#  include <exec/types.h>
#endif
#ifndef EXEC_LISTS_H
#  include <exec/lists.h>
#endif
#ifndef UTILITY_TAGITEM_H
#  include <utility/tagitem.h>
#endif

#define CD_Linkages 0

typedef union{
	ULONG l[2];
	UBYTE b[4];
}MidiMsg;

#define mm_Msg l[0]
#define mm_Time l[1]
#if !AROS_BIG_ENDIAN
#  define mm_Status b[3]
#  define mm_Data1 b[2]
#  define mm_Data2 b[1]
#  define mm_Port b[0]
#else
#  define mm_Status b[0]
#  define mm_Data1 b[1]
#  define mm_Data2 b[2]
#  define mm_Port b[3]
#endif
#define mm_Data b

struct MidiCluster{
	struct Node mcl_Node;
	WORD mcl_Participants;
	struct List mcl_Receivers;
	struct List mcl_Senders;
	WORD mcl_PublicParticipants;
	UWORD mcl_Flags;

	/* Private data */
};

/* Please use GetMidiLinkAttrsA instead of reading directly, and no
   writing is allowed.
*/
struct MidiLink{
	struct Node ml_Node;
	WORD ml_Pad;

	struct MinNode ml_OwnerNode;
	struct MidiNode *ml_MidiNode;
	struct MidiCluster *ml_Location;
	char *ml_ClusterComment;

	UBYTE ml_Flags;
	UBYTE ml_PortID;
	UWORD ml_ChannelMask;
	ULONG ml_EventTypeMask;
	union SysExFilter{
		UBYTE b[4];
		ULONG sxf_Packed;
	}ml_SysExFilter;

	APTR ml_ParserData;
	APTR ml_UserData;

	/* Private data may follow. (currently not, but..) */
};
#if !AROS_BIG_ENDIAN
#  define sxf_Mode b[3]
#  define sxf_ID1 b[2]
#  define sxf_ID2 b[1]
#  define sxf_ID3 b[0]
#else
#  define sxf_Mode b[0]
#  define sxf_ID1 b[1]
#  define sxf_ID2 b[2]
#  define sxf_ID3 b[3]
#endif

#define MLTYPE_Receiver 0
#define MLTYPE_Sender 1
#define MLTYPE_NTypes 2

#define MLF_Sender 1
#define MLF_PartChange 2
#define MLF_PrivateLink 4
#define MLF_DeviceLink 8

#define MLINK_Base (TAG_USER+65)
enum{
	MLINK_Location=MLINK_Base,
	MLINK_ChannelMask,
	MLINK_EventMask,
	MLINK_UserData,
	MLINK_Comment,
	MLINK_PortID,
	MLINK_Private,
	MLINK_Priority,
	MLINK_SysExFilter,
	MLINK_SysExFilterX,
	MLINK_Parse,
	MLINK_Reserved,
	MLINK_ErrorCode,
	MLINK_Name
};

#define SXF_ModeBits 4
#define SXF_CountBits 3
#define SXFM_Off 0
#define SXFM_1Byte 0
#define SXFM_3Byte 4

/* Please use GetMidiAttrsA instead of reading directly, and no
   writing is allowed.
*/
struct MidiNode{
	struct Node mi_Node;

	UWORD mi_ClientType;

	struct Image *mi_Image;

	struct MinList mi_OutLinks;
	struct MinList mi_InLinks;

	struct Task *mi_SigTask;
	struct Hook *mi_ReceiveHook;
	struct Hook *mi_ParticipantHook;
	BYTE mi_ReceiveSigBit;
	BYTE mi_ParticipantSigBit;

	UBYTE mi_ErrFilter;

	UBYTE mi_pad;

	ULONG *mi_TimeStamp;

	ULONG mi_MsgQueueSize;
	ULONG mi_SysExQueueSize;

	/* Private data. */
};


#define CCType_Sequencer 1
#define CCType_SampleEditor 2
#define CCType_PatchEditor 4
#define CCType_Notator 8
#define CCType_EventProcessor 16
#define CCType_EventFilter 32
#define CCType_EventRouter 64
#define CCType_ToneGenerator 128
#define CCType_EventGenerator 256
#define CCType_GraphicAnimator 512

#define MIDI_Base (TAG_USER+65)
enum{
	MIDI_Name=MIDI_Base,
	MIDI_SignalTask,
	MIDI_RecvHook,
	MIDI_PartHook,
	MIDI_RecvSignal,
	MIDI_PartSignal,
	MIDI_MsgQueue,
	MIDI_SysExSize,
	MIDI_TimeStamp,
	MIDI_ErrFilter,
	MIDI_ClientType,
	MIDI_Image,
	MIDI_ErrorCode
};

enum{
	CME_NoMem=801,
	CME_NoSignals,
	CME_NoTimer
};
#define CME_NoUnit(a) (820+(a))

enum{
	CMB_Note,
	CMB_Prog,
	CMB_PitchBend,
	CMB_CtrlMSB,
	CMB_CtrlLSB,
	CMB_CtrlSwitch,
	CMB_CtrlByte,
	CMB_CtrlParam,
	CMB_CtrlUndef,
	CMB_Mode,
	CMB_ChanPress,
	CMB_PolyPress,
	CMB_RealTime,
	CMB_SysCom,
	CMB_SysEx
};

#define CMF_Note (1L<<CMB_Note)
#define CMF_Prog (1L<<CMB_Prog)
#define CMF_PitchBend (1L<<CMB_PitchBend)
#define CMF_CtrlMSB (1L<<CMB_CtrlMSB)
#define CMF_CtrlLSB (1L<<CMB_CtrlLSB)
#define CMF_CtrlSwitch (1L<<CMB_CtrlSwitch)
#define CMF_CtrlByte (1L<<CMB_CtrlByte)
#define CMF_CtrlParam (1L<<CMB_CtrlParam)
#define CMF_CtrlUndef (1L<<CMB_CtrlUndef)
#define CMF_Mode (1L<<CMB_Mode)
#define CMF_ChanPress (1L<<CMB_ChanPress)
#define CMF_PolyPress (1L<<CMB_PolyPress)
#define CMF_RealTime (1L<<CMB_RealTime)
#define CMF_SysCom (1L<<CMB_SysCom)
#define CMF_SysEx (1L<<CMB_SysEx)

#define CMF_Ctrl (CMF_CtrlMSB|CMF_CtrlLSB|CMF_CtrlSwitch|CMF_CtrlByte|CMF_CtrlParam|CMF_CtrlUndef)
#define CMF_Channel (CMF_Note|CMF_Prog|CMF_PitchBend|CMF_Ctrl|CMF_Mode|CMF_ChanPress|CMF_PolyPress)
#define CMD_All (CMF_Ctrl|CMF_Channel|CMF_SysCom|CMF_SysEx)

#define PutMidiMsg(midilink,msg) PutMidi((midilink),(msg)->mm_Msg)

enum{
	CMEB_MsgErr,
	CMEB_BufferFull,
	CMEB_SysExFull,
	CMEB_ParseMem,
	CMEB_RecvErr,
	CMEB_RecvOverflow,
	CMEB_SysExTooBig
};

#define CMEF_MsgErr (1L<<CMEB_MsgErr)
#define CMEF_BufferFull (1L<<CMEB_BufferFull)
#define CMEF_SysExFull (1L<<CMEB_SysExFull)
#define CMEF_ParseMem (1L<<CMEB_ParseMem)
#define CMEF_RecvErr (1L<<CMEB_RecvErr)
#define CMEF_RecvOverflow (1L<<CMEB_RecvOverflow)
#define CMEF_SysExTooBig (1L<<CMEB_SysExTooBig)

#define CMEF_All (CMEF_MsgErr|CMEF_BufferFull|CMEF_SysExFull|CMEF_ParseMem|CMEF_RecvErr|CMEF_RecvOverflow|CMEF_SysExTooBig)

struct ClusterNotifyNode{
  struct MinNode cnn_Node;
  struct Task *cnn_Task;
  BYTE cnn_SigBit;
};


#endif

