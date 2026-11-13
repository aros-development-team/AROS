/*
//	$VER: pminternals.h 10.00 (9.11.00)
//
//	Internal structures used by popupmenu.library
//
//	©1996-2000 Henrik Isaksson
//	All Rights Reserved.
//
//	Changes:
//
//	10.00	Initial version
//
//  *WARNING*
//
//  Everything in this file is subject to change.
*/

#ifndef LIBRARIES_PMINTERNALS_H
#define LIBRARIES_PMINTERNALS_H

/*
// Flags
*/
#define NPM_HBAR_BIT            0x00000001          /* Draw a horizontal bar */
#define NPM_NOSELECT            0x00000002          /* Make this item unselectable. */
#define NPM_WIDE_BAR_BIT        0x00000004          /* Almost same as NPM_HBAR_BIT, but wider */
#define NPM_UDATASTRING         0x00000008          /* FreeVec(UserData) when menu is freed */
#define NPM_HIDDEN              0x00000010          /* Hidden */
#define NPM_SELECTABLE          0x00000020          /* Item with SubMenu is selectable */
#define NPM_GROUP               0x00000040          /* Group */
#define NPM_EXCLUDE_SHARED      0x00000080          /* Don't free exclude list */
#define NPM_FIXEDSIZE           0x00000100          /* Fixed size */
#define NPM_ISIMAGE             0x00000200          /* This item has an image not an icon */
#define NPM_OUTLINED            0x00000400          /* Outline text */
#define NPM_HILITETEXT          0x00000800          /* Highlight Text */
#define NPM_FILLTEXT            0x00001000          /* */
#define NPM_SHADOWTEXT          0x00002000          /* */
#define NPM_SHINETEXT           0x00004000          /* */
#define NPM_BOLDTEXT            0x00008000          /* Bold text */
#define NPM_ITALICTEXT          0x00010000          /* Italic text */
#define NPM_UNDERLINEDTEXT      0x00020000          /* Underline text */
#define NPM_EMBOSSED            0x00040000          /* Emboss text */
#define NPM_DISABLED            0x00080000          /* Ghosted text */
#define NPM_CUSTOMPEN           0x00100000          /* Use custom pen */
#define NPM_SHADOWED            0x00200000          /* Shadowed text */
#define NPM_CENTRED             0x00400000          /* Centred text */
#define NPM_ISSELECTED          0x00800000          /* This item has been selected this time */
#define NPM_NOTOGGLE            0x01000000          /* Don't un-check checked checkmarks... =) */
#define NPM_ICONISID            0x02000000          /* FREE */
#define NPM_COLOURBOX           0x04000000          /* Colour box */
#define NPM_INITIAL_CHECKED     0x08000000          /* Checked when menu opened */
#define NPM_FREE03              0x10000000          /* FREE */
#define NPM_MINSIZE             0x20000000          /* Minimum size = fixed size */
#define NPM_CHECKIT             0x40000000			/* Space for checkmark */
#define NPM_CHECKED             0x80000000			/* Checkmark */
#define NPM_WIDE_BAR            (NPM_WIDE_BAR_BIT|NPM_NOSELECT) // Same as NPM_HBAR but it covers the whole window
#define NPM_HBAR                (NPM_HBAR_BIT|NPM_NOSELECT)     // Horizontal bar.

#define NPM_CENTERED			NPM_CENTRED
#define NPM_COLORBOX			NPM_COLOURBOX

#endif /* LIBRARIES_PMINTERNALS_H */
