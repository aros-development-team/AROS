
	include	exec/types.i

	STRUCTURE toccata,0
	UBYTE	t_Flags
	UBYTE	t_Input
	UWORD	t_DisableCount
	APTR	t_MasterTask
	APTR	t_AHIsubBase
	APTR	t_PlaySoftInt
	APTR	t_MixSoftInt
	APTR	t_SlaveProcess
	APTR	t_MixBuffer1
	APTR	t_MixBuffer2
	APTR	t_MixBuffer3
	APTR	t_SampBuffer1
	APTR	t_SampBuffer2
	APTR	t_RecBuffer
	APTR	t_RecMessage
	ULONG	t_TocSamples		* Size of Toc's playbuffer
	ULONG	t_TocSamplesCnt		* Counter
	ULONG	t_MixSamplesCnt		* Counter for mixbuffer
	APTR	t_MixBufferPtr
	Fixed	t_Loopback
	ULONG	t_Mode
	BOOL	t_NoTask
	BYTE	t_MasterSignal
	BYTE	t_SlaveSignal
	BYTE	t_PlaySignal
	BYTE	t_RecordSignal
	BYTE	t_MixSignal
	LABEL	toccata_SIZEOF

