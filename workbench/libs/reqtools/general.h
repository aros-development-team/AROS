#ifndef GENERAL_H
#define GENERAL_H

/* general.h */

/* For req.c */

#define CHECK_PASSWORD			0
#define ENTER_PASSWORD			1
#define ENTER_STRING			2
#define ENTER_NUMBER			3
#define IS_EZREQUEST			4

/* These are for AmigaOS. On AROS they are ignored */

#define USE_ASM_FUNCS 	    	    	1
#define USE_OPTASM_FUNCS    	    	1

/* compiler/AmigaOS/AROS specific defines */

#ifndef _AROS

/* AmigaOS */

#include "compilerspecific.h"

#else

#include <aros/asmcall.h>

/* AROS */

#undef USE_ASM_FUNCS
#define USE_ASM_FUNCS 	    	    	0

#undef USE_OPTASM_FUNCS
#define USE_OPTASM_FUNCS    	    	0

/* AROS: FIXME Hmm ... */
#define MININT      	    	    	0x80000000
#define MAXINT      	    	    	0x7FFFFFFF

#undef 	REGARGS
#define REGARGS

#undef 	STDARGS
#define STDARGS

#undef  ALIGNED
#define ALIGNED

#undef	CHIP
#define CHIP

#undef 	ASM
#define ASM

#undef	SAVEDS
#define SAVEDS

#define REGPARAM(reg,type,name)     	type name
#define OPT_REGPARAM(reg,type,name) 	type name
#define ASM_REGPARAM(reg,type,name) 	type name

#endif

struct PWCallBackArgs;
typedef STDARGS char * (*PWCALLBACKFUNPTR) (long, long, struct PWCallBackArgs *);

struct BackFillMsg;
struct NewGadget;

extern void REGARGS InitNewGadget (struct NewGadget *, int, int,
																		int, int, char *, UWORD);
extern int REGARGS GetVpCM (struct ViewPort *, APTR *);
extern void REGARGS RefreshVpCM (struct ViewPort *, APTR);
extern void REGARGS LoadCMap (struct ViewPort *, APTR);
extern void REGARGS FreeVpCM (struct ViewPort *, APTR, BOOL);

extern struct TextFont * REGARGS GetReqFont (struct TextAttr *,
				struct TextFont *, int *, int *, int);
extern struct Screen *REGARGS LockPubScreenByAddr (struct Screen *);
extern struct Screen *REGARGS GetReqScreen (struct NewWindow *,
				struct Window **, struct Screen *, char *);
extern void REGARGS DoWaitPointer (struct Window *, int, int);
extern APTR REGARGS DoLockWindow (struct Window *, int, APTR, int);
extern void REGARGS DoScreenToFront (struct Screen *, int, int);
extern struct IntuiMessage *REGARGS GetWin_GT_Msg (struct Window *,
						  struct Hook *, APTR);
extern struct IntuiMessage *REGARGS ProcessWin_Msg (struct Window *,
				struct IntuiMessage *, struct Hook *, APTR);
extern void REGARGS Reply_GT_Msg (struct IntuiMessage *);
extern void REGARGS DoCloseWindow (struct Window *, int);
extern void REGARGS mySetWriteMask (struct RastPort *, ULONG);

#ifdef _AROS
AROS_UFP3(void, WinBackFill,
    AROS_UFPA(struct Hook *, hook, A0),
    AROS_UFPA(struct RastPort *, the_rp, A2),
    AROS_UFPA(struct BackFillMsg *, msg, A1));
#else
void SAVEDS ASM WinBackFill (REGPARAM(a0, struct Hook *,),
	REGPARAM(a2, struct RastPort *,), REGPARAM(a1, struct BackFillMsg *,));
#endif

struct Window *REGARGS OpenWindowBF (struct NewWindow *,
				struct Hook *, UWORD *, ULONG *, WORD *, BOOL);
int CheckReqPos (int, int, struct NewWindow *);
int REGARGS StrWidth_noloc (struct IntuiText *, UBYTE *);
int CheckBoxWidth (struct NewGadget *);
int CheckBoxHeight (struct NewGadget *);
LONG BottomBorderHeight (struct Screen *);

#define REQPOS_DEFAULT		(ULONG)0xFFFF

#endif /* GENERAL_H */
