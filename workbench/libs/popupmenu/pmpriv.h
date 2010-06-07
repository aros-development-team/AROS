//
// Popup Menu
// ©1996-2002 Henrik Isaksson
// Private definitons
//

#ifndef PMPRIV_H
#define PMPRIV_H

#include "compiler.h"

#ifndef __AROS__
#include "screens.h"
#include "gui.h"
#endif

#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/commodities.h>
#include <proto/layers.h>
#include <proto/keymap.h>
#include <proto/cybergraphics.h>

#include <exec/libraries.h>
#include <intuition/intuition.h>
#include <intuition/imageclass.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <exec/libraries.h>
#include <exec/interrupts.h>
#include <devices/inputevent.h>
#include <devices/input.h>
#include <devices/timer.h>

#include <dos/dos.h>

#include <clib/alib_protos.h>

#include <string.h>

#include <cybergraphx/cybergraphics.h>  /* v41 uses cybergraphx/ dir */

#include "popupmenu.h"

#include "pmtypes.h"
#include "pmmem.h"
#include "pmprefs.h"
#include "prefs.h"
#include "pmshadow.h"
#include "pmtopography.h"
#include "pminput.h"
#include "pmfind.h"
#include "pminternals.h"
#include "pmgraph.h"
#include "pmimages.h"

#ifndef __AROS__

/*--- functions in V50 or higher (Release 4) ---*/
#pragma libcall IntuitionBase ShowWindow 342 9802
#pragma libcall IntuitionBase HideWindow 348 801
#pragma libcall IntuitionBase GetAttrsA 34e 9802
#pragma libcall IntuitionBase LockGUIPrefs 354 801
#pragma libcall IntuitionBase UnlockGUIPrefs 35a 801
#pragma libcall IntuitionBase SetGUIAttrsA 360 A9803
#pragma libcall IntuitionBase GetGUIAttrsA 366 A9803
#pragma tagcall IntuitionBase GetGUIAttrs 366 A9803
#pragma libcall IntuitionBase GetHalfPens 36c A90804
#pragma libcall IntuitionBase GadgetBox 372 A109805
#pragma libcall IntuitionBase RefreshSetGadgetAttrsA 378 BA9804
#pragma libcall IntuitionBase IDoSuperMethodA 37e 9A803
#pragma libcall IntuitionBase ISetSuperAttrsA 384 9A803
#pragma libcall IntuitionBase ICoerceMethodA 38a 9A803
#pragma libcall IntuitionBase IDoMethodA 390 9A02
#pragma libcall IntuitionBase OpenClass 396 0802
#pragma libcall IntuitionBase CloseClass 39c 801
#pragma libcall IntuitionBase GetClassBase 3a2 801
#pragma libcall IntuitionBase LockScreen 3a8 801
#pragma libcall IntuitionBase UnlockScreen 3ae 801
#pragma libcall IntuitionBase GetWindowAttrsA 3b4 9802
#pragma libcall IntuitionBase SetWindowAttrsA 3ba 9802
#pragma libcall IntuitionBase GetWindowAttr 3c0 190804
#pragma libcall IntuitionBase SetWindowAttr 3c6 190804
#pragma libcall IntuitionBase StripIntuiMessages 3cc 9802
#pragma libcall IntuitionBase StartScreenNotify 3d2 801
#pragma libcall IntuitionBase EndScreenNotify 3d8 801
#pragma libcall IntuitionBase GetScreenAttrsA 3de 9802
#pragma libcall IntuitionBase SetScreenAttrsA 3e4 9802
#pragma libcall IntuitionBase GetScreenAttr 3ea 190804
#pragma libcall IntuitionBase SetScreenAttr 3f0 190804
#pragma libcall IntuitionBase LockScreenList 3f6 0
#pragma libcall IntuitionBase UnlockScreenList 3fc 0
#pragma libcall IntuitionBase LockScreenGI 402 801
#pragma libcall IntuitionBase UnlockScreenGI 408 9802
#endif

#define PM_Req(c, s)		if(c) EZReq("Assertion failed:\n\n" s, "OK", 0)

#define SelBarLeft(a)       (a->p->BorderWidth+1+PM_Prefs->pmp_XSpace)
#define SelBarRight(a)      (a->p->BorderWidth-1-PM_Prefs->pmp_XSpace)
#define SelBarTop(a, pm)    (pm->Top+PM_Prefs->pmp_YSpace+1)
#define SelBarBottom(a, pm) (pm->Top-PM_Prefs->pmp_YSpace+pm->Height-1)

#define FirstColumn(a)          (PM_Prefs->pmp_XOffset+a->p->BorderWidth+1) /* sista 1 == selbarwidth */

#define BarTop(a, pm)       (pm->Top+pm->Height/2+1)

#define BarLeft(a, pm)      (pm->Left)

#define BarRight(a, pm)     (pm->Left+pm->Width)

#define XAPosWideSelBar(a)      SelBarLeft(a)
#define XBPosWideSelBar(a)      SelBarRight(a)
#define XAPosSelBar(a)          SelBarLeft(a)
#define XBPosSelBar(a)          SelBarRight(a)
#define YAPosSelBar(a, pm)  SelBarTop(a, pm)
#define YBPosSelBar(a, pm)      SelBarBottom(a, pm)
#define CheckmarkSize(a)        (PM_Prefs->pmp_Intermediate+a->p->MenuFont->tf_XSize)
#define SubmarkSize(a)          (PM_Prefs->pmp_Intermediate+a->p->MenuFont->tf_XSize)
#define YPosText(a, pm)         (pm->Top+a->p->MenuFont->tf_Baseline+(pm->Height+1)/2-(a->p->MenuFont->tf_YSize)/2+PM_Prefs->pmp_TextDisplace)
//#define XPosLastCol(a)          (a->Width-(1)-a->p->MenuFont->tf_XSize-PM_Prefs->pmp_Intermediate-PM_Prefs->pmp_XSpace)
//#define XPosLastColImg(a, img)  (a->Width-a->p->BorderWidth-PMImgWidth(img)-PM_Prefs->pmp_Intermediate)
#define XPosLastCol(a)          (pm->Left+pm->Width-a->p->MenuFont->tf_XSize-PM_Prefs->pmp_XSpace)
#define XPosLastColImg(a, img)  (pm->Left+pm->Width-PMImgWidth(img)-PM_Prefs->pmp_XSpace)
#define YPosImage(a, pm, img)   (pm->Top+(pm->Height+1)/2-PMImgHeight(img)/2)
#define YPosImage_(a, pm, img)  (pm->Top+(pm->Height+1)/2-img->Height/2)

#define OutOfRange(x, min, max) (x>=max || x<=min)

#define SCREENMOUSEPOS(p)	p->RootWnd->WScreen->MouseX, p->RootWnd->WScreen->MouseY
#define SCREENMOUSEX(p)		p->RootWnd->WScreen->MouseX
#define SCREENMOUSEY(p)		p->RootWnd->WScreen->MouseY

#ifdef __AROS__
#warning "FIXME: Pens"
#define MENUSHINEPEN 	    SHINEPEN
#define MENUSHADOWPEN 	    SHADOWPEN
#define MENUTEXTPEN 	    TEXTPEN
#define SELECTPEN   	    FILLPEN
#define SELECTTEXTPEN	    FILLTEXTPEN
#define MENUBACKGROUNDPEN   BACKGROUNDPEN
#endif

#define SHINE(xyz)              xyz->DrawInfo->dri_Pens[MENUSHINEPEN]
#define SHADOW(xyz)             xyz->DrawInfo->dri_Pens[MENUSHADOWPEN]
#define TEXT(xyz)               xyz->DrawInfo->dri_Pens[MENUTEXTPEN]
#define FILL(xyz)               xyz->DrawInfo->dri_Pens[SELECTPEN]
#define FTPEN(xyz)              xyz->DrawInfo->dri_Pens[SELECTTEXTPEN]
#define BGPEN(xyz)              xyz->DrawInfo->dri_Pens[MENUBACKGROUNDPEN]
#define HILITE(xyz)		xyz->DrawInfo->dri_Pens[HIGHLIGHTTEXTPEN]
#define TEXTSHADOW(xyz)		xyz->DrawInfo->dri_Pens[BARDETAILPEN]
#define TEXTOUTLINE(xyz)	xyz->DrawInfo->dri_Pens[BARDETAILPEN]

#define BGSHADOW(xyz)           xyz->DrawInfo->dri_Pens[MENUSHADOWPEN]
#define BGSHINE(xyz)            xyz->DrawInfo->dri_Pens[MENUSHINEPEN]
#define HALF(xyz)               xyz->DrawInfo->dri_Pens[MENUBACKGROUNDPEN]

#define PATCH(p, n)		if(PM_Prefs->pmp_MenuTitles & p) pm->Flags|=n; else pm->Flags&=~n;
#define TPATCH(p, n)		if(PM_Prefs->pmp_MenuItems & p) pm->Flags|=n;

#define PM_IDCMP                (IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE|IDCMP_INACTIVEWINDOW|IDCMP_INTUITICKS|IDCMP_REFRESHWINDOW)

#define BWRP                    &a->Wnd->RPort
#define BWX                     (a->Wnd->Left)
#define BWY                     (a->Wnd->Top)
#define BWW                     (a->Wnd->Width)
#define BWH                     (a->Wnd->Height)
#define BWXX                    (a->Wnd->Left+a->Wnd->Width)
#define BWYY                    (a->Wnd->Top+a->Wnd->Height)

//
// Extended flags
//

#define NPX_IMGPLANAR       0x0000          // 
#define NPX_IMGCHUNKY       0x0001          // 
#define NPX_IMGVECTOR       0x0002          // 
#define NPX_TXTNORMAL       0x0000          // Normal (STRPTR) title
#define NPX_TXTLOCALE       0x0010          // Localized title
#define NPX_TXTBOOPSI       0x0020          // BOOPSI Object title

#define GET_TXTMODE(x)      (x->ExtFlags&0x0030)
#define GET_IMGMODE(x)      (x->ExtFlags&0x0003)

#define SET_TXTMODE(x, y)   { x->ExtFlags&=~0x0030; x->ExtFlags|=y; }
#define SET_IMGMODE(x, y)   { x->ExtFlags&=~0x0003; x->ExtFlags|=y; }

//
// Built-in Images
//

                        //            /
#define IMG_CHECKMARK       (0x10L)     // Checkmark        \/
#define IMG_RIGHTARROW      (0x0CL)     // SysIClass Arrow  |>|
#define IMG_MXIMAGE     (0x0FL)     // SysIClass MX     (*)
#define IMG_CHECKIMAGE      (0x0EL)     // SysIClass Checkbox   | |
#define IMG_AMIGAKEY        (0x11L)     // Amiga Key Image  |A|

#define IMG_MMCHECK     (0x7000L)   // MagicMenu CheckMark
#define IMG_MMAMIGA     (0x7001L)   // MagicMenu AmigaKey
#define IMG_MMSUB       (0x7002L)   // MagicMenu Submenu arrow
#define IMG_MMEXCLUDE       (0x7003L)   // MagicMenu MX Image

#define IMG_BULLET_A        (0x8000L)   // bullet       ·
#define IMG_BULLET_B        (0x8100L)   // 3d bullet        ·
#define IMG_ARROW_A     (0x8200L)   // Arrow        >
#define IMG_ARROW_B     (0x8300L)   // 3d arrow     >
#define IMG_ARROW_C     (0x8400L)   // Arrow        ->

struct PM_IDLst {
        struct PM_IDLst         *Next;
        ULONG                   ID;
        UBYTE                   Kind;
        UBYTE                   Flags;
};

#define IDKND_EXCLUDE           0x1
#define IDKND_INCLUDE           0x2
#define IDKND_REFLECT           0x3
#define IDKND_INVERSE           0x4

struct PM_Root;

//
// PM_Window holds menu window information 
//
struct PM_Window {
	struct PM_Window		*Prev;		// Previous window in the chain, for bounds checking of the entire menu
	struct PM_Window		*NextWindow;	// Points to a submenu, while it's open

	UWORD				Width;		// Menu size differs from window size, because of shadows.
	UWORD				Height;		//
	UWORD				MenuX;
	UWORD				MenuY;

	struct Window   		*Wnd;		// The menu window
	struct RastPort			*RPort;		// Window RastPort, NEVER use wnd->RPort

	struct PM_WBackground {				// Background buffer for transparency or backdrop images
		UBYTE			*BgArray;
	} bg;

	struct PM_EffectBuffer {			// Used for transition effects
	        struct BitMap   	*BMap;
		struct RastPort 	*RPort;		// RastPort, PM_Window::RPort will be redirected here when TE's are used
	} te;

	struct PopupMenu		*Selected;	// Currently selected item (the one in 'focus'), or NULL
        struct PopupMenu 		*PrevSel;       // Previously selected
        struct PopupMenu		*WasSelected;   // Was selected when button was released
        struct PopupMenu		*SubMenuToOpen;
        struct PopupMenu		*SubMenuParent;	// For construct hook

        LONG				AltXPos;	// Alternate X position

        BOOL				Running;        // TRUE while active, setting to FALSE will terminate this window
							// Can be set to TRUE by submenus

	BOOL				MenuDisabled;   // Entire menu is disabled
        ULONG				IconColumn;     // Width of Icon Column(s)
        ULONG				ItemHeight;     // Height of items
        ULONG				MenuLevel;      // Wich level of submenu this is
        BOOL				FirstTime;	// Pulldown menus need to be drawn differently the first time
	BOOL				ReverseDirection;	// Open menus to the left
	BOOL				StickyFlag;	// Sticky mode: MB is down => multiselect on, close on up

	PMTRList			*Topographic;	// Shadow management - topography
	PMSRList			*Shadowmap;	// Shadow management - overlapping control

        struct PopupMenu		PM;

        struct PM_Root		*p;		// Menu globals
};

struct MultiSlct {
        struct MultiSlct        *next;
        struct PopupMenu        *pm;
};

//
// Root menu structure, holds "global" variables
//

struct PM_Root {
        struct Window        	*RootWnd;	// Window that the menu belongs to
	struct Screen		*Scr;		// Menu screen
        struct DrawInfo         *DrawInfo;      // Screen's drawinfo
        struct PopupMenu        *PM;		// The menu
        struct PM_Window	*RootMenu;	// First window in the menu

        ULONG                   Subtimer;       // Timer for submenu
        APTR                    ReturnCode;     // Return code
        ULONG                   ReturnID;       // Return ID

        LONG                    BorderWidth;    // Line width of menu borders
        LONG                    BorderHeight;   // Line Height of menu borders

        // Menu options
        BOOL                    PullDown;       // This menu is a pulldown menu

        // Shadows
        
        UWORD                   ShadowWidth;    // Width of shadows
        UWORD                   ShadowHeight;   // Height of shadows
        UWORD                   ShadowAddX;     // Width increase for each layer
        UWORD                   ShadowAddY;     // Height increase for each layer
        
        //
        // Timeout
        //

        ULONG                   TimeOut;
        struct timerequest      *treq;
        struct MsgPort          *tport;

	//
	// Current Timer - Used to figure out when to refresh menus (update hook for dynamic menus)
	//

	ULONG Secs, Micros;

        //
        // MultiSelect
        //

        struct MultiSlct        *MultiSel;
        BOOL                    DoMultiSel;
        ULONG                   MultiSelCount;

        //
        // Hooks
        //

        struct Hook		*MenuHandler;	// Handles multiselect
        struct Hook		*LocaleHook;	// Translates numbers to strings

        //
        // Button that opened the menu
        //

        BOOL    RButton;
        BOOL    LButton;

	//
	// Menu position
	//

	BOOL            MenuCenter; //
	UWORD           MenuHeight; //
	UWORD           MenuWidth;  // Only used for the first menu
	UWORD           MenuRight;  //
	UWORD           MenuBottom; //

	//
	// Menu font - All font operations must use this font!
	//

	struct TextFont     *MenuFont;

	//
	// HintBox
	//

	BOOL            HintBox;



	BOOL            DoneMulti;  // MultiSelect has been used => turns SingleSelect off

	struct PM_InputHandler *pmh;

	struct Image *MenuImages[PMIMG_LAST];
};

extern struct PM_Root *p;

///

#define IST_ACTIVE              1
#define IST_INACTIVE            2

#define PMACT_DESELECT          2
#define PMACT_SELECT            3

/// Protos
//
// Prototypes
//

#ifndef PM_NOPROTOS

void PM_DrawShadow(struct PM_Window *w, int x, int y, int xb, int yb);
struct PM_Root *PM_AllocPMRoot(struct Window *w);

void PM_FreeSubWindow(struct PM_Window *parent, struct PM_Window *a);
struct PM_Window *PM_SetupSubWindow(struct PM_Window *parent, struct PM_Root *p, struct PopupMenu *pm);
APTR PM_DoPopup(struct PM_Window *a);
void PM_CloseWindow(struct PM_Window *);
void StripIntuiMessages(struct MsgPort *, struct Window *);
BOOL PM_OpenWindow(struct PM_Window *pw, int left, int top, int width, int height, struct Screen *scr);
void PM_ResizeWindow(struct PM_Window *bw, int l, int t, int w, int h);
BOOL PM_InsideWindows(int px, int py, struct PM_Window *wnd);
UWORD PM_ItemHeight(struct PM_Window *a, struct PopupMenu *pm);
UWORD PM_ItemWidth(struct PM_Window *a, struct PopupMenu *pm);
void PM_LayoutMenu(struct PM_Window *a);
void PM_RenderMenu(struct PM_Window *a, BOOL MenuDisable, BOOL);
void LoadPref(void);

void EZDeleteTimer(struct timerequest *);
struct timerequest *EZCreateTimer(LONG Unit);

//
// PMCreate.c
//

void PM_FreeTitle(struct PopupMenu *p);

//
// PMError.C
//

LONG EZReq(STRPTR Text, STRPTR ButtonText, ULONG Arg, ...);

//
// PMLayout.C
//

void PM_LayoutMenu(struct PM_Window *a);
void PM_LayoutGroup(struct PM_Window *a, struct PopupMenu *pm);
void PM_CalcItemSize(struct PM_Window *a, struct PopupMenu *pm);
UWORD PM_ItemWidth(struct PM_Window *a, struct PopupMenu *pm);
UWORD PM_ItemHeight(struct PM_Window *a, struct PopupMenu *pm);

//
// PMInit.C
//

#ifndef INIT
BOOL OpenLibs(void);
void CloseLibs(void);
#endif

//
// Globals
//

extern BOOL V40Gfx;
extern BOOL CyberGfx;
extern APTR MemPool;

//
// Lib funcs
//

APTR __saveds ASM PM_OpenPopupMenuA(register __a1 struct Window *prevwnd GNUCREG(a1),
    register __a2 struct TagItem *tags GNUCREG(a2));
      // register __a6 struct PopupMenuBase *l GNUCREG(a6));

void __saveds ASM PM_FreePopupMenu(register __a1 struct PopupMenu *p GNUCREG(a1));

struct PopupMenu * __saveds ASM PM_MakeItemA(register __a1 struct TagItem *tags GNUCREG(a1));

struct PopupMenu *  __saveds ASM PM_MakeMenuA(register __a1 struct TagItem *tags GNUCREG(a1));

struct PM_IDLst * __saveds ASM PM_MakeIDListA(register __a1 struct TagItem *tags GNUCREG(a1));

LONG __saveds ASM PM_GetItemAttrsA(register __a2 struct PopupMenu *p GNUCREG(a2),
    register __a1 struct TagItem *tags GNUCREG(a1));

LONG __saveds ASM PM_SetItemAttrsA(register __a2 struct PopupMenu *p GNUCREG(a2),
    register __a1 struct TagItem *tags GNUCREG(a1));

void __saveds ASM PM_AlterState(register __a1 struct PopupMenu *base GNUCREG(a1),
    register __a2 struct PM_IDLst *ids GNUCREG(a2),
    register __d1 UWORD action GNUCREG(d1));

struct PM_IDLst * __saveds ASM PM_ExLstA(register __a1 ULONG *id GNUCREG(a1));

APTR __asm __saveds LIBPM_FilterIMsgA(register __a0 struct Window *w GNUCREG(a0),
    register __a1 struct PopupMenu *pm GNUCREG(a1),
    register __a2 struct IntuiMessage *im GNUCREG(a2),
    register __a3 struct TagItem *tags GNUCREG(a3));

struct PopupMenu * __saveds ASM PM_FindItem(register __a1 struct PopupMenu *pm GNUCREG(a1),
    register __d1 ULONG ID GNUCREG(d1));

BOOL __saveds ASM PM_ItemChecked(register __a1 struct PopupMenu *pm GNUCREG(a1),
    register __d1 ULONG id GNUCREG(d1));


#endif
///

#endif
