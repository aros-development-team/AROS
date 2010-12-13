
	IFND EXEC_TYPES_I
	INCLUDE 'exec/types.i'
	ENDC

n_note		EQU	0  ; W
n_cmd		EQU	2  ; W
n_cmdlo		EQU	3  ; B
n_start		EQU	4  ; L
n_length	EQU	8  ; W
n_loopstart	EQU	10 ; L
n_replen	EQU	14 ; W
n_period	EQU	16 ; W
n_finetune	EQU	18 ; B
n_volume	EQU	19 ; B
n_dmabit	EQU	20 ; W
n_toneportdirec	EQU	22 ; B
n_toneportspeed	EQU	23 ; B
n_wantedperiod	EQU	24 ; W
n_vibratocmd	EQU	26 ; B
n_vibratopos	EQU	27 ; B
n_tremolocmd	EQU	28 ; B
n_tremolopos	EQU	29 ; B
n_wavecontrol	EQU	30 ; B
n_glissfunk	EQU	31 ; B
n_sampleoffset	EQU	32 ; B
n_pattpos	EQU	33 ; B
n_loopcount	EQU	34 ; B
n_funkoffset	EQU	35 ; B
n_wavestart	EQU	36 ; L
n_reallength	EQU	40 ; W
n_SIZEOF	EQU	42

	STRUCTURE PaulaEmul,0
	BOOL	pe_NewSample			;Flag
	ULONG	pe_Offset			;Offset from sample start
	UWORD	pe_Length			;Length in *WORDS* (like Paula)
	BOOL	pe_NewLoopSample		;Flag
	ULONG	pe_LoopOffset			;Offset from sample start
	UWORD	pe_LoopLength			;Length in *WORDS* (like Paula)
	BOOL	pe_NewPeriod			;Flag
	UWORD	pe_Period			;Paula period
	BOOL	pe_NewVolume			;Flag
	UWORD	pe_Volume			;Paula volume, 0-64
	LABEL	PaulaEmul_SIZEOF


	STRUCTURE PTData,0
	APTR	ptd_AHIBase			;Must be initialized!
	APTR	ptd_AudioCtrl			;Must be initialized!
	APTR	ptd_ModuleAddress		;Must be initialized!

	LABEL	ptd_Chs
	STRUCT	ptd_Ch2,PaulaEmul_SIZEOF	;Ch2 (left)
	STRUCT	ptd_Ch1,PaulaEmul_SIZEOF	;Ch1 (right)
	STRUCT	ptd_Ch3,PaulaEmul_SIZEOF	;Ch3 (left)
	STRUCT	ptd_Ch4,PaulaEmul_SIZEOF	;Ch4 (right)
	STRUCT	ptd_SampleStarts,4*31
	STRUCT	ptd_chan1temp,n_SIZEOF
	STRUCT	ptd_chan2temp,n_SIZEOF
	STRUCT	ptd_chan3temp,n_SIZEOF
	STRUCT	ptd_chan4temp,n_SIZEOF

	UWORD	ptd_NewTempo
	UWORD	ptd_Tempo

	UBYTE	ptd_speed
	UBYTE	ptd_counter
	UBYTE	ptd_SongPos
	UBYTE	ptd_PBreakPos
	UBYTE	ptd_PosJumpFlag
	UBYTE	ptd_PBreakFlag
	UBYTE	ptd_LowMask
	UBYTE	ptd_PattDelTime
	UBYTE	ptd_PattDelTime2
	UBYTE	ptd_Enable
	UWORD	ptd_PatternPos
	UWORD	ptd_DMACONtemp
	LABEL	PTData_SIZEOF
