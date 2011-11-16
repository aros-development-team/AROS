/****************************************************************************

                 __   __                                    V/\V.       /\
                |" | |" |                                   mMMnw,      || []
                |  | |  |                                  (o  o)W   () || ||
                |__|_|_"|                                  | /  |Mw  || ||//
                ("  "  \|                                  \ -'_/mw   \\||/
                 \______)                                   ~%%/WM"    \||
 _____    ___     ______  _____  __  _____     ___  __  __/~~__ ~~\    _||
|"("  \()/\" \ ()/"_    )|"(___) ) )|"("  \ ()/\" \(__)/" ) /" ) " \  /_)O
|  )   )/" \  \ (_/"\__/ |  )_  ( ( |  )_  ) /" \  \  /  /|/  / ·\  \/ ,|O
| (___/(  (_\__) _\  \_  | (__)  ) )| (__) |(  (_\__)/  /"/  /   |\   '_|O
|  |  _ \  /  / /" \_/ ) | ")__ ( ( |  )"  ) \  /  //  /|/  / . .|/\__/ ||
|__| (_) \/__/ (______/  |_(___) )_)|_(___/ . \/__/(__/ (__/ .:.:|      ||
                 _____
                |" __ \  Poseidon -- The divine USB stack for Amiga computers
                | (__) ) Version: 4.3 (30.05.09)
                |  __ (  Designed and written by
                |"(__) )   Chris Hodges <chrisly@platon42.de>
                |_____/  Copyright ©2002-2009 Chris Hodges. All rights reserved.

 ****************************************************************************/

/*
 *----------------------------------------------------------------------------
 *                         Includes for poseidon.library
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 *
 */

#ifndef LIBRARIES_POSEIDON_H
#define LIBRARIES_POSEIDON_H

#include <devices/usbhardware.h>
#include <devices/timer.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/errors.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <utility/pack.h>
#include <libraries/iffparse.h>

/* Types for psdGetAttrs() and psdSetAttrs() */
#define PGA_STACK      0x01
#define PGA_USBCLASS   0x02
#define PGA_HARDWARE   0x03
#define PGA_DEVICE     0x04
#define PGA_CONFIG     0x05
#define PGA_INTERFACE  0x06
#define PGA_ENDPOINT   0x07
#define PGA_ERRORMSG   0x08
#define PGA_PIPE       0x09
#define PGA_APPBINDING 0x0a
#define PGA_EVENTNOTE  0x0b
#define PGA_STACKCFG   0x0c
#define PGA_PIPESTREAM 0x0d
#define PGA_DESCRIPTOR 0x0e
#define PGA_RTISO      0x0f
#define PGA_LAST       0x0f

/* Tags for psdGetAttrs(PGA_STACK,...) */
#define PA_Dummy             (TAG_USER + 2612)
#define PA_ConfigRead        (PA_Dummy + 0x01)
#define PA_HardwareList      (PA_Dummy + 0x20)
#define PA_ClassList         (PA_Dummy + 0x21)
#define PA_ErrorMsgList      (PA_Dummy + 0x22)
#define PA_GlobalConfig      (PA_Dummy + 0x44)
#define PA_CurrConfigHash    (PA_Dummy + 0x45)
#define PA_SavedConfigHash   (PA_Dummy + 0x46)
#define PA_MemPoolUsage      (PA_Dummy + 0x50)
#define PA_ReleaseVersion    (PA_Dummy + 0x60)
#define PA_OSVersion         (PA_Dummy + 0x61)

/* Tags for psdGetAttrs(PGA_ERRORMSG,...) */
#define EMA_Dummy            (TAG_USER  + 103)
#define EMA_Level            (EMA_Dummy + 0x10)
#define EMA_Origin           (EMA_Dummy + 0x11)
#define EMA_Msg              (EMA_Dummy + 0x12)
#define EMA_DateStamp        (EMA_Dummy + 0x13)

/* Tags for psdGetAttrs(PGA_USBCLASS,...) */

#define UCA_Dummy            (TAG_USER  + 4489)
#define UCA_ClassBase        (UCA_Dummy + 0x10)
#define UCA_ClassName        (UCA_Dummy + 0x11)
#define UCA_UseCount         (UCA_Dummy + 0x12)
#define UCA_FullPath         (UCA_Dummy + 0x13)

/* Tags for psdGetAttrs(PGA_HARDWARE,...) */
#define HA_Dummy             (TAG_USER + 0x2612)
#define HA_DeviceName        (HA_Dummy + 0x10)
#define HA_DeviceUnit        (HA_Dummy + 0x11)
#define HA_ProductName       (HA_Dummy + 0x12)
#define HA_Manufacturer      (HA_Dummy + 0x13)
#define HA_Version           (HA_Dummy + 0x14)
#define HA_Revision          (HA_Dummy + 0x15)
#define HA_Description       (HA_Dummy + 0x16)
#define HA_Copyright         (HA_Dummy + 0x17)
#define HA_DriverVersion     (HA_Dummy + 0x18)
#define HA_DeviceList        (HA_Dummy + 0x20)

/* Tags for psdGetAttrs(PGA_DEVICE,...) */
#define DA_Dummy             (TAG_USER + 42)
#define DA_IsLowspeed        (DA_Dummy + 0x01)
#define DA_IsConnected       (DA_Dummy + 0x02)
#define DA_HasAddress        (DA_Dummy + 0x03)
#define DA_HasDevDesc        (DA_Dummy + 0x04)
#define DA_IsConfigured      (DA_Dummy + 0x05)
#define DA_HasAppBinding     (DA_Dummy + 0x06)
#define DA_IsHighspeed       (DA_Dummy + 0x07)
#define DA_IsDead            (DA_Dummy + 0x08)
#define DA_Config            (DA_Dummy + 0x09)
#define DA_IsSuspended       (DA_Dummy + 0x0a)
#define DA_Address           (DA_Dummy + 0x10)
#define DA_NumConfigs        (DA_Dummy + 0x11)
#define DA_CurrConfig        (DA_Dummy + 0x12)
#define DA_HubDevice         (DA_Dummy + 0x13)
#define DA_UsbVersion        (DA_Dummy + 0x14)
#define DA_Class             (DA_Dummy + 0x15)
#define DA_SubClass          (DA_Dummy + 0x16)
#define DA_Protocol          (DA_Dummy + 0x17)
#define DA_Version           (DA_Dummy + 0x18)
#define DA_VendorID          (DA_Dummy + 0x19)
#define DA_ProductID         (DA_Dummy + 0x1a)
#define DA_Manufacturer      (DA_Dummy + 0x1b)
#define DA_ProductName       (DA_Dummy + 0x1c)
#define DA_SerialNumber      (DA_Dummy + 0x1d)
#define DA_Hardware          (DA_Dummy + 0x1e)
#define DA_Binding           (DA_Dummy + 0x1f)
#define DA_ConfigList        (DA_Dummy + 0x20)
#define DA_LangIDArray       (DA_Dummy + 0x21)
#define DA_CurrLangID        (DA_Dummy + 0x22)
#define DA_BindingClass      (DA_Dummy + 0x23)
#define DA_IDString          (DA_Dummy + 0x24)
#define DA_CloneCount        (DA_Dummy + 0x25)
#define DA_AtHubPortNumber   (DA_Dummy + 0x26)
#define DA_NeedsSplitTrans   (DA_Dummy + 0x27)
#define DA_OrigProductName   (DA_Dummy + 0x28)
#define DA_DescriptorList    (DA_Dummy + 0x29)
#define DA_MaxPktSize0       (DA_Dummy + 0x2a)
#define DA_HubThinkTime      (DA_Dummy + 0x2b)
#define DA_PowerSupply       (DA_Dummy + 0x30)
#define DA_PowerDrained      (DA_Dummy + 0x31)
#define DA_LowPower          (DA_Dummy + 0x32)
#define DA_InhibitPopup      (DA_Dummy + 0x40)
#define DA_IsNewToMe         (DA_Dummy + 0x41)
#define DA_InhibitClassBind  (DA_Dummy + 0x42)
#define DA_OverridePowerInfo (DA_Dummy + 0x43)
#ifdef AROS_USB30_CODE
#define DA_IsSuperspeed      (DA_Dummy + 0x44)
#endif

/* Tags for psdGetAttrs(PGA_CONFIG,...) */
#define CA_Dummy             (TAG_USER + 23)
#define CA_Attrs             (CA_Dummy + 0x01)
#define CA_SelfPowered       (CA_Dummy + 0x02)
#define CA_RemoteWakeup      (CA_Dummy + 0x03)
#define CA_ConfigNum         (CA_Dummy + 0x10)
#define CA_MaxPower          (CA_Dummy + 0x11)
#define CA_ConfigName        (CA_Dummy + 0x12)
#define CA_NumInterfaces     (CA_Dummy + 0x13)
#define CA_Device            (CA_Dummy + 0x14)
#define CA_InterfaceList     (CA_Dummy + 0x20)

/* Tags for psdGetAttrs(PGA_DESCRIPTOR,...) */
#define DDA_Dummy            (TAG_USER + 888)
#define DDA_Device           (DDA_Dummy + 0x01)
#define DDA_Config           (DDA_Dummy + 0x02)
#define DDA_Interface        (DDA_Dummy + 0x03)
#define DDA_Endpoint         (DDA_Dummy + 0x04)
#define DDA_Name             (DDA_Dummy + 0x08)
#define DDA_DescriptorType   (DDA_Dummy + 0x10)
#define DDA_DescriptorData   (DDA_Dummy + 0x11)
#define DDA_DescriptorLength (DDA_Dummy + 0x12)
#define DDA_CS_SubType       (DDA_Dummy + 0x20)

/* Tags for psdGetAttrs(PGA_INTERFACE,...) */
#define IFA_Dummy            (TAG_USER + 4711)
#define IFA_InterfaceNum     (IFA_Dummy + 0x10)
#define IFA_AlternateNum     (IFA_Dummy + 0x11)
#define IFA_Class            (IFA_Dummy + 0x12)
#define IFA_SubClass         (IFA_Dummy + 0x13)
#define IFA_Protocol         (IFA_Dummy + 0x14)
#define IFA_InterfaceName    (IFA_Dummy + 0x15)
#define IFA_Config           (IFA_Dummy + 0x16)
#define IFA_Binding          (IFA_Dummy + 0x17)
#define IFA_NumEndpoints     (IFA_Dummy + 0x18)
#define IFA_BindingClass     (IFA_Dummy + 0x19)
#define IFA_IDString         (IFA_Dummy + 0x1a)
#define IFA_EndpointList     (IFA_Dummy + 0x20)
#define IFA_AlternateIfList  (IFA_Dummy + 0x21)

/* Tags for psdGetAttrs(PGA_ENDPOINT,...) */
#define EA_Dummy             (TAG_USER + 1138)
#define EA_IsIn              (EA_Dummy + 0x01)
#define EA_EndpointNum       (EA_Dummy + 0x10)
#define EA_TransferType      (EA_Dummy + 0x11)
#define EA_MaxPktSize        (EA_Dummy + 0x12)
#define EA_Interval          (EA_Dummy + 0x13)
#define EA_Interface         (EA_Dummy + 0x14)
#define EA_NumTransMuFrame   (EA_Dummy + 0x15)
#define EA_SyncType          (EA_Dummy + 0x16)
#define EA_UsageType         (EA_Dummy + 0x17)

/* Tags for psdGetAttrs(PGA_PIPE,...) */
#define PPA_Dummy            (TAG_USER  + 1234)
#define PPA_Endpoint         (PPA_Dummy + 0x01)
#define PPA_Error            (PPA_Dummy + 0x02)
#define PPA_Actual           (PPA_Dummy + 0x03)
#define PPA_EndpointNum      (PPA_Dummy + 0x04)
#define PPA_DeviceAddress    (PPA_Dummy + 0x05)
#define PPA_IORequest        (PPA_Dummy + 0x06)
#define PPA_NoZeroPktTerm    (PPA_Dummy + 0x07)
#define PPA_NoShortPackets   PPA_NoZeroPktTerm /* obsolete, bad naming */
#define PPA_NakTimeout       (PPA_Dummy + 0x08)
#define PPA_NakTimeoutTime   (PPA_Dummy + 0x09)
#define PPA_AllowRuntPackets (PPA_Dummy + 0x0a)
#define PPA_MaxPktSize       (PPA_Dummy + 0x0b)
#define PPA_Interval         (PPA_Dummy + 0x0c)

/* Tags for application binding and psdGetAttrs(PGA_APPBINDING,...)*/
#define ABA_Dummy            (TAG_USER + 666)
#define ABA_ReleaseHook      (ABA_Dummy + 0x01)
#define ABA_Device           (ABA_Dummy + 0x02)
#define ABA_UserData         (ABA_Dummy + 0x03)
#define ABA_Task             (ABA_Dummy + 0x04)
#define ABA_ForceRelease     (ABA_Dummy + 0x10)

/* Tags for psdGetAttrs(PGA_EVENTNOTE,...)*/
#define ENA_Dummy            (TAG_USER + 777)
#define ENA_EventID          (ENA_Dummy + 0x01)
#define ENA_Param1           (ENA_Dummy + 0x02)
#define ENA_Param2           (ENA_Dummy + 0x03)

/* Tags for psdGetAttrs(PGA_GLOBALCFG,...) */
#define GCA_Dummy            (TAG_USER + 0x1138)
#define GCA_LogInfo          (GCA_Dummy + 0x01)
#define GCA_LogWarning       (GCA_Dummy + 0x02)
#define GCA_LogError         (GCA_Dummy + 0x03)
#define GCA_LogFailure       (GCA_Dummy + 0x04)
#define GCA_SubTaskPri       (GCA_Dummy + 0x10)
#define GCA_BootDelay        (GCA_Dummy + 0x11)
#define GCA_PopupDeviceNew   (GCA_Dummy + 0x20)
#define GCA_PopupDeviceGone  (GCA_Dummy + 0x21)
#define GCA_PopupDeviceDeath (GCA_Dummy + 0x22)
#define GCA_PopupCloseDelay  (GCA_Dummy + 0x23)
#define GCA_PopupActivateWin (GCA_Dummy + 0x30)
#define GCA_PopupWinToFront  (GCA_Dummy + 0x31)
#define GCA_InsertionSound   (GCA_Dummy + 0x40)
#define GCA_RemovalSound     (GCA_Dummy + 0x41)
#define GCA_AutoDisableLP    (GCA_Dummy + 0x60)
#define GCA_AutoDisableDead  (GCA_Dummy + 0x61)
#define GCA_AutoRestartDead  (GCA_Dummy + 0x63)
#define GCA_PowerSaving      (GCA_Dummy + 0x64)
#define GCA_ForceSuspend     (GCA_Dummy + 0x65)
#define GCA_SuspendTimeout   (GCA_Dummy + 0x66)
#define GCA_PrefsVersion     (GCA_Dummy + 0x70)

/* Tags for psdGetAttrs(PGA_PIPESTREAM,...) */
#define PSA_Dummy            (TAG_USER + 0x0409)
#define PSA_MessagePort      (PSA_Dummy + 0x01)
#define PSA_AsyncIO          (PSA_Dummy + 0x02)
#define PSA_NumPipes         (PSA_Dummy + 0x03)
#define PSA_BufferSize       (PSA_Dummy + 0x04)
#define PSA_ShortPktTerm     (PSA_Dummy + 0x05)
#define PSA_ReadAhead        (PSA_Dummy + 0x06)
#define PSA_BufferedRead     (PSA_Dummy + 0x07)
#define PSA_BufferedWrite    (PSA_Dummy + 0x08)
#define PSA_NoZeroPktTerm    (PSA_Dummy + 0x09)
#define PSA_NakTimeout       (PSA_Dummy + 0x0a)
#define PSA_NakTimeoutTime   (PSA_Dummy + 0x0b)
#define PSA_AllowRuntPackets (PSA_Dummy + 0x0c)
#define PSA_TermArray        (PSA_Dummy + 0x0d)
#define PSA_DoNotWait        (PSA_Dummy + 0x0e)
#define PSA_AbortSigMask     (PSA_Dummy + 0x0f)
#define PSA_BytesPending     (PSA_Dummy + 0x10)
#define PSA_Error            (PSA_Dummy + 0x11)
#define PSA_ActivePipe       (PSA_Dummy + 0x12)

/* Tags for psdGetAttrs(PGA_RTISO,...) */
#define RTA_Dummy            (TAG_USER + 999)
#define RTA_InRequestHook    (RTA_Dummy + 0x01)
#define RTA_OutRequestHook   (RTA_Dummy + 0x02)
#define RTA_InDoneHook       (RTA_Dummy + 0x03)
#define RTA_OutDoneHook      (RTA_Dummy + 0x04)
#define RTA_ReleaseHook      (RTA_Dummy + 0x05)
#define RTA_OutPrefetchSize  (RTA_Dummy + 0x10)

/* NumToStr types */
#define NTS_IOERR        1
#define NTS_LANGID       2
#define NTS_TRANSTYPE    3
#define NTS_VENDORID     4
#define NTS_CLASSCODE    5
#define NTS_DESCRIPTOR   6
#define NTS_SYNCTYPE     7
#define NTS_USAGETYPE    8
#define NTS_COMBOCLASS   9

/* NTS_COMBOCLASS flags */
#define NTSCCS_CLASS      0      /* Class field is bits 0-7 */
#define NTSCCS_SUBCLASS   8      /* Subclass field is bits 8-15 */
#define NTSCCS_PROTO     16      /* Protocol field is bits 16-24 */
#define NTSCCF_CLASS     (1<<24) /* Class field is valid */
#define NTSCCF_SUBCLASS  (1<<25) /* Subclass field is valid */
#define NTSCCF_PROTO     (1<<26) /* Protocol field is valid */

/* Event Handler stuff */
#define EHMB_ADDHARDWARE  0x01 /* Param1 = phw */
#define EHMB_REMHARDWARE  0x02 /* Param1 = phw */
#define EHMB_ADDDEVICE    0x03 /* Param1 = pd */
#define EHMB_REMDEVICE    0x04 /* Param1 = pd */
#define EHMB_ADDCLASS     0x05 /* Param1 = puc */
#define EHMB_REMCLASS     0x06 /* Param1 = puc */
#define EHMB_ADDBINDING   0x07 /* Param1 = pd */
#define EHMB_REMBINDING   0x08 /* Param1 = pd */
#define EHMB_ADDERRORMSG  0x09 /* Param1 = pem */
#define EHMB_REMERRORMSG  0x0a /* Param1 = pem */
#define EHMB_CONFIGCHG    0x0b /* Param1 = void */
#define EHMB_DEVICEDEAD   0x0c /* Param1 = pd */
#define EHMB_DEVICELOWPW  0x0d /* Param1 = pd */
#define EHMB_DEVSUSPENDED 0x0e /* Param1 = pd */
#define EHMB_DEVRESUMED   0x0f /* Param1 = pd */

#define EHMF_ADDHARDWARE  (1L<<EHMB_ADDHARDWARE)
#define EHMF_REMHARDWARE  (1L<<EHMB_REMHARDWARE)
#define EHMF_ADDDEVICE    (1L<<EHMB_ADDDEVICE)
#define EHMF_REMDEVICE    (1L<<EHMB_REMDEVICE)
#define EHMF_ADDCLASS     (1L<<EHMB_ADDCLASS)
#define EHMF_REMCLASS     (1L<<EHMB_REMCLASS)
#define EHMF_ADDBINDING   (1L<<EHMB_ADDBINDING)
#define EHMF_REMBINDING   (1L<<EHMB_REMBINDING)
#define EHMF_ADDERRORMSG  (1L<<EHMB_ADDERRORMSG)
#define EHMF_REMERRORMSG  (1L<<EHMB_REMERRORMSG)
#define EHMF_CONFIGCHG    (1L<<EHMB_CONFIGCHG)
#define EHMF_DEVICEDEAD   (1L<<EHMB_DEVICEDEAD)
#define EHMF_DEVICELOWPW  (1L<<EHMB_DEVICELOWPW)
#define EHMF_DEVSUSPENDED (1L<<EHMB_DEVSUSPENDED)
#define EHMF_DEVRESUMED   (1L<<EHMB_DEVRESUMED)

/* Configuration stuff */

/* How this works:
   Configurations are stored in an IFF structure. It is managed by Trident
   and the stack itself. Class drivers may only inject, delete or read out
   certain sections of the configuration data. They either get pointer to the
   FORM sections or return pointers to this FORM. The form data has to be
   copied in each case (the length is given in the form).

   The configuration normally looks like this:
   FORM PSDC (Poseidon Config file)

[1*] FORM STKC (Stack config)
[n*]   FORM UDEV (USB hardware device)
         CHNK NAME [varlen] (device name)
         CHNK UNIT [4 bytes]
         [...]
[n*]   FORM UCLS (USB class driver)
         CHNK NAME [varlen] (library name)
         [...]
       [Chunks/Forms] (chunks containing some more data for the main stack file)

[n*] FORM CLSC (Class config)
       CHNK OWNR [varlen] name of class which stores this data
       FORM GCPD (global class private data) <- that's the form the class can modify
         [...]

[n*] FORM DEVC (Device configurations)
       CHNK DVID [varlen] DeviceID-String
       CHNK FBND [varlen] <classname> (forced binding)
       CHNK NAME [varlen] Custom name
[n*]   FORM DCFG (Device configuration data)
         CHNK OWNR [varlen] name of class for binding (AppName?)
         FORM DCPD (device config private data) <- that's the form the class can modify
           [...]
[n*]   FORM ICFG (Interface configuration data)
         CHNK OWNR [varlen] name of class for binding
         CHNK IFID [varlen] InterfaceID-String
         CHNK FBND [varlen] <classname> (forced binding)
         FORM ICPD (interface config private data) <- that's the form the class can modify

    psdReadCfg(pic, formdata):
        pic == NULL; replace (!) root form with given formdata
        pic != NULL; replace given pic with new form data.
    psdWriteCfg(pic)
        pic == NULL; generate the whole configuration buffer to save
        pic != NULL; generate the form buffer of the pic and its subform.
    psdFindCfgForm(pic, formid)
        pic == NULL -> pic = root form;
        find and return pic with given form ID or NULL if no such form exists.
    psdNextCfgForm(pic)
        get next form pic with same ID or return NULL if this was the only form with this ID.

    psdRemCfgForm(pic)
        pic == NULL -> pic = root form;
        delete form pic (effectly calls pFreeForm()).

    psdAddCfgChunk(pic, formdata)
        pic == NULL -> pic = root form;
        add the form OR CHUNK to the pic context form. Do not replace existing forms.
        Replaces existing chunks.

    psdRemCfgChunk(pic, chnkid)
        pic == NULL -> pic = root form;
        delete the chunk from the pic context form.

*/

#if AROS_BIG_ENDIAN
/* While Poseidon writes a correct big endian IFF structure on both big and little endian
 * the contents are *not* endian agnostic. Therefore we will avoid loading prefs of
 * different endianess by defining the outmost FORM ID endian specific. Also notice that
 * compatibility of the config files with 68k and MorphOS versions is not given. */
#define IFFFORM_PSDCFG     MAKE_ID('P','S','B','C')
#else
#define IFFFORM_PSDCFG     MAKE_ID('P','S','L','C')
#endif
#define IFFFORM_STACKCFG   MAKE_ID('S','T','K','C')
#define IFFFORM_DEVICECFG  MAKE_ID('D','E','V','C')
#define IFFFORM_CLASSCFG   MAKE_ID('C','L','S','C')
#define IFFFORM_UHWDEVICE  MAKE_ID('U','H','W','D')
#define IFFFORM_USBCLASS   MAKE_ID('U','C','L','S')
#define IFFFORM_CLASSDATA  MAKE_ID('G','C','P','D')
#define IFFFORM_DEVCFGDATA MAKE_ID('D','C','F','G')
#define IFFFORM_DEVCLSDATA MAKE_ID('D','C','P','D')
#define IFFFORM_IFCFGDATA  MAKE_ID('I','C','F','G')
#define IFFFORM_IFCLSDATA  MAKE_ID('I','C','P','D')

#define IFFCHNK_OWNER      MAKE_ID('O','W','N','R')
#define IFFCHNK_NAME       MAKE_ID('N','A','M','E')
#define IFFCHNK_UNIT       MAKE_ID('U','N','I','T')
#define IFFCHNK_OFFLINE    MAKE_ID('O','F','F','L')
#define IFFCHNK_GLOBALCFG  MAKE_ID('G','C','F','G')
#define IFFCHNK_DEVID      MAKE_ID('D','V','I','D')
#define IFFCHNK_IFID       MAKE_ID('I','F','I','D')
#define IFFCHNK_FORCEDBIND MAKE_ID('F','B','N','D')
#define IFFCHNK_POPUP      MAKE_ID('P','O','P','O')
#define IFFCHNK_INSERTSND  MAKE_ID('I','N','S','F')
#define IFFCHNK_REMOVESND  MAKE_ID('R','M','S','F')

/* Private stuff starts here */

#if defined(__GNUC__)
# pragma pack(2)
#endif

/* GCA_PopupDeviceNew definitions */

#define PGCP_NEVER      0 /* never open a pop-up window */
#define PGCP_ERROR      1 /* popup, on error condition (e.g. low power) */
#define PGCP_ISNEW      2 /* popup, if this is the first time the device is connected */
#define PGCP_NOBINDING  3 /* popup, if there is no binding */
#define PGCP_ASKCONFIG  4 /* popup and ask to configure, if not existent */
#define PGCP_CANCONFIG  5 /* popup and ask to configure, if possible */
#define PGCP_HASBINDING 6 /* popup, if there is a binding to a class */
#define PGCP_ALWAYS     7 /* popup always */

struct PsdGlobalCfg
{
    ULONG pgc_ChunkID;                    /* ChunkID=IFFCHNK_GLOBALCFG */
    ULONG pgc_Length;                     /* sizeof(struct PsdGlobalCfg)-8 */
    BOOL  pgc_LogInfo;                    /* Log normal messages */
    BOOL  pgc_LogWarning;                 /* Log warnings */
    BOOL  pgc_LogError;                   /* Log errors */
    BOOL  pgc_LogFailure;                 /* Log failures */
    ULONG pgc_BootDelay;                  /* boot delay */
    WORD  pgc_SubTaskPri;                 /* Subtask priority */
    UWORD pgc_PopupDeviceNew;             /* New device popup */
    BOOL  pgc_PopupDeviceGone;            /* Device removed popup */
    BOOL  pgc_PopupDeviceDeath;           /* Device dead popup */
    ULONG pgc_PopupCloseDelay;            /* Delay in seconds before closing */
    BOOL  pgc_PopupActivateWin;           /* Activate window on opening */
    BOOL  pgc_PopupWinToFront;            /* Pop window to front on content change */
    BOOL  pgc_AutoDisableLP;              /* Automatically disable on LowPower */
    BOOL  pgc_AutoDisableDead;            /* Automatically disable on Dead */
    BOOL  pgc_AutoRestartDead;            /* Automatically restart on Dead */
    ULONG pgc_PrefsVersion;               /* Reference version of prefs saved */
    BOOL  pgc_PowerSaving;                /* Enable power saving features */
    BOOL  pgc_ForceSuspend;               /* Force Suspend on classes not supporting it, but with remote wakeup */
    ULONG pgc_SuspendTimeout;             /* Timeout when to suspend a device after inactivity */
};

/* DA_OverridePowerInfo definitions */
#define POCP_TRUST_DEVICE 0
#define POCP_BUS_POWERED  1
#define POCP_SELF_POWERED 2

struct PsdPoPoCfg
{
    ULONG poc_ChunkID;                    /* ChunkID=IFFCHNK_POPO */
    ULONG poc_Length;                     /* sizeof(struct PsdPopoCfg)-8 */
    BOOL  poc_InhibitPopup;               /* Inhibit opening of popup window */
    BOOL  poc_NoClassBind;                /* Inhibit class scan */
    UWORD poc_OverridePowerInfo;          /* 0=keep, 1=buspowered, 2=selfpowered */
};

#if defined(__GNUC__)
# pragma pack()
#endif

struct PsdLockSem
{
    struct Node	        pls_Node;         /* Linkage */
    BOOL                pls_Dead;         /* Has Semaphore been deactivated? */
    struct List         pls_WaitQueue;    /* List of waiting tasks (ReadLock structs) */
    struct List         pls_ReadLocks;    /* List of obtained shared locks */
    struct Task        *pls_Owner;        /* Current owner of exclusive lock */
    UWORD               pls_ExclLockCount; /* Exclusive lock count */
    UWORD               pls_SharedLockCount; /* Count of *different* shared lock owners */
};

struct PsdReadLock
{
    struct Node         prl_Node;         /* Linkage */
    BOOL                prl_IsExcl;       /* Is this lock exclusive? */
    struct Task        *prl_Task;         /* Task waiting for or obtaining this lock */
    UWORD               prl_Count;        /* Shared lock count */
};

struct PsdSemaInfo
{
    struct Node         psi_Node;         /* Linkage */
    struct PsdLockSem  *psi_LockSem;      /* Pointer to semaphore */
};

struct PsdBorrowLock
{
    struct Node         pbl_Node;         /* Linkage */
    UWORD               pbl_ExclLockCount; /* Was exclusive before */
    UWORD               pbl_Count;        /* Shared lock count */
    struct PsdLockSem  *pbl_LockSem;      /* Pointer to semaphore */
    struct PsdReadLock *pbl_ReadLock;     /* Readlock that was changed */
};

struct PsdPoPo
{
    struct Task        *po_Task;          /* PoPo Task */
    struct MsgPort     *po_MsgPort;       /* Port for EventNote messages */
    LONG                po_ReadySignal;   /* Signal to respond to task changes */
    struct Task        *po_ReadySigTask;  /* task to signal */
    struct Library     *po_IntBase;       /* Intuition base for PoPo Task */
    struct Library     *po_MUIBase;       /* MUI base for PoPo Task */
    struct Library     *po_DTBase;        /* DataTypes base for PoPo Task */
    struct MsgPort     *po_TimerMsgPort;  /* Standard timer MsgPort */
    struct timerequest *po_TimerIOReq;    /* Standard timer request */
    struct MUI_CustomClass *po_PoPoClass; /* PoPo Action Class */
    ULONG              *po_PoPoObj;       /* PoPo Action Object */
    ULONG              *po_AppObj;        /* App Object */
    ULONG              *po_WindowObj;     /* Window Object */
    ULONG              *po_GroupObj;      /* Group Object */
    ULONG              *po_SaveObj;       /* Save Button Object */
    ULONG              *po_CloseObj;      /* Close Button Object */
    ULONG              *po_StickyObj;     /* Sticky Object */
    ULONG              *po_AboutMI;       /* About MenuItem */
    ULONG              *po_CloseMI;       /* Close MenuItem */
    ULONG              *po_TridentMI;     /* Trident MenuItem */
    ULONG              *po_MUIPrefsMI;    /* MUI Prefs MenuItem */
    struct PsdEventHook *po_EventHandler; /* Event handler */
    struct List         po_GadgetList;    /* List of gadgets for a device */
    BOOL                po_OpenRequest;   /* open window requested */
    BOOL                po_Sticky;        /* sticky entries */
    STRPTR              po_InsertSndFile; /* Path to insertion sound file */
    STRPTR              po_RemoveSndFile; /* Path to removal sound file */
    struct List         po_Sounds;        /* List of loaded soundfiles */
};

struct PsdHandlerTask
{
    struct Task        *ph_Task;          /* Event Handler Task */
    struct MsgPort     *ph_MsgPort;       /* Port for EventNote messages */
    LONG                ph_ReadySignal;   /* Signal to respond to task changes */
    struct Task        *ph_ReadySigTask;  /* task to signal */
    struct PsdEventHook *ph_EventHandler; /* Event handler */
    struct MsgPort     *ph_TimerMsgPort;  /* Port for timer requests */
    struct timerequest *ph_TimerIOReq;    /* Standard timer request */
};

struct PsdWStringMap
{
    WORD   psm_ID;
    STRPTR psm_String;
};

struct PsdUWStringMap
{
    UWORD  psm_ID;
    STRPTR psm_String;
};

struct PsdULStringMap
{
    ULONG  psm_ID;
    STRPTR psm_String;
};

/* The library node - private
*/
struct PsdBase
{
    struct Library      ps_Library;       /* standard */
    UWORD               ps_Flags;         /* various flags */
    struct UtilityBase *ps_UtilityBase;   /* for tags etc */
    struct Library     *ps_DosBase;       /* for dos stuff */
    BOOL                ps_StackInit;     /* Did we initialize the stack yet? */
    APTR                ps_MemPool;       /* Public Memory Pool */
    APTR                ps_SemaMemPool;   /* Memory Pool exclusively for Semaphore ReadLocks */
    struct List         ps_DeadlockDebug; /* linked list of semaphore allocations */
    struct PsdLockSem   ps_Lock;          /* PBase lock */
    struct PsdLockSem   ps_ConfigLock;    /* Config semaphore */
    struct timerequest  ps_TimerIOReq;    /* Standard timer request */
    struct List         ps_Hardware;      /* List of Hardware Interfaces in use */
    struct List         ps_Classes;       /* List of Classes loaded */
    struct List         ps_ErrorMsgs;     /* List of Error Msgs */
    struct List         ps_EventHooks;    /* List of EventHandlers */
    struct MsgPort      ps_EventReplyPort; /* Replyport for Events */
    struct List         ps_ConfigRoot;    /* Configuration FORMs */
    struct List         ps_AlienConfigs;  /* Configuration FORM from outer space */
    BOOL                ps_CfgChangeMute; /* Don't generate config changed events */
    struct SignalSemaphore ps_ReentrantLock; /* Lock for non-reentrant stuff */
    struct SignalSemaphore ps_PoPoLock;   /* Lock for non-reentrant stuff */
    ULONG               ps_MemAllocated;  /* Bytes of memory allocated by stack */
    UWORD               ps_FunnyCount;    /* Funny Message Counter */
    BOOL                ps_ConfigRead;    /* Has a config been loaded? */
    BOOL                ps_CheckConfigReq; /* Set to true, to check if config changed */
    ULONG               ps_ConfigHash;    /* Last config hash value */
    ULONG               ps_SavedConfigHash; /* Hash sum of last saved config */
    struct PsdGlobalCfg *ps_GlobalCfg;    /* Global Config structure */
    struct PsdPoPo      ps_PoPo;
    ULONG               ps_ReleaseVersion; /* Release Version for update info */
    ULONG               ps_OSVersion;     /* Internal OS Version descriptor */
    BOOL                ps_StartedAsTask; /* Did we start in Task Mode before DOS was available? */
    struct PsdHandlerTask ps_EventHandler; /* Event handler */
};

/* ps_Flags */
#define PSF_KLOG 0x0001

struct PsdEventHook
{
    struct Node         peh_Node;         /* Node linkage */
    struct MsgPort     *peh_MsgPort;      /* Target message port */
    ULONG               peh_MsgMask;      /* Mask of messages to send */
};

struct PsdEventNote
{
    struct Message      pen_Msg;          /* Intertask communication message */
    UWORD               pen_Event;        /* Event number as specified above */
    APTR                pen_Param1;       /* Parameter 1 for event */
    APTR                pen_Param2;       /* Parameter 2 */
};

struct PsdEventNoteInternal
{
    struct Node         peni_Node;        /* Node linkage */
    struct PsdEventNote peni_EventNote;   /* Encapsulated PsdEventNote */
};

struct PsdErrorMsg
{
    struct Node         pem_Node;         /* Node linkage */
    struct PsdBase     *pem_Base;         /* Uplinking */
    UWORD               pem_Level;        /* RC: 0=Note, 5=Warn, 10=Error, 20=Fail */
    STRPTR              pem_Origin;       /* From whom? */
    STRPTR              pem_Msg;          /* Actual error message */
    struct DateStamp    pem_DateStamp;    /* Date Stamp (if DOS available) */
};

struct PsdIFFContext
{
    struct Node         pic_Node;         /* Node linkage */
    //struct PsdIFFContext *pic_Parent;     /* Uplinking */
    struct List         pic_SubForms;     /* All sub forms */
    ULONG               pic_FormID;       /* 4 bytes FORM ID */
    ULONG               pic_FormLength;   /* Length of form */
    ULONG              *pic_Chunks;       /* Chunks (no forms) */
    ULONG               pic_ChunksLen;    /* Total length of chunks */
    ULONG               pic_BufferLen;    /* size of buffer allocated */
};

struct PsdUsbClass
{
    struct Node         puc_Node;         /* Node linkage */
    struct PsdBase     *puc_Base;         /* Uplinking */
    struct Library     *puc_ClassBase;    /* Library pointer */
    STRPTR              puc_ClassName;    /* Name of class */
    STRPTR              puc_FullPath;     /* Full path and class name */
    UWORD               puc_UseCnt;       /* Number of bindings in use */
    BOOL                puc_RemoveMe;     /* Class scheduled for removal */
};

struct PsdAppBinding
{
    struct Node         pab_Node;         /* Node linkage */
    struct PsdDevice   *pab_Device;       /* Uplinking */
    struct Hook        *pab_ReleaseHook;  /* CallBackHook for releasing binding */
    IPTR                pab_UserData;     /* User Data */
    struct Task        *pab_Task;         /* Task bound to */
    BOOL                pab_ForceRelease; /* Force release of other app or class bindings */
};

struct PsdHardware
{
    struct Node         phw_Node;         /* Node linkage */
    struct PsdBase     *phw_Base;         /* Uplinking */
    struct Task        *phw_ReadySigTask; /* Task to send ready signal to */
    LONG                phw_ReadySignal;  /* Signal to send when ready */
    struct Task        *phw_Task;         /* Device task */
    STRPTR              phw_DevName;      /* Device name */
    ULONG               phw_Unit;         /* Unit number */

    STRPTR              phw_ProductName;  /* Product name */
    STRPTR              phw_Manufacturer; /* Manufacturer name */
    STRPTR              phw_Description;  /* Description string */
    STRPTR              phw_Copyright;    /* Copyright string */
    UWORD               phw_Version;      /* Version of device */
    UWORD               phw_Revision;     /* Device revision */
    UWORD               phw_DriverVers;   /* Driver version */
    ULONG               phw_Capabilities; /* Driver/HW capabilities */

    struct IOUsbHWReq  *phw_RootIOReq;    /* First IO Request */

    struct PsdDevice   *phw_RootDevice;   /* Link to root hub of this hardware */
    struct PsdDevice   *phw_DevArray[128]; /* DevAddress->Device mapping */
    struct List         phw_Devices;      /* List of devices */
    struct List         phw_DeadDevices;  /* List of disconnected devices */
    BOOL                phw_RemoveMe;     /* Hardware scheduled for removal */
    struct MsgPort      phw_DevMsgPort;   /* Quick device message port */
    struct MsgPort      phw_TaskMsgPort;  /* Quick task message port */
    volatile ULONG      phw_MsgCount;     /* Number of Messages pending */
};

/* Flags for pd_Flags */

#define PDFF_LOWSPEED    0x0001
#define PDFF_CONNECTED   0x0002
#define PDFF_HASDEVADDR  0x0004
#define PDFF_HASDEVDESC  0x0008
#define PDFF_CONFIGURED  0x0010
#define PDFF_HIGHSPEED   0x0020
#define PDFF_NEEDSSPLIT  0x0040
#define PDFF_LOWPOWER    0x0080
#define PDFF_DEAD        0x0100
#define PDFF_SUSPENDED   0x0200
#ifdef AROS_USB30_CODE
#define PDFF_SUPERSPEED  0x0400
#endif
#define PDFF_APPBINDING  0x4000
#define PDFF_DELEXPUNGE  0x8000

struct PsdDevice
{
    struct Node         pd_Node;          /* Node linkage */
    struct PsdHardware *pd_Hardware;      /* Interfacing hardware */
    struct PsdLockSem   pd_Lock;          /* Access locking */
    struct PsdDevice   *pd_Hub;           /* Hub for device */
    APTR                pd_DevBinding;    /* Device binding */
    struct PsdUsbClass *pd_ClsBinding;    /* Which class has the bond? */
    struct PsdConfig   *pd_CurrentConfig; /* Direct pointer to currently set config */
    UWORD               pd_UseCnt;        /* Usage counter */
    UWORD               pd_DevAddr;       /* Device address */
    UWORD               pd_CurrCfg;       /* Current Configuration Number */
    UWORD               pd_NumCfgs;       /* Number of configurations available */
    UWORD               pd_PowerDrain;    /* Current power usage */
    UWORD               pd_PowerSupply;   /* Power provided from parent */
    UWORD               pd_CurrLangID;    /* Current Language ID */
    UWORD              *pd_LangIDArray;   /* Array of supported languages */
    UWORD               pd_Flags;         /* Lowspeed? */
    UWORD               pd_HubPort;       /* Port number at parent hub */
    UWORD               pd_HubThinkTime;  /* Think time for TT inter-transaction gap */
    UWORD               pd_USBVers;       /* USB Version */
    UWORD               pd_DevClass;      /* Class code */
    UWORD               pd_DevSubClass;   /* Subclass code */
    UWORD               pd_DevProto;      /* Device protocol code */
    UWORD               pd_MaxPktSize0;   /* Packet size for EP0 */
    UWORD               pd_VendorID;      /* Vendor ID */
    UWORD               pd_ProductID;     /* Product ID */
    UWORD               pd_DevVers;       /* Device release version */
    UWORD               pd_CloneCount;    /* Running Number to distinguish same devices */
    UWORD               pd_DeadCount;     /* Number of timeouts on the device */
    UWORD               pd_IOBusyCount;   /* Number of busy IOs (not including interrupt transfers) */
    struct timeval      pd_LastActivity;  /* Timestamp of last IO access (start or end) */
    STRPTR              pd_MnfctrStr;     /* Manufacturer string */
    STRPTR              pd_ProductStr;    /* Product string (custom?) */
    STRPTR              pd_OldProductStr; /* Original product string */
    STRPTR              pd_SerNumStr;     /* Serial number string */
    STRPTR              pd_IDString;      /* Whole Device ID string */
    struct List         pd_Configs;       /* List of configurations */
    BOOL                pd_IsNewToMe;     /* Whether the device is connected the first time */
    struct PsdPoPoCfg   pd_PoPoCfg;       /* Inhibit PopUp and Class scan Config */
    struct List         pd_Descriptors;   /* Descriptors collected */
    struct List         pd_RTIsoHandlers; /* List of RTIsoHandlers */
};

struct PsdDescriptor
{
    struct Node         pdd_Node;         /* Node linkage */
    struct PsdDevice   *pdd_Device;       /* Up linkage */
    struct PsdConfig   *pdd_Config;       /* Up linkage (optional, depending on type of desc) */
    struct PsdInterface *pdd_Interface;   /* Up linkage (optional, depending on type of desc) */
    struct PsdEndpoint *pdd_Endpoint;     /* Up linkage (optional, depending on type of desc) */
    STRPTR              pdd_Name;         /* Supposed Descriptor Type Name */
    UWORD               pdd_Type;         /* Descriptor Type */
    UWORD               pdd_Length;       /* Number of bytes in descriptor */
    UWORD               pdd_CSSubType;    /* Subtype of ClassSpecific Descriptor */
    UBYTE              *pdd_Data;         /* Pointer to data */
};

struct PsdConfig
{
    struct Node         pc_Node;          /* Node linkage */
    struct PsdDevice   *pc_Device;        /* Up linkage */
    UWORD               pc_CfgNum;        /* Config number */
    UWORD               pc_Attr;          /* Attributes */
    UWORD               pc_MaxPower;      /* MaxPower (in mA) */
    UWORD               pc_NumIfs;        /* Number of interfaces */
    STRPTR              pc_CfgStr;        /* Name of config */
    struct List         pc_Interfaces;    /* List of interfaces */
};

struct PsdInterface
{
    struct Node         pif_Node;         /* Node linkage */
    struct PsdConfig   *pif_Config;       /* Up linkage */
    APTR                pif_IfBinding;    /* Interface Binding */
    struct PsdUsbClass *pif_ClsBinding;   /* Which class has the bond? */
    UWORD               pif_IfNum;        /* Interface number */
    UWORD               pif_Alternate;    /* Alternate setting */
    UWORD               pif_NumEPs;       /* Number of Endpoints */
    UWORD               pif_IfClass;      /* Interface Class */
    UWORD               pif_IfSubClass;   /* Interface Subclass */
    UWORD               pif_IfProto;      /* Interface Protocol */
    STRPTR              pif_IfStr;        /* Interface String */
    STRPTR              pif_IDString;     /* Interface ID String */
    struct PsdInterface *pif_ParentIf;    /* If interface is an alternate, uplink */
    struct List         pif_AlterIfs;     /* List of alternate interface not in use */
    struct List         pif_EPs;          /* List of endpoints */
};

struct PsdEndpoint
{
    struct Node         pep_Node;         /* Node linkage */
    struct PsdInterface *pep_Interface;   /* Up linkage */
    UWORD               pep_EPNum;        /* Endpoint address */
    UWORD               pep_Direction;    /* Direction (0=OUT, 1=IN) */
    UWORD               pep_TransType;    /* TransferType, see USEA-Flags */
    UWORD               pep_MaxPktSize;   /* Maximum packet size for EP */
    UWORD               pep_NumTransMuFr; /* Number of transactions per µFrame */
    UWORD               pep_Interval;     /* Interval for polling in ms */
    UWORD               pep_SyncType;     /* Iso Synchronization Type */
    UWORD               pep_UsageType;    /* Iso Usage Type */
};

/* Flags for pp_Flags */
#define PFF_INPLACE     0x0001            /* streams: buffer is in place, needs no copying */

struct PsdPipe
{
    struct Message      pp_Msg;           /* Intertask communication message */
    struct PsdDevice   *pp_Device;        /* Up linkage */
    struct PsdEndpoint *pp_Endpoint;      /* Endpoint linkage or NULL for default pipe */
    struct MsgPort     *pp_MsgPort;       /* Msg Port of task allocated pipe */
    struct PsdPipe     *pp_AbortPipe;     /* Pipe to abort */
    ULONG               pp_Num;           /* internal pipe number (used for streams) */
    UWORD               pp_Flags;         /* internal flags (used for streams) */
    struct IOUsbHWReq   pp_IOReq;         /* IO Request allocated for this pipe */
};

/* Flags for pps_Flags */
#define PSFF_ASYNCIO     0x0001           /* async task (r/w) */
#define PSFF_SHORTTERM   0x0002           /* terminate read on short packet (r) */
#define PSFF_READAHEAD   0x0004           /* read from usb device until buffer full (r) */
#define PSFF_BUFFERREAD  0x0008           /* reads smaller than maxpktsize are buffered (r) */
#define PSFF_BUFFERWRITE 0x0010           /* writes smaller than maxpktsize are buffered -- flush required (w) */
#define PSFF_NOSHORTPKT  0x0020           /* don't terminate writes with a short packet (w) */
#define PSFF_NAKTIMEOUT  0x0040           /* enable nak timeout (r/w) */
#define PSFF_ALLOWRUNT   0x0080           /* allow reading of runt packets (r) */
#define PSFF_DONOTWAIT   0x0100           /* non blocking IO (r) */
#define PSFF_OWNMSGPORT  0x8000           /* internal flag */

struct PsdPipeStream
{
    struct Node         pps_Node;         /* Node linkage */
    struct PsdDevice   *pps_Device;       /* Up linkage */
    struct PsdEndpoint *pps_Endpoint;     /* Endpoint linkage */
    struct MsgPort     *pps_MsgPort;      /* Msg Port of task allocated pipe */
    struct PsdPipe    **pps_Pipes;        /* Array of pipes */
    struct List         pps_FreePipes;    /* Inactive pipes */
    struct List         pps_ReadyPipes;   /* Ready pipes */
    struct Task        *pps_AsyncTask;    /* Task used for asynchroneous transfers */
    struct SignalSemaphore pps_AccessLock; /* Semaphore for reading & writing */
    struct PsdPipe     *pps_ActivePipe;   /* Pipe currently active (w) */
    ULONG               pps_NumPipes;     /* Number of pipes */
    ULONG               pps_NakTimeoutTime; /* Nak Timeout time for pipe */
    ULONG               pps_AbortSigMask; /* Signal mask for waking up read or write */
    UWORD               pps_Flags;        /* transfer flags */

    UBYTE              *pps_Buffer;       /* globally allocated buffer for all pipes */
    ULONG               pps_BufferSize;   /* Size of each pipe buffer */
    ULONG               pps_Offset;       /* read or write offset into the first returned buffer */
    ULONG               pps_BytesPending; /* bytes in read or write buffer */
    ULONG               pps_ReqBytes;     /* number of bytes currently requested */
    LONG                pps_Error;        /* last error occurred */
    UBYTE              *pps_TermArray;    /* termination char array */
};

struct PsdRTIsoHandler
{
    struct Node         prt_Node;         /* Node linkage */
    struct PsdDevice   *prt_Device;       /* Up linkage */
    struct PsdEndpoint *prt_Endpoint;     /* Endpoint linkage */
    struct PsdPipe     *prt_Pipe;         /* Pipe */
    struct Hook        *prt_ReleaseHook;  /* Hook to be called when device gets removed */
    struct IOUsbHWRTIso prt_RTIso;        /* RT Iso structure */
};

#endif /* LIBRARIES_POSEIDON_H */
