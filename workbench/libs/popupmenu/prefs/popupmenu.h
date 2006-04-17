#ifndef PREFS_POPUPMENU_H
#define PREFS_POPUPMENU_H
/*
**	$VER: popupmenu.h 50.1 (15.5.2002)
**	Includes Release 50.1
**
**	File format for example preferences
**
**	(C) Copyright 2002 Amiga, Inc.
**	All Rights Reserved
*/

/*****************************************************************************/


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif


/*****************************************************************************/


#define ID_PMNU MAKE_ID('P','M','N','U')


struct PopupMenuPrefs
{
    UBYTE pmp_Flags;		/* Leave at 0 for now			*/
    UBYTE pmp_SubMenuDelay;	/* Delay before opening submenus	*/
    UBYTE pmp_Animation;	/* Animation, see below for defines	*/
    UBYTE pmp_PulldownPos;	/* Where to show pulldownmenus		*/
    BOOL  pmp_Sticky;		/* Use 'sticky' mode			*/
    BOOL  pmp_SameHeight;	/* Try to give all items same height	*/
    UBYTE pmp_MenuBorder;	/* Menu border				*/
    UBYTE pmp_SelItemBorder;	/* Border around selected item		*/
    UBYTE pmp_SeparatorBar;	/* Separator bar style			*/
    UBYTE pmp_MenuTitles;	/* Flags for menu titles		*/
    UBYTE pmp_MenuItems;	/* Flags for menu items			*/
    UBYTE pmp_XOffset;
    UBYTE pmp_YOffset;
    UBYTE pmp_XSpace;
    UBYTE pmp_YSpace;
    UBYTE pmp_Intermediate;
    BYTE  pmp_TextDisplace;
    BYTE  pmp_ShadowR;
    BYTE  pmp_ShadowG;
    BYTE  pmp_ShadowB;
    BYTE  pmp_TransparencyR;
    BYTE  pmp_TransparencyG;
    BYTE  pmp_TransparencyB;
    UBYTE pmp_TransparencyBlur;
    UBYTE pmp_AnimationSpeed;
    UBYTE pmp_Reserved[16];	/* Reserved for future use		*/
};

#define PMP_ANIM_NONE		0
#define PMP_ANIM_ZOOM		1
#define PMP_ANIM_FADE		2
#define PMP_ANIM_EXPLODE	3

#define PMP_PD_SCREENBAR	0
#define PMP_PD_WINDOWBAR	1
#define PMP_PD_MOUSE		2	/* Show as popup menu */

#define PMP_TITLE_NORMAL	0x00
#define PMP_TITLE_ITALIC	0x01
#define PMP_TITLE_BOLD		0x02
#define PMP_TITLE_UNDERLINE	0x04
#define PMP_TITLE_SHADOW	0x08
#define PMP_TITLE_EMBOSS	0x10
#define PMP_TITLE_OUTLINE	0x20

#define PMP_TEXT_NORMAL		0x00
#define PMP_TEXT_ITALIC		0x01
#define PMP_TEXT_BOLD		0x02
#define PMP_TEXT_UNDERLINE	0x04
#define PMP_TEXT_SHADOW		0x08
#define PMP_TEXT_EMBOSS		0x10
#define PMP_TEXT_OUTLINE	0x20

#define PMP_MENUBORDER_THIN	0
#define PMP_MENUBORDER_MM	1
#define PMP_MENUBORDER_THICK	2
#define PMP_MENUBORDER_RIDGE	3
#define PMP_MENUBORDER_DROPBOX	4
#define PMP_MENUBORDER_OLDSTYLE	5

#define PMP_SELITEM_NO_BORDER	0
#define PMP_SELITEM_RECESS	1
#define PMP_SELITEM_RAISE	2

/*****************************************************************************/


#endif /* PREFS_POPUPMENU_H */
