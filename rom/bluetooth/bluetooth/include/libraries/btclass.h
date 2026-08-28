/*
 *----------------------------------------------------------------------------
 *                Includes for Bluetooth class libraries
 *----------------------------------------------------------------------------
 *
 * A Bluetooth class is a library in SYS:Classes/Bluetooth exporting
 * btcGetAttrsA(), btcSetAttrsA() and btcDoMethodA() (see btclass.conf), the
 * counterpart of the Poseidon usb classes. bluetooth.library offers registered
 * devices and their services to every class in priority order through the
 * BCM_Attempt* methods; a class that takes a binding usually spawns a task
 * that allocates channels on the device's endpoints.
 */

#ifndef LIBRARIES_BTCLASS_H
#define LIBRARIES_BTCLASS_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <utility/pack.h>

/* Types for btcGetAttrs() and btcSetAttrs() */

#define BCGA_CLASS      0x01
#define BCGA_BINDING    0x02
#define BCGA_CONFIG     0x03

/* Tags for btcGetAttrs(BCGA_CLASS,...) */

#define BCCA_Dummy            (TAG_USER + 0xB000)
#define BCCA_Priority         (BCCA_Dummy + 0x01) /* SIPTR: binding order, higher first */
#define BCCA_Description      (BCCA_Dummy + 0x02) /* STRPTR */
#define BCCA_HasClassCfgGUI   (BCCA_Dummy + 0x10) /* BOOL */
#define BCCA_HasBindingCfgGUI (BCCA_Dummy + 0x11) /* BOOL */
#define BCCA_AfterDOSRestart  (BCCA_Dummy + 0x20) /* BOOL: rebind after DOS is up */
#define BCCA_UsingDefaultCfg  (BCCA_Dummy + 0x30) /* BOOL */

/* Tags for btcGetAttrs(BCGA_BINDING,...) */

#define BCBA_Dummy            (TAG_USER + 0xB080)
#define BCBA_UsingDefaultCfg  (BCBA_Dummy + 0x30) /* BOOL */
#define BCBA_Device           (BCBA_Dummy + 0x31) /* APTR BtDevice */
#define BCBA_Service          (BCBA_Dummy + 0x32) /* APTR BtService or NULL */
#define BCBA_Task             (BCBA_Dummy + 0x33) /* struct Task * of the binding */

/* Tags for btcGetAttrs(BCGA_CONFIG,...) */

#define BCFA_Dummy            (TAG_USER + 0xB0C0)

/* Methods for btcDoMethod() */

#define BCM_AttemptServiceBinding   0x0001 /* { service } -> binding or NULL */
#define BCM_ForceServiceBinding     0x0002 /* { service } -> binding or NULL */
#define BCM_ReleaseServiceBinding   0x0003 /* { binding } */
#define BCM_AttemptDeviceBinding    0x0004 /* { device } -> binding or NULL */
#define BCM_ForceDeviceBinding      0x0005 /* { device } -> binding or NULL */
#define BCM_ReleaseDeviceBinding    0x0006 /* { binding } */
#define BCM_OpenCfgWindow           0x0020
#define BCM_CloseCfgWindow          0x0021
#define BCM_OpenBindingCfgWindow    0x0022 /* { binding } */
#define BCM_CloseBindingCfgWindow   0x0023 /* { binding } */
#define BCM_LocaleAvailableEvent    0x0030
#define BCM_DOSAvailableEvent       0x0031
#define BCM_ConfigChangedEvent      0x0032
#define BCM_SoftRestart             0x0040
#define BCM_HardRestart             0x0041
#define BCM_DeviceConnected         0x0050 /* { binding } link came up */
#define BCM_DeviceDisconnected      0x0051 /* { binding, (IPTR) reason } link went down */

#endif /* LIBRARIES_BTCLASS_H */
