
#include <devices/ahi.h>

extern __asm BOOL mt_init( register __a2 struct PTData * );
extern __asm BOOL mt_start( register __a2 struct PTData * );
extern __asm void mt_stop( register __a2 struct PTData * );
extern __asm void mt_end( register __a2 struct PTData * );
extern __asm void mt_music( register __a2 struct PTData * );

#define	n_note		0
#define	n_cmd		2
#define	n_cmdlo		3
#define	n_start		4
#define	n_length	8
#define	n_loopstart	10
#define	n_replen	14
#define	n_period	16
#define	n_finetune	18
#define	n_volume	19
#define	n_dmabit	20
#define	n_toneportdirec	22
#define	n_toneportspeed	23
#define	n_wantedperiod	24
#define	n_vibratocmd	26
#define	n_vibratopos	27
#define	n_tremolocmd	28
#define	n_tremolopos	29
#define	n_wavecontrol	30
#define	n_glissfunk	31
#define	n_sampleoffset	32
#define	n_pattpos	33
#define	n_loopcount	34
#define	n_funkoffset	35
#define	n_wavestart	36
#define	n_reallength	40
#define	n_SIZEOF	42

struct PaulaEmul
{
	BOOL	pe_NewSample;			/* Flag				 */
	ULONG	pe_Offset;			/* Offset from sample start	 */
	UWORD	pe_Length;			/* Length in *WORDS* (like Paula)*/
	BOOL	pe_NewLoopSample;		/* Flag				 */
	ULONG	pe_LoopOffset;			/* Offset from sample start	 */
	UWORD	pe_LoopLength;			/* Length in *WORDS* (like Paula)*/
	BOOL	pe_NewPeriod;			/* Flag				 */
	UWORD	pe_Period;			/* Paula period			 */
	BOOL	pe_NewVolume;			/* Flag				 */
	UWORD	pe_Volume;			/* Paula volume, 0-64		 */
};

struct PTData
{
	struct AHIBase		*ptd_AHIBase;		/* Must be initialized!	 */
	struct AHIAudioCtrl	*ptd_AudioCtrl;		/* Must be initialized!	 */
	APTR			 ptd_ModuleAddress;	/* Must be initialized!	 */

	struct PaulaEmul	 ptd_Ch2;		/* Ch2 (left)		 */
	struct PaulaEmul	 ptd_Ch1;		/* Ch1 (right)		 */
	struct PaulaEmul	 ptd_Ch3;		/* Ch3 (left)		 */
	struct PaulaEmul	 ptd_Ch4;		/* Ch4 (right)		 */

	ULONG			 ptd_SampleStarts[31];
	UBYTE			 ptd_chan1temp[n_SIZEOF];
	UBYTE			 ptd_chan2temp[n_SIZEOF];
	UBYTE			 ptd_chan3temp[n_SIZEOF];
	UBYTE			 ptd_chan4temp[n_SIZEOF];

	UWORD			ptd_NewTempo;
	UWORD			ptd_Tempo;

	UBYTE			ptd_speed;
	UBYTE			ptd_counter;
	UBYTE			ptd_SongPos;
	UBYTE			ptd_PBreakPos;
	UBYTE			ptd_PosJumpFlag;
	UBYTE			ptd_PBreakFlag;
	UBYTE			ptd_LowMask;
	UBYTE			ptd_PattDelTime;
	UBYTE			ptd_PattDelTime2;
	UBYTE			ptd_Enable;
	UWORD			ptd_PatternPos;
	UWORD			ptd_DMACONtemp;
};
