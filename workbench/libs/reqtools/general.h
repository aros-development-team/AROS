/* general.h */

/* For req.c */

#define CHECK_PASSWORD			0
#define ENTER_PASSWORD			1
#define ENTER_STRING			2
#define ENTER_NUMBER			3
#define IS_EZREQUEST			4

#ifdef _AROS

/* AROS: FIXME Hmm ... */
#define MININT 0x80000000
#define MAXINT 0x7FFFFFFF

#undef 	REGARGS
#define REGARGS

#undef 	STDARGS
#define STDARGS

#undef  ALIGNED
#define ALIGNED

#undef	CHIP
#define CHIP

#undef 	register
#define register

#undef 	__a0
#define __a0

#undef 	__a1
#define __a1

#undef 	__a2
#define __a2

#undef 	__a3
#define __a3

#undef 	__a4
#define __a4

#undef 	__a5
#define __a5

#undef 	__d0
#define __d0

#undef 	__d1
#define __d1

#undef 	__d2
#define __d2

#undef 	__d3
#define __d3

#undef 	__d4
#define __d4

#undef 	__d5
#define __d5

#undef 	__d6
#define __d6

#undef	__d7
#define	__d7

#undef 	ASM
#define ASM

#undef	SAVEDS
#define SAVEDS

#endif

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
void SAVEDS ASM WinBackFill (register __a0 struct Hook *,
	register __a2 struct RastPort *, register __a1 struct BackFillMsg * );
struct Window *REGARGS OpenWindowBF (struct NewWindow *,
				struct Hook *, UWORD *, ULONG *, WORD *, BOOL);
int CheckReqPos (int, int, struct NewWindow *);
int REGARGS StrWidth_noloc (struct IntuiText *, UBYTE *);
int CheckBoxWidth (struct NewGadget *);
int CheckBoxHeight (struct NewGadget *);
LONG BottomBorderHeight (struct Screen *);

#define REQPOS_DEFAULT		(ULONG)0xFFFF
