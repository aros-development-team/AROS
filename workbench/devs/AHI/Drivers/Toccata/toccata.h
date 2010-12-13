
/* Flags */

#define TF_ISPLAYING	(1<<0)
#define TF_ISRECORDING	(1<<1)
#define TF_IAMTHEOWNER	(1<<2)

struct toccata {
	UBYTE			 t_Flags;
	UBYTE			 t_Input;
	UWORD			 t_DisableCount;
	struct Task		*t_MasterTask;
	struct Library		*t_AHIsubBase;
	struct Interrupt	*t_PlaySoftInt;
	struct Interrupt	*t_MixSoftInt;
	struct Process		*t_SlaveProcess;
	WORD			*t_MixBuffer1;		/* Filled mixing routine */
	WORD			*t_MixBuffer2;		/* Filled mixing routine */
	WORD			*t_MixBuffer3;		/* Filled mixing routine */
	WORD			*t_SampBuffer1;		/* Played by Toccata */
	WORD			*t_SampBuffer2;		/* Played by Toccata */
	ULONG			*t_RecBuffer;		/* Filled by Toccata */
	struct AHIRecordMessage *t_RecMessage;
	ULONG			 t_TocSamples;		/* Size of Toc's playbuffer */
	ULONG			 t_TocSamplesCnt;	/* Counter */
	ULONG			 t_MixSamplesCnt;	/* Counter for mixbuffer */
	WORD			*t_MixBufferPtr;
	Fixed			 t_Loopback;
	ULONG			 t_Mode;
	BOOL			 t_NoTask;
	BYTE			 t_MasterSignal;
	BYTE			 t_SlaveSignal;
	BYTE			 t_PlaySignal;
	BYTE			 t_RecordSignal;
	BYTE			 t_MixSignal;
};
