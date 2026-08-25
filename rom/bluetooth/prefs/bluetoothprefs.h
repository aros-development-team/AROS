/*
** Bluetooth preferences - shared declarations.
**
** The program is split up the same way Poseidon's Trident prefs is:
**   bluetoothprefs.c - main(): application, menus, main window, input loop
**   ActionClass.c    - the big content class (nav + pages + log + buttons)
**   IconListClass.c  - the left hand navigation list (a List subclass)
**   DevWinClass.c    - the device information sub window
** with debug.h for debugging macros.
*/

#ifndef BLUETOOTHPREFS_H
#define BLUETOOTHPREFS_H

#include <exec/types.h>
#include <exec/lists.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>

/* Library bases we open ourselves (Intuition/Utility/DOS come from the C
 * startup and are declared by their proto headers). */
extern struct Library      *MUIMasterBase;
extern struct Library      *BluetoothBase;
extern struct Library      *IconBase;

/* pick a device-class icon (ICON_DEV_*) from Class-of-Device / LE Appearance */
ULONG DeviceIconFor(IPTR cod, IPTR appearance, IPTR isclassic);

/* Custom MUI classes, created by main() */
extern struct MUI_CustomClass *IconListClass;
extern struct MUI_CustomClass *ActionClass;
extern struct MUI_CustomClass *ScanWinClass;
extern struct MUI_CustomClass *DevWinClass;

/* Left navigation pages, in list order. */
enum
{
    BTPAGE_GENERAL = 0,
    BTPAGE_HARDWARE,
    BTPAGE_DEVICES,
    BTPAGE_CLASSES,
    BTPAGE_OPTIONS,
    BTPAGE_CONFIG,
    BTPAGE_COUNT
};

#endif /* BLUETOOTHPREFS_H */
