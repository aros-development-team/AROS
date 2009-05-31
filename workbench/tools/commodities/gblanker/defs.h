/*
 *	Copyright (c) 1994 Michael D. Bayne.
 *	All rights reserved.
 *
 *	Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

/* Return values */
#define FAILED     -1L
#define QUIT        0L
#define OK          1L
#define UNBLANK     2L
#define CLOSEWIN    3L
#define RESTART     4L
#define DELAYEDQUIT 5L

/* Cx message IDs */
#define EVT_CX_POPUP 1L
#define EVT_CX_BLANK 2L

/* Module prefs command IDs */
#define STARTUP 1L
#define IDCMP   2L
#define KILL    3L

/* Blanker message IDs */
#define BM_INITMSG     10
#define BM_DOBLANK     11
#define BM_DOPREFS     12
#define BM_DOQUIT      13
#define BM_UNBLANK     14
#define BM_FAILED      15
#define BM_DELAYEDQUIT 17
#define BM_SENDBLANK   18
#define BM_SENDUNBLANK 19
#define BM_TIMER       20
#define BM_CHECKMOUSE  21
#define BM_PING        22
#define BM_RELOADPREFS 23
#define BM_DOTESTBLANK 24
#define BM_SENDTEST    25

/* Blanker message/prefs flags */
#define BF_LOCKED    ( 1L << 0 )
#define BF_REPLY     ( 1L << 1 )
#define BF_INTERNAL  ( 1L << 2 )
#define BF_REPLACE   ( 1L << 3 )

/* Prefs->bp_BlankCorner defines */
#define BC_NONE       0
#define BC_UPPERLEFT  1
#define BC_UPPERRIGHT 2
#define BC_LOWERRIGHT 3
#define BC_LOWERLEFT  4

/* Signal masks */
#define SIG_SERVPORT   ( 1L << ServerPort->mp_SigBit )
#define SIG_TIMER      ( 1L << TimerPort->mp_SigBit )
#define SIG_PORT       ( 1L << ClientPort->mp_SigBit )

typedef struct _BlankerEntry
{
	struct Node be_Node;
	STRPTR be_Name;
	BYTE be_Path[128];
	BYTE be_PrefFile[128];
	LONG be_Disabled;
} BlankerEntry;

typedef struct _BlankerPrefs
{
	LONG bp_Priority;
	LONG bp_PopUp;
	LONG bp_Timeout;
	LONG bp_RandTimeout;
	LONG bp_BlankCorner;
	LONG bp_DontCorner;
	BYTE bp_PopKey[128];
	BYTE bp_BlankKey[128];
	BYTE bp_Blanker[64];
	BYTE bp_Dir[128];
	LONG bp_Flags;
} BlankerPrefs;

typedef struct _BlankMsg
{
	struct Message bm_Mess;
	LONG bm_Type;
	LONG bm_Flags;
} BlankMsg;
