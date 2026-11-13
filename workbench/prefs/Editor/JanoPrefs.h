/*** Definition of some common ressources ***/

#include "Utils.h"

#define CBS			4						/* Number of checkbox gadget */
#define CGS			5						/* Number of cycle gadget */
#define UCS			(CGS+CBS)			/* Start Id of Use/Cancel/Save */
#define MiscTxt	(PrefMsg+1)			/* Some shortcut definition */
#define ChkTxt		(MiscTxt+CGS+1)
#define OkCanSav	(ChkTxt+CBS+1)
#define FTCycTxt	(OkCanSav+4)
#define ScrCycTxt	(FTCycTxt+4)
#define ColCycTxt (ScrCycTxt+5)

/*** Shared message strings ***/
extern struct NewMenu newmenu[];
extern STRPTR Errors[], PrefMsg[], FSCycTxt[];

BYTE setup_guipref(void);
void close_prefwnd( BYTE cmd);
