
struct aura {
	UBYTE			 a_Flags;
	UBYTE			 a_Status;
	UBYTE			 a_GotTimerA;
	UBYTE			 a_GotTimerB;
	ULONG			 a_CIAperiod;
	APTR			 a_OldLevel6Int;
	struct Interrupt	*a_AuraInt;
	struct Interrupt	*a_SoftInt;
	struct AHIRecordMessage *a_RecMessage;
	WORD			*a_MixBuffer1;		/* Filled mixing routine */
	WORD			*a_MixBuffer2;		/* Filled mixing routine */
	WORD			*a_RecBuffer1;
	WORD			*a_RecBuffer2;
/* Don't change order of these! */
	WORD			*a_MixBufferPtr;
	ULONG			 a_MixSamplesCnt;	/* Counter */
	WORD			*a_RecBufferPtr;
	ULONG			 a_RecSamplesCnt;	/* Counter */
};

/* Bits for a_Status */

#define STATUS_PLAY	1
#define STATUS_RECORD	2
#define STATUS_STEREO	4
