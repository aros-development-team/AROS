/*
//	$VER: pm.h 10.05 (11.11.00)
//
//	Library base, tags and macro definitions
//	for popupmenu.library.
//
//	©1996-2000 Henrik Isaksson
//	All Rights Reserved.
//
//	Changes:
//
//	9.00	New PopupMenu structure.
//		Several new tags and updated macros.
//	9.01	Added PM_HintBox
//	10.0	Added PM_Toggle, PM_ExcludeShared
//		Added macro PMMXItem
//		Added two flags, PM_CHECKIT and PM_CHECKED
//	10.05	Changed the PopupMenu structure a bit.
//		Added typedef for PopupMenu.
//		Added american english equalients for some tags.
//
*/

#ifndef LIBRARIES_POPUPMENU_H
#define LIBRARIES_POPUPMENU_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef INTUITION_CLASSUSR_H
#include <intuition/classusr.h>
#endif

/*
// Tags passed to PM_OpenPopupMenuA and PM_FilterIMsgA
*/

#define PM_Menu			(TAG_USER+4)	/* (struct PopupMenu *) Pointer to menulist initialized by MakeMenu()		*/
#define PM_Top			(TAG_USER+12)	/* (LONG) Top (Y) position							*/
#define PM_Left			(TAG_USER+13)	/* (LONG) Left (X) position							*/
#define PM_Code			(TAG_USER+14)	/* (UWORD) Obsolete.								*/
#define PM_Right		(TAG_USER+15)	/* (LONG) X position relative to right edge					*/
#define PM_Bottom		(TAG_USER+16)	/* (LONG) Y position relative to bottom edge					*/
#define PM_MinWidth		(TAG_USER+17)	/* (LONG) Minimum width								*/
#define PM_MinHeight		(TAG_USER+18)	/* (LONG) Minimum height							*/
#define PM_ForceFont		(TAG_USER+19)	/* (struct TextFont *tf) Use this font instead of user preferences.		*/
#define PM_PullDown		(TAG_USER+90)	/* (BOOL) Turn the menu into a pulldown menu.					*/
#define PM_MenuHandler		(TAG_USER+91)	/* (struct Hook *) Hook that is called for each selected item, after the	*/
						/* menu has been closed. This tag turns on MultiSelect.				*/
#define PM_AutoPullDown		(TAG_USER+92)	/* (BOOL) Automatic pulldown menu. (PM_FilterIMsg only)				*/
#define PM_LocaleHook		(TAG_USER+93)	/* (struct Hook *) Locale "GetString()" hook. (Not yet implemented)		*/
#define PM_CenterScreen		(TAG_USER+94)	/* (BOOL) Center menu on the screen						*/
#define PM_UseLMB		(TAG_USER+95)	/* (BOOL) Left mouse button should be used to select an item			*/
						/* (right button selects multiple items)					*/
#define PM_DRIPensOnly		(TAG_USER+96)	/* (BOOL) Only use the screen's DRI pens, revert to system images if necessary.	*/
						/* Use with care as it overrides the user's prefs!				*/
#define PM_HintBox		(TAG_USER+97)	/* (BOOL) Close the menu when the mouse is moved.				*/
#define PM_RawKey		(TAG_USER+98)	/* (BOOL) Let PM_FilterIMsgA parse IDCMP_RAWKEY instead of IDCMP_VANILLAKEY.	*/

/*
// Tags passed to MakeItem
*/

#define PM_Title		(TAG_USER+20)	/* (STRPTR) Item title								*/
#define PM_UserData		(TAG_USER+21)	/* (void *) Anything, returned by OpenPopupMenu when this item is selected	*/
#define PM_ID			(TAG_USER+22)	/* (ULONG) ID number, if you want an easy way to access this item later		*/
#define PM_CommKey		(TAG_USER+47)	/* (char) Keyboard shortcut for this item.					*/
#define PM_TitleID		(TAG_USER+49)	/* (ULONG) Locale string ID 							*/
#define PM_Object		(TAG_USER+43)	/* (Object *) BOOPSI object with the abillity to render this item		*/

/* Submenu & Layout tags */
/* PM_Sub & PM_Members are mutally exclusive */
#define PM_Sub			(TAG_USER+23)	/* (PopupMenu *) Pointer to submenu list (from PM_MakeMenu)			*/
#define PM_Members		(TAG_USER+65)	/* (PopupMenu *) Members for this group (list created by PM_MakeMenu)		*/
#define PM_LayoutMode		(TAG_USER+64)	/* (ULONG) Layout method (PML_Horizontal / PML_Vertical)			*/

/* Text attributes */
#define PM_FillPen		(TAG_USER+26)	/* (BOOL) Make the item appear in FILLPEN					*/
#define PM_Italic		(TAG_USER+29)	/* (BOOL) Italic text								*/
#define PM_Bold			(TAG_USER+30)	/* (BOOL) Bold text								*/
#define PM_Underlined		(TAG_USER+31)	/* (BOOL) Underlined text							*/
#define PM_ShadowPen		(TAG_USER+34)	/* (BOOL) Draw text in SHADOWPEN						*/
#define PM_ShinePen		(TAG_USER+35)	/* (BOOL) Draw text in SHINEPEN							*/
#define PM_Centre		(TAG_USER+36)	/* (BOOL) Center the text of this item						*/
#define PM_Center		PM_Centre	/* American version... */
#define PM_TextPen		(TAG_USER+45)	/* (ULONG) Pen number for text colour of this item				*/
#define PM_Shadowed		(TAG_USER+48)	/* (BOOL) Draw a shadow behind the text						*/

/* Other item attributes */
#define PM_TitleBar		(TAG_USER+32)	/* (BOOL) Horizontal separator bar						*/
#define PM_WideTitleBar		(TAG_USER+33)	/* (BOOL) Same as above, but full width						*/
#define PM_NoSelect		(TAG_USER+25)	/* (BOOL) Make the item unselectable (without visual indication)		*/
#define PM_Disabled		(TAG_USER+38)	/* (BOOL) Disable an item							*/
#define PM_Hidden		(TAG_USER+63)	/* (BOOL) This item is not to be drawn (nor used in the layout process)		*/

/* Images & Icons */
#define PM_ImageSelected	(TAG_USER+39)	/* (struct Image *) Image when selected, title will be rendered on top it	*/
#define PM_ImageUnselected	(TAG_USER+40)	/* (struct Image *) Image when unselected					*/
#define PM_IconSelected		(TAG_USER+41)	/* (struct Image *) Icon when selected						*/
#define PM_IconUnselected	(TAG_USER+42)	/* (struct Image *) Icon when unselected					*/

/* Check/MX items */
#define PM_Checkit		(TAG_USER+27)	/* (BOOL) Leave space for a checkmark						*/
#define PM_Checked		(TAG_USER+28)	/* (BOOL) Make this item is checked						*/
#define PM_AutoStore		(TAG_USER+44)	/* (BOOL *) Pointer to BOOL reflecting the current state of the item		*/
#define PM_Exclude		(TAG_USER+37)	/* (PM_IDLst *) Items to unselect or select when this gets selected		*/
#define PM_ExcludeShared	(TAG_USER+101)	/* (BOOL) Used if the list is shared between two or more items			*/
#define PM_Toggle		(TAG_USER+100)	/* (BOOL) Enable/disable toggling of check/mx items. Default: TRUE		*/

/* Dynamic construction/destruction */
#define PM_SubConstruct		(TAG_USER+61)	/* (struct Hook *) Constructor hook for submenus. Called before menu is opened.	*/
#define PM_SubDestruct		(TAG_USER+62)	/* (struct Hook *) Destructor hook for submenus. Called after menu has closed.	*/

/* Special/misc. stuff */
#define PM_UserDataString	(TAG_USER+46)	/* (STRPTR) Allocates memory and copies the string to UserData.			*/
#define PM_Flags		(TAG_USER+24)	/* (UlONG) For internal use							*/
#define PM_ColourBox		(TAG_USER+60)	/* (UlONG) Filled rectangle (for palettes etc.)					*/
#define PM_ColorBox		PM_ColourBox	/* For Americans... */
/*
// Tags passed to MakeMenu
*/

#define PM_Item			(TAG_USER+50)	/* (PopupMenu *) Item pointer from MakeItem					*/
#define PM_Dummy		(TAG_USER+51)	/* (void) Ignored.								*/

/*
// Tags passed to MakeIDList
*/

#define PM_ExcludeID		(TAG_USER+55)	/* (ULONG) ID number of menu to deselect when this gets selected		*/
#define PM_IncludeID		(TAG_USER+56)	/* (ULONG) ID number of menu to select when this gets selected			*/
#define PM_ReflectID		(TAG_USER+57)	/* (ULONG) ID number of menu that should reflect the state of this one		*/
#define PM_InverseID		(TAG_USER+58)	/* (ULONG) ID number of menu to inverse reflect the state of this one		*/

/*
// Tags for PM_InsertMenuItemA()
*/

#define PM_Insert_Before	(TAG_USER+200)	/* (BOOL) Insert before the item pointed to by the following argument	(N/A)	*/
#define PM_Insert_BeforeID	(TAG_USER+201)	/* (ULONG) Insert before the first item with ID equal to the argument		*/
#define PM_Insert_After		(TAG_USER+202)	/* (PopupMenu *) Insert after the item pointed to by the following argument	*/
#define PM_Insert_AfterID	(TAG_USER+203)	/* (ULONG) Insert after the first item with ID equal to the argument		*/
#define PM_Insert_Last		(TAG_USER+205)	/* (BOOL) Insert after the last item						*/
#define PM_Insert_First		(TAG_USER+209)	/* (BOOL) Insert after the first item (which is usually invisible)		*/
#define PM_InsertSub_First	(TAG_USER+206)	/* (PopupMenu *) Insert before the first item in the submenu			*/ 
#define PM_InsertSub_Last	(TAG_USER+207)	/* (PopupMenu *) Insert at the and of a submenu					*/
#define PM_InsertSub_Sorted	(TAG_USER+208)	/* (PopupMenu *) 							(N/A)	*/
#define PM_Insert_Item		(TAG_USER+210)	/* (PopupMenu *) Item to insert, may be repeated for several items		*/

/*
// Layout methods
*/

#define PML_None		0		/* Normal item		*/
#define PML_Horizontal		1		/* Horizontal group	*/
#define PML_Vertical		2		/* Vertical group	*/
#define PML_Table		3		/* Table group		*/
#define PML_Default		255		/* Don't use		*/

/*
// Macros
*/

#define PMMenu(t)	PM_MakeMenu(\
			PM_Item, PM_MakeItem(PM_Hidden, TRUE, TAG_DONE),\
			PM_Item, PM_MakeItem(PM_Title, t, PM_NoSelect, TRUE, PM_ShinePen, TRUE, PM_Shadowed, TRUE, PM_Center, TRUE, TAG_DONE),\
			PM_Item, PM_MakeItem(PM_WideTitleBar, TRUE, TAG_DONE)
#define PMMenuID(t)	PM_MakeMenu(\
			PM_Item, PM_MakeItem(PM_Hidden, TRUE, TAG_DONE),\
			PM_Item, PM_MakeItem(PM_TitleID, t, PM_NoSelect, TRUE, PM_ShinePen, TRUE, PM_Shadowed, TRUE, PM_Center, TRUE, TAG_DONE),\
			PM_Item, PM_MakeItem(PM_WideTitleBar, TRUE, TAG_DONE)
#define PMSubMenu(t)	PM_Sub, PM_MakeMenu(PM_Item, PM_MakeItem(PM_Title, t, PM_NoSelect, TRUE, PM_ShinePen, TRUE, TAG_DONE),\
			PM_Item, PM_MakeItem(PM_WideTitleBar, TRUE, TAG_DONE)
#define PMSimpleSub	PM_Sub, PM_MakeMenu(PM_Dummy, 0
#define PMItem(t)	PM_Item, PM_MakeItem(PM_Title, t
#define PMItemID(t)	PM_Item, PM_MakeItem(PM_TitleID, t
#define PMInfo(t)	PM_Item, PM_MakeItem(PM_Title, t, PM_NoSelect, TRUE, PM_ShinePen, TRUE
#define PMColBox(c)	PM_Item, PM_MakeItem(PM_ColourBox, c
#define PMBar		PM_Item, PM_MakeItem(PM_TitleBar, TRUE
#define PMMenuTitle(t)	PM_Item, PM_MakeItem(PM_Title, t, PM_NoSelect, TRUE, PM_ShinePen, TRUE, PM_Shadowed, TRUE, PM_Center, TRUE, TAG_DONE),\
			PM_Item, PM_MakeItem(PM_WideTitleBar, TRUE, TAG_DONE)
#define PMHoriz		PM_Item, PM_MakeItem(PM_NoSelect, TRUE, PM_LayoutMode, PML_Horizontal
#define PMVert		PM_Item, PM_MakeItem(PM_NoSelect, TRUE, PM_LayoutMode, PML_Vertical
#define PMMembers	PM_Members, PM_MakeMenu(PM_Dummy, 0

#define PMExcl		PM_Exclude, PM_MakeIDList(
#define ExID(id)	PM_ExcludeID, id
#define InID(id)	PM_IncludeID, id
#define RefID(id)	PM_ReflectID, id
#define InvID(id)	PM_InverseID, id

#define PMCheckItem(t,id)	PM_Item, PM_MakeItem(PM_Title, t, PM_ID, id, PM_Checkit, TRUE

#define PMMXItem(t,id)	PM_Item, PM_MakeItem(PM_Title, t, PM_ID, id, PM_Checkit, TRUE, PM_Toggle, FALSE

#define PMEnd		TAG_DONE)

#ifndef End
#define End		TAG_DONE)
#endif

#define PMERR		(-5L)

/* For compatibility with old sources - DO NOT USE */
#define PMTitleBar	PMBar
#define PMNarrowBar	PMBar
#define PMNarrowTitleBar PMBar

/*
// Library base
*/

#ifndef PM_NOBASE

struct PopupMenuBase {
	struct Library		pmb_Library;
	ULONG			pmb_SegList;
	ULONG			pmb_Flags;
	struct Library		*pmb_ExecBase;		/* Theese library */
	struct Library		*pmb_UtilityBase;	/* pointers are   */
	struct Library		*pmb_IntuitionBase;	/* valid as long  */
	struct Library		*pmb_GfxBase;		/* as pm lib      */
	struct Library		*pmb_DOSBase;		/* is open.       */
	BOOL			pmb_NewPrefs;		/* Reload prefs.  */
	struct Library		*pmb_CxBase;		/* commodities.   */
	struct Library		*pmb_LayersBase;	/* layers.library */
	struct Library		*pmb_CyberGfxBase;	/* cybergfx.lib   */
};

#endif

#define POPUPMENU_VERSION	10L
#define POPUPMENU_NAME		"popupmenu.library"
#define OPEN_PM_LIB			(PopupMenuBase=(struct PopupMenuBase *)\
							OpenLibrary(POPUPMENU_NAME, POPUPMENU_VERSION))
#define CLOSE_PM_LIB		if(PopupMenuBase) CloseLibrary((struct Library *)PopupMenuBase);

/*
// PopupMenu structure
//
// Note:
// This structure may change in future versions.
// Do not read or write fields directly, use PM_Set/GetPopupMenuAttrs()
*/

struct PopupMenu {
    struct PopupMenu    	*Next;		/* Next item in menu */
    union {
    	struct PopupMenu	*Sub;		/* Sub menu pointer */
    	struct PopupMenu	*Group;		/* Group members */
	};

    union {
        STRPTR      		Title;      /* Title */
        ULONG       		TitleID;    /* Locale string ID */
        Object      		*Boopsi;    /* Boopsi object */
    };

    ULONG           		Flags;		/* Flags */
    ULONG           		ID;     	/* Item ID */
    APTR            		UserData;   /* UserData */
    
    WORD            		Left;       /* Left pos of this item */
    WORD            		Top;        /* Top pos of this item */
    UWORD           		Width;      /* Width of this item */
    UWORD           		Height;     /* Height of this item */

	/* Very private and undocumented stuff follows. */
	/* Mess with it at your own risk. */

    UWORD           		ExtFlags; 	/* Extended flags */

    UBYTE           		Layout;		/* Layout mode */
    UBYTE           		CBox;       /* ColourBox pen */

    struct PM_IDLst     	*Exclude;   /* Exclude/Included/Reflected items */
    BOOL            		*AutoSetPtr;/* Ptr to BOOL containing current state */

    union {
    	struct Image       	*Images[2]; /* Images/Icons (0 - unselected, 1 - sel) */
		STRPTR				IconID;
	};

    UBYTE           		CommKey;    /* Command Key */
    UBYTE          			Weight;     /* Weight */
    struct Hook     		*SubConstruct;  /* SubMenu Constructor hook */
    struct Hook     		*SubDestruct;   /* SubMenu Destructor hook */
    UBYTE           		TextPen;    /* Pen number for item's text */
    UBYTE           		Pad;

	APTR			Image;
};

typedef struct PopupMenu PopupMenu;

/* Public flags for the PopupMenu->Header.Flags field */

#define PM_CHECKIT             0x40000000
#define PM_CHECKED             0x80000000

#endif
