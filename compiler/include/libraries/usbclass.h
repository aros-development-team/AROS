/*
 *----------------------------------------------------------------------------
 *                         Includes for usb class libraries
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <hodges@in.tum.de>
 *
 * History
 *
 *  08-05-2002  - Initial
 *  16-10-2004  - Extended some parts
 *
 */

#ifndef USBCLASS_H
#define USBCLASS_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <utility/pack.h>

/* Types for usbGetAttrs() and usbSetAttrs() */

#define UGA_CLASS      0x01
#define UGA_BINDING    0x02
#define UGA_CONFIG     0x03

/* Tags for usbGetAttrs(UGA_CLASS,...) */

#define UCCA_Dummy           (TAG_USER + 4489)
#define UCCA_Priority        (UCCA_Dummy + 0x01)
#define UCCA_Description     (UCCA_Dummy + 0x02)
#define UCCA_HasClassCfgGUI  (UCCA_Dummy + 0x10)
#define UCCA_HasBindingCfgGUI (UCCA_Dummy + 0x11)
#define UCCA_AfterDOSRestart (UCCA_Dummy + 0x20)
#define UCCA_UsingDefaultCfg (UCCA_Dummy + 0x30)
#define UCCA_SupportsSuspend (UCCA_Dummy + 0x40)

/* Tags for usbGetAttrs(UGA_BINDING,...) */

#define UCBA_Dummy           (TAG_USER  + 103)
#define UCBA_UsingDefaultCfg (UCBA_Dummy + 0x30)

/* Tags for usbGetAttrs(UGA_CONFIG,...) */

#define UCFA_Dummy          (TAG_USER  + 2612)

/* Methods for usbDoMethod() */

#define UCM_AttemptInterfaceBinding 0x0001
#define UCM_ForceInterfaceBinding   0x0002
#define UCM_ReleaseInterfaceBinding 0x0003
#define UCM_AttemptDeviceBinding    0x0004
#define UCM_ForceDeviceBinding      0x0005
#define UCM_ReleaseDeviceBinding    0x0006
#define UCM_OpenCfgWindow           0x0020
#define UCM_CloseCfgWindow          0x0021
#define UCM_OpenBindingCfgWindow    0x0022 /* { binding } */
#define UCM_CloseBindingCfgWindow   0x0023 /* { binding } */
#define UCM_LocaleAvailableEvent    0x0030
#define UCM_DOSAvailableEvent       0x0031
#define UCM_ConfigChangedEvent      0x0032
#define UCM_SoftRestart             0x0040
#define UCM_HardRestart             0x0041
#define UCM_AttemptSuspendDevice    0x0050 /* success = { binding } */
#define UCM_AttemptResumeDevice     0x0051 /* success = { binding } */

/* only for hubs */
#define UCM_HubPowerCyclePort       0x0f01 /* { device, portnumber } */
#define UCM_HubClassScan            0x0f02 /* { hubbinding } */
#define UCM_HubClaimAppBinding      0x0f03 /* { hubbinding, taglist } */
#define UCM_HubReleaseIfBinding     0x0f04 /* { hubbinding, if } */
#define UCM_HubReleaseDevBinding    0x0f05 /* { hubbinding, device } */
#define UCM_HubDisablePort          0x0f06 /* { device, portnumber } */
#define UCM_HubSuspendDevice        0x0f07 /* { hubbinding, device } */
#define UCM_HubResumeDevice         0x0f08 /* { hubbinding, device } */

#endif /* USBCLASS_H */
