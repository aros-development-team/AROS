/* Descriptor field types */
#define DT_INVALID			0L
#define DT_PAGE_DESCRIPTOR	1L
#define DT_VALID4BYTE		2L
#define DT_VALID8BYTE		3L

/* TC */
#define TC_E			31
#define TC_SRE			25
#define TC_FCL 		24
#define TC_PS_END 	23
#define TC_PS_START  20
#define TC_IS_END		19
#define TC_IS_START	16
#define TC_TIA_END	15
#define TC_TIA_START	12
#define TC_TIB_END	11
#define TC_TIB_START	 8
#define TC_TIC_END	 7
#define TC_TIC_START	 4
#define TC_TID_END	 3
#define TC_TID_START	 0

/* CRP / SRP */
#define RP_LU				63
#define RP_LIMIT_END		62
#define RP_LIMIT_START	48
#define RP_DT_END			33
#define RP_DT_START		32
#define RP_TA_END			31
#define RP_TA_START		 4

/* TT0 / TT1 */
#define TT_LAB_END		31
#define TT_LAB_START		24
#define TT_LAM_END		23
#define TT_LAM_START		16
#define TT_E				15
#define TT_CI				10
#define TT_RW				 9
#define TT_RWM				 8
#define TT_FCB_END		 6
#define TT_FCB_START		 4
#define TT_FCM_END		 2
#define TT_FCM_START		 0

/* MMUSR */
#define SR_B			15
#define SR_L			14
#define SR_S			13
#define SR_W			11
#define SR_I			10
#define SR_M			 9
#define SR_T			 6
#define SR_N_END		 2
#define SR_N_START	 0

/* Long table descriptor */
#define LT_LU				63
#define LT_LIMIT_END    62
#define LT_LIMIT_START	48
#define LT_S				40
#define LT_U				35
#define LT_WP				34
#define LT_DT_END			33
#define LT_DT_START		32
#define LT_TA_END			31
#define LT_TA_START		 4

/* Short table descriptor */
#define ST_TA_END		31
#define ST_TA_START	 4
#define ST_U			 3
#define ST_WP		 	 2
#define ST_DT_END		 1
#define ST_DT_START	 0

/* Page descriptor */
#define PA_PA_END			31
#define PA_PA_START		8
#define PA_CI				6
#define PA_M				4
#define PA_U				3
#define PA_WP				2
#define PA_DT_END			1
#define PA_DT_START		0

/* Illegal descriptor */
#define IL_DT_END			1
#define IL_DT_START		0

/* Definitionen für neue MMU-Tabelle */
#define SEGMENT( adr, segmentSize ) ( (adr) & ~((segmentSize)-1) )
#define MAKE_MASK( from, to, val ) ( (ULONG)( (val) << (from) ) )
#define VAL32(val, from, to) (((val) >> (from)) & ((1L<<((to)-(from)+1))-1))
#define VAL64(val, from, to) (((from) >= 32) ? \
               VAL32 (val.hi,(from)&0x1f,(to)&0x1f) :\
               VAL32 (val.lo,(from)&0x1f,(to)&0x1f))

typedef enum { ML_A, ML_B, ML_C, ML_D } MMULevels;

#define TABLE_ADDRESS_SHIFT 4
#define TABLE_ADDRESS_ALIGNMENT (1L<<TABLE_ADDRESS_SHIFT)

#define PAGE_ADDRESS_SHIFT 8
#define PAGE_ADDRESS_ALIGNMENT (1L<<PAGE_ADDRESS_SHIFT)

#define LEVEL_A_BITS	7
#define LEVEL_B_BITS	7

#if PAGESIZE==4096
#define LEVEL_C_BITS	6
#define PAGE_BITS		12
#endif

#if PAGESIZE==8192
#define LEVEL_C_BITS	5
#define PAGE_BITS		13
#endif

/* Bit-Operationen */
#define SET(word,bit) ((word) |=  (1 << (bit)))
#define CLR(word,bit) ((word) &= ~(1 << (bit)))
#define TST(word,bit) ( ((word) >> (bit)) & 1 )

typedef ULONG Reg32;
typedef struct {	ULONG hi; ULONG lo; } Reg64;
