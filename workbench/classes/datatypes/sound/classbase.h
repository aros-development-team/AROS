#ifndef DATATYPES_SOUNDCLASS_H
#include <datatypes/soundclass.h>
#endif

/****************************************************************************/

struct ClassBase
{
	/* std library stuff */
	struct Library			LibNode;
 	UWORD				pad_word;
 	BPTR				LibSegment;
 	struct SignalSemaphore	cb_LibLock;
 	/* library bases */
#if !defined(__MAXON__) && !defined(__AROS__)
	struct Library			*cb_IntuitionBase;
	struct Library			*cb_GfxBase;
	struct Library			*cb_SysBase;
	struct Library			*cb_DOSBase;
	struct Library			*cb_UtilityBase;
	struct Library			*cb_DataTypesBase;
	struct Library			*cb_IFFParseBase;
	struct Library			*cb_TapeDeckBase;
#endif
	Class	      			*cb_Class;
	ULONG				*cb_Methods;
	/* prefs */
	BOOL				cb_AIFF;
	BOOL				cb_AHI;
	ULONG				cb_AHIModeID;
	ULONG				cb_AHIMixFrequency;
	UWORD				cb_NomWidth;
	UWORD				cb_NomHeight;
	ULONG				cb_WfCol[3];
	ULONG				cb_BgCol[3];
	BOOL				cb_Compress;
	BOOL				cb_ForceAHIMode;
	ULONG				cb_BufferSize;
	UWORD				cb_Volume;
	BOOL				cb_ControlPanel;
	BOOL				cb_NoGTSlider;
};

/****************************************************************************/

struct InstanceData
{
	struct ClassBase	*ClassBase;
	struct SignalSemaphore	 Lock; 
	/* v39 tags */
	struct VoiceHeader  VoiceHeader;
	BYTE			*Sample;
	ULONG			 SampleLength;
	UWORD			 Frequency;
	UWORD			 Volume;
	UWORD			 Cycles;
	/* v40 tags */
	UWORD			 Continuous;
	struct Task		*SignalTask;
	BYTE			 SignalBit;
	/* v41 tags */
	UBYTE			 SampleType;
	ULONG			 Panning;
	/* v44 tags */
	struct timeval	 ReplayPeriod;
	BOOL			 LeftSample; // SDTA_Sample is SDTA_LeftSample?
	UWORD			 pad_word2;
	BYTE			*RightSample;
	BOOL			 SyncSampleChange;
	BOOL			 FreeSampleData;
	/* additional tags */
	BOOL			Immediate;
	BOOL			Repeat;
	BOOL			DelayedImmed;
	/* offset */
	ULONG			StartSample;
	ULONG			EndSample;
	/* private data */
	struct Process	*PlayerProc;
	struct MsgPort	*PlayerPort;
	/* for STM_STOP (doubleclick) */
	struct timeval		LastClick;
	/* DTM_SELECT */
	WORD			MinX;
	WORD			MaxX;
	BOOL			MarkMode;
	/* controlpanel */
	WORD			pad_word;
	WORD			TapeDeckHeight;
	BOOL			ControlPanel;
	struct Gadget		*TapeDeckGadget;
	struct Gadget		*VolumeSlider;
	struct Gadget		*ActiveMember;

	struct Screen		*Screen;	// DTM_DRAW
	struct DrawInfo	*DrawInfo;	// DTM_DRAW
	struct Window	*Window;
	struct Requester	*Requester;
	struct Gadget		*Gadget;

	struct ColorMap	*ColorMap;		// Needed to release allocated pens, GInfo of DTM_REMOVEDTOBJECT == NULL	
	WORD			WaveformPen;	// Drawing pens
	WORD			BackgroundPen;

	BOOL			ForceRefresh;	// bugfix (?) for gmv
};

/****************************************************************************/

struct ObjectMsg {
	struct Message	Message;
	ULONG			Command;
	APTR			Data;
//	ULONG			ErrorCode;
};

enum {
	COMMAND_INIT,
	COMMAND_EXIT,
	COMMAND_PLAY,
	COMMAND_STOP,
	COMMAND_PAUSE,
	COMMAND_PERVOL,
	COMMAND_NEXT_BUFFER
};

/****************************************************************************/
