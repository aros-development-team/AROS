/*
 */

#ifdef __AROS__
#include <aros/asmcall.h>
#endif

#define NOSCRTOFRONT_GADID	1
#define IMMSORT_GADID		2
#define DIRSFIRST_GADID		3
#define DIRSMIXED_GADID		4
#define NOLED_GADID		5
#define DEFAULTFONT_GADID	6
#define REQTYPE_GADID		7
#define DEFSIZE_GADID		8
#define SETSIZE_GADID		9
#define MINENTRIES_GADID	10
#define MAXENTRIES_GADID	11
#define TEST_GADID		12
#define POSITION_GADID		13
#define OFFSETX_GADID		14
#define OFFSETY_GADID		15
#define SAVE_GADID		16
#define USE_GADID		17
#define CANCEL_GADID		18

#define FKEYS_GADID		19
#define DOWHEEL_GADID		20
#define MMB_GADID		21

#define QUIT_MENUID		1
#define OPEN_MENUID		2
#define SAVEAS_MENUID		3
#define ABOUT_MENUID		4
#define RESET_MENUID		5
#define LAST_MENUID		6
#define RESTORE_MENUID		7

#define EDIT_MENU		1

#define OPTIONS_MENU		2
#define SAVEICONS_ITEM		0

extern struct ReqToolsPrefs	RTPrefs;
extern struct Screen 		*Screen;
extern struct Window 		*WindowPtr;
extern struct DrawInfo		*DrawInfo;
extern struct Gadget *glist, *xoffgad, *yoffgad, *mixdirsgad, *dirsfirstgad, *mmbgad;
extern struct Gadget *mingad, *maxgad;
extern struct Menu *Menus;
extern APTR	VisualInfo;
extern UWORD Zoom[];
extern WORD	CurrentReq, WheelType;
extern BOOL	UseScreenFont;

extern void RenderPrefsWindow (void);
extern long OpenPrefsWindow (void);
extern void UpdatePrefsWindow (int);
extern void ClosePrefsWindow (void);

extern BOOL	OpenGUI( VOID );
extern VOID	CloseGUI( VOID );
extern VOID	LoopGUI( VOID );

extern VOID	GadgetOff( struct Gadget * );
extern VOID	GadgetOn( struct Gadget * );
extern VOID	SetCheckState( struct Gadget *, BOOL );
extern VOID	SetIntGad( struct Gadget *, LONG );
extern LONG	IntGadValue( struct Gadget * );

extern BOOL	ProcessGadget( UWORD, UWORD );
extern BOOL	ProcessMenuItem( UWORD );

#ifdef __AROS__
AROS_UFP3(void, IntuiMsgFunc,
    AROS_UFPA(struct Hook *, hook, A0),
    AROS_UFPA(APTR, req, A2),
    AROS_UFPA(struct IntuiMessage *, imsg, A1));
#else
extern void __asm __saveds IntuiMsgFunc( register __a0 struct Hook *, register __a2 APTR, register __a1 struct IntuiMessage * );
#endif
