//
// Menu Prefs
//

#ifndef PM_PREFS_H
#define PM_PREFS_H

#include "prefs/popupmenu.h"

/// Frames
#define BUTTON_FRAME        0
#define MAGIC_FRAME     1
#define THICK_BUTTON_FRAME  2
#define DOUBLE_FRAME        3
#define DROPBOX_FRAME       4
#define INTUI_FRAME     5
///

/// TextPatch
#define TP_CENTER       0x0001
#define TP_UNDERLINE        0x0002
#define TP_BOLD         0x0004
#define TP_SHINE        0x0008
#define TP_SHADOW       0x0010
#define TP_TEXT         0x0020
#define TP_HILITE       0x0040
#define TP_SHADOWED     0x0080
#define TP_LEFT         0x0100
#define TP_RIGHT        0x0200
#define TP_EMBOSS       0x0400
#define TP_KILLBAR      0x0800
#define TP_OUTLINE      0x1000
#define TP_ACTIVATE     0x8000
///

/// File name/ID
#define PMP_ID		(0x504d4e55)
#define PMP_VERSION	1
#define PMP_PATH	"ENV:sys/PopupMenu.prefs"
///

extern struct PopupMenuPrefs	*PM_Prefs;

void PM_Prefs_Load(STRPTR file);
void PM_Prefs_Free();

#endif
