
	include	exec/types.i

	STRUCTURE aura,0
	UBYTE	a_Flags
	UBYTE	a_Status
	UBYTE	a_GotTimerA
	UBYTE	a_GotTimerB
	ULONG	a_CIAperiod
	APTR	a_OldLevel6Int
	APTR	a_AuraInt
	APTR	a_SoftInt
	APTR	a_RecMessage

	APTR	a_MixBuffer1		* Filled mixing routine
	APTR	a_MixBuffer2		* Filled mixing routine
	APTR	a_RecBuffer1
	APTR	a_RecBuffer2

	LABEL	a_IntLocalData
* Don't change order of these!
	APTR	a_MixBufferPtr
	ULONG	a_MixSamplesCnt		* Counter
	APTR	a_RecBufferPtr
	ULONG	a_RecSamplesCnt		* Counter
	LABEL	aura_SIZEOF
