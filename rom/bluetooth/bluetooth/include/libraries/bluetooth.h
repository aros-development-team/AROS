/*
 *----------------------------------------------------------------------------
 *                     Includes for bluetooth.library
 *----------------------------------------------------------------------------
 *
 * bluetooth.library is the Bluetooth host stack for AROS. It is modelled on
 * poseidon.library: radios are exec devices ("bluetoothhci.device" style
 * units, see <devices/bluetoothhci.h>) added with btAddHardware(); remote
 * devices, their services and endpoints are objects queried with
 * btGetAttrs(); I/O goes through channels (btAllocChannel/btSendChannel/btWaitChannel);
 * profile drivers are class libraries in SYS:Classes/Bluetooth (see
 * <libraries/btclass.h>); events are delivered to message ports registered
 * with btAddEventHandler(); the configuration is an IFF file handled by the
 * bt*Cfg*() functions.
 */

#ifndef LIBRARIES_BLUETOOTH_H
#define LIBRARIES_BLUETOOTH_H

#include <devices/bluetoothhci.h>
#include <devices/timer.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/errors.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <utility/pack.h>
#include <libraries/iffparse.h>

/* Types for btGetAttrs() and btSetAttrs() */
#define BGA_STACK      0x01
#define BGA_BTCLASS    0x02
#define BGA_HARDWARE   0x03
#define BGA_DEVICE     0x04
#define BGA_SERVICE    0x05
#define BGA_ENDPOINT   0x06
#define BGA_ERRORMSG   0x07
#define BGA_CHANNEL       0x08
#define BGA_APPBINDING 0x09
#define BGA_EVENTNOTE  0x0a
#define BGA_STACKCFG   0x0b
#define BGA_LAST       0x0b

/* Tag bases: one block of 256 per object type */
#define BT_TAGBASE           (TAG_USER + 0xB100)

/* Tags for btGetAttrs(BGA_STACK,...) */
#define BSA_Dummy            (BT_TAGBASE + 0x000)
#define BSA_ConfigRead       (BSA_Dummy + 0x01) /* BOOL: has a config been loaded? */
#define BSA_HardwareList     (BSA_Dummy + 0x20) /* struct List * of BtHardware (lock the library base first!) */
#define BSA_ClassList        (BSA_Dummy + 0x21) /* struct List * of BtClass */
#define BSA_ErrorMsgList     (BSA_Dummy + 0x22) /* struct List * of BtErrorMsg */
#define BSA_GlobalConfig     (BSA_Dummy + 0x44) /* struct BtGlobalCfg * */
#define BSA_CurrConfigHash   (BSA_Dummy + 0x45) /* ULONG */
#define BSA_SavedConfigHash  (BSA_Dummy + 0x46) /* ULONG */
#define BSA_MemPoolUsage     (BSA_Dummy + 0x50) /* ULONG bytes allocated */
#define BSA_ReleaseVersion   (BSA_Dummy + 0x60) /* ULONG */
#define BSA_OSVersion        (BSA_Dummy + 0x61) /* ULONG */

/* Tags for btGetAttrs(BGA_BTCLASS,...) */
#define BCA_Dummy            (BT_TAGBASE + 0x100)
#define BCA_ClassBase        (BCA_Dummy + 0x10) /* struct Library * */
#define BCA_ClassName        (BCA_Dummy + 0x11) /* STRPTR */
#define BCA_UseCount         (BCA_Dummy + 0x12) /* ULONG bindings held */
#define BCA_FullPath         (BCA_Dummy + 0x13) /* STRPTR */

/* Tags for btGetAttrs(BGA_HARDWARE,...) */
#define BHA_Dummy            (BT_TAGBASE + 0x200)
#define BHA_DeviceName       (BHA_Dummy + 0x10) /* STRPTR exec device name */
#define BHA_DeviceUnit       (BHA_Dummy + 0x11) /* ULONG */
#define BHA_ProductName      (BHA_Dummy + 0x12) /* STRPTR (from BTCMD_QUERYDEVICE) */
#define BHA_Manufacturer     (BHA_Dummy + 0x13) /* STRPTR */
#define BHA_Version          (BHA_Dummy + 0x14) /* ULONG */
#define BHA_Revision         (BHA_Dummy + 0x15) /* ULONG */
#define BHA_Description      (BHA_Dummy + 0x16) /* STRPTR */
#define BHA_Copyright        (BHA_Dummy + 0x17) /* STRPTR */
#define BHA_DriverVersion    (BHA_Dummy + 0x18) /* ULONG */
#define BHA_DeviceList       (BHA_Dummy + 0x20) /* struct List * of BtDevice (lock the library base first!) */
#define BHA_NumDevices       (BHA_Dummy + 0x21) /* ULONG */
#define BHA_State            (BHA_Dummy + 0x30) /* ULONG BHS_xxx */
#define BHA_IsReady          (BHA_Dummy + 0x31) /* BOOL: HCI bring-up complete */
#define BHA_IsDiscovering    (BHA_Dummy + 0x32) /* BOOL */
#define BHA_IsClassic        (BHA_Dummy + 0x33) /* BOOL: BR/EDR supported */
#define BHA_IsLE             (BHA_Dummy + 0x34) /* BOOL: Low Energy supported */
#define BHA_Discoverable     (BHA_Dummy + 0x35) /* BOOL (settable): inquiry scan enabled */
#define BHA_Connectable      (BHA_Dummy + 0x36) /* BOOL (settable): page scan enabled */
#define BHA_Address          (BHA_Dummy + 0x40) /* BD_ADDR * (local address, 6 bytes) */
#define BHA_AddressString    (BHA_Dummy + 0x41) /* STRPTR "xx:xx:xx:xx:xx:xx" */
#define BHA_LocalName        (BHA_Dummy + 0x42) /* STRPTR (settable) */
#define BHA_ClassOfDevice    (BHA_Dummy + 0x43) /* ULONG (settable) */
#define BHA_HCIVersion       (BHA_Dummy + 0x44) /* ULONG */
#define BHA_HCIRevision      (BHA_Dummy + 0x45) /* ULONG */
#define BHA_LMPVersion       (BHA_Dummy + 0x46) /* ULONG */
#define BHA_LMPSubversion    (BHA_Dummy + 0x47) /* ULONG */
#define BHA_ManufacturerID   (BHA_Dummy + 0x48) /* ULONG Bluetooth SIG company identifier */
#define BHA_ManufacturerName (BHA_Dummy + 0x49) /* STRPTR */
#define BHA_Features         (BHA_Dummy + 0x4a) /* UBYTE * (8 bytes LMP features) */
#define BHA_ACLMaxPktSize    (BHA_Dummy + 0x50) /* ULONG */
#define BHA_ACLNumPkts       (BHA_Dummy + 0x51) /* ULONG */
#define BHA_SCOMaxPktSize    (BHA_Dummy + 0x52) /* ULONG */
#define BHA_SCONumPkts       (BHA_Dummy + 0x53) /* ULONG */
#define BHA_LEACLMaxPktSize  (BHA_Dummy + 0x54) /* ULONG */
#define BHA_LEACLNumPkts     (BHA_Dummy + 0x55) /* ULONG */
#define BHA_ErrorCount       (BHA_Dummy + 0x60) /* ULONG transport errors seen */
#define BHA_LastHCIError     (BHA_Dummy + 0x61) /* ULONG last HCI status code */

/* BHA_State values */
#define BHS_OFFLINE      0 /* device not opened / task not running */
#define BHS_STARTING     1 /* HCI bring-up in progress */
#define BHS_READY        2 /* operational */
#define BHS_ERROR        3 /* bring-up failed or hardware error */

/* Tags for btGetAttrs(BGA_DEVICE,...) */
#define BDA_Dummy            (BT_TAGBASE + 0x300)
#define BDA_IsClassic        (BDA_Dummy + 0x01) /* BOOL: seen over BR/EDR */
#define BDA_IsLE             (BDA_Dummy + 0x02) /* BOOL: seen over LE */
#define BDA_IsDiscovered     (BDA_Dummy + 0x03) /* BOOL: seen during the current/last discovery */
#define BDA_IsRegistered     (BDA_Dummy + 0x04) /* BOOL: persistent, bindings are attempted */
#define BDA_IsBonded         (BDA_Dummy + 0x05) /* BOOL: keys stored */
#define BDA_IsConnected      (BDA_Dummy + 0x06) /* BOOL: ACL link up */
#define BDA_IsEncrypted      (BDA_Dummy + 0x07) /* BOOL */
#define BDA_IsDead           (BDA_Dummy + 0x08) /* BOOL: repeatedly unreachable */
#define BDA_HasAppBinding    (BDA_Dummy + 0x09) /* BOOL */
#define BDA_IsNewToMe        (BDA_Dummy + 0x0a) /* BOOL: never seen before */
#define BDA_ServicesKnown    (BDA_Dummy + 0x0b) /* BOOL: SDP/GATT enumeration done */
#define BDA_Address          (BDA_Dummy + 0x10) /* BD_ADDR * */
#define BDA_AddressType      (BDA_Dummy + 0x11) /* ULONG BDAT_xxx */
#define BDA_AddressString    (BDA_Dummy + 0x12) /* STRPTR */
#define BDA_Name             (BDA_Dummy + 0x13) /* STRPTR (settable: custom name) */
#define BDA_OrigName         (BDA_Dummy + 0x14) /* STRPTR name reported by the device */
#define BDA_ClassOfDevice    (BDA_Dummy + 0x15) /* ULONG */
#define BDA_Appearance       (BDA_Dummy + 0x16) /* ULONG (LE appearance) */
#define BDA_RSSI             (BDA_Dummy + 0x17) /* LONG dBm, 127 = unknown */
#define BDA_ConnHandle       (BDA_Dummy + 0x18) /* ULONG ACL connection handle */
#define BDA_Role             (BDA_Dummy + 0x19) /* ULONG BDR_xxx */
#define BDA_Hardware         (BDA_Dummy + 0x1a) /* APTR BtHardware */
#define BDA_Binding          (BDA_Dummy + 0x1b) /* APTR class binding */
#define BDA_BindingClass     (BDA_Dummy + 0x1c) /* APTR BtClass */
#define BDA_ServiceList      (BDA_Dummy + 0x1d) /* struct List * of BtService */
#define BDA_NumServices      (BDA_Dummy + 0x1e) /* ULONG */
#define BDA_IDString         (BDA_Dummy + 0x1f) /* STRPTR unique id (address based) */
#define BDA_LastSeen         (BDA_Dummy + 0x20) /* struct DateStamp * */
#define BDA_FirstSeen        (BDA_Dummy + 0x21) /* struct DateStamp * */
#define BDA_VendorID         (BDA_Dummy + 0x22) /* ULONG (DI/PnP) */
#define BDA_ProductID        (BDA_Dummy + 0x23) /* ULONG */
#define BDA_ProductVersion   (BDA_Dummy + 0x24) /* ULONG */
#define BDA_VendorIDSource   (BDA_Dummy + 0x25) /* ULONG 1=SIG 2=USB */
#define BDA_LMPVersion       (BDA_Dummy + 0x26) /* ULONG remote LMP version */
#define BDA_ManufacturerID   (BDA_Dummy + 0x27) /* ULONG remote company id */
#define BDA_AdvData          (BDA_Dummy + 0x28) /* UBYTE * raw advertising/EIR data */
#define BDA_AdvDataLength    (BDA_Dummy + 0x29) /* ULONG */
#define BDA_PairingState     (BDA_Dummy + 0x2a) /* ULONG BDPS_xxx */
#define BDA_PairingRequest   (BDA_Dummy + 0x2b) /* ULONG BPRT_xxx pending request */
#define BDA_PairingPasskey   (BDA_Dummy + 0x2c) /* ULONG passkey to display/compare */
#define BDA_LinkType         (BDA_Dummy + 0x2d) /* ULONG BDLT_xxx of the current link */
#define BDA_BondFlags        (BDA_Dummy + 0x2e) /* ULONG BDKF_xxx: which keys are stored (never the keys themselves) */
#define BDA_InhibitPopup     (BDA_Dummy + 0x40) /* BOOL (settable) */
#define BDA_InhibitClassBind (BDA_Dummy + 0x41) /* BOOL (settable) */
#define BDA_AutoConnect      (BDA_Dummy + 0x42) /* BOOL (settable): connect on I/O and reconnect */
#define BDA_Trusted          (BDA_Dummy + 0x43) /* BOOL (settable): accept incoming connections */

/* BDA_AddressType */
#define BDAT_PUBLIC          0
#define BDAT_RANDOM          1
#define BDAT_PUBLIC_IDENTITY 2
#define BDAT_RANDOM_IDENTITY 3

/* BDA_BondFlags */
#define BDKF_LINKKEY 0x01 /* BR/EDR link key */
#define BDKF_LTK     0x02 /* LE long term key */
#define BDKF_IRK     0x04 /* LE identity resolving key (peer uses private addresses) */
#define BDKF_CSRK    0x08 /* LE signing key */
#define BDKF_SC      0x10 /* LE Secure Connections key */

/* BDA_Role */
#define BDR_NONE             0
#define BDR_CENTRAL          1 /* we initiated (master/central) */
#define BDR_PERIPHERAL       2 /* they initiated (slave/peripheral) */

/* BDA_LinkType */
#define BDLT_NONE            0
#define BDLT_ACL             1 /* BR/EDR ACL */
#define BDLT_LE              2 /* LE ACL */

/* BDA_PairingState */
#define BDPS_NONE            0
#define BDPS_INPROGRESS      1
#define BDPS_WAITUSER        2 /* a BEHMB_PAIRINGREQUEST is outstanding */
#define BDPS_DONE            3
#define BDPS_FAILED          4

/* Pairing request types (BDA_PairingRequest / BEHMB_PAIRINGREQUEST param2) */
#define BPRT_NONE            0
#define BPRT_PINCODE         1 /* legacy PIN entry: reply BPRA_PINCode */
#define BPRT_CONSENT         2 /* just works: reply BPRA_Confirm */
#define BPRT_NUMERICCOMPARE  3 /* show BDA_PairingPasskey, reply BPRA_Confirm */
#define BPRT_PASSKEYDISPLAY  4 /* show BDA_PairingPasskey, nothing to reply */
#define BPRT_PASSKEYENTRY    5 /* reply BPRA_Passkey */

/* Tags for btGetAttrs(BGA_SERVICE,...) */
#define BSVA_Dummy           (BT_TAGBASE + 0x400)
#define BSVA_Device          (BSVA_Dummy + 0x10) /* APTR BtDevice */
#define BSVA_UUID            (BSVA_Dummy + 0x11) /* UBYTE * 16 bytes (128 bit UUID, big endian) */
#define BSVA_UUID16          (BSVA_Dummy + 0x12) /* ULONG 16 bit UUID or 0 */
#define BSVA_UUIDString      (BSVA_Dummy + 0x13) /* STRPTR */
#define BSVA_Name            (BSVA_Dummy + 0x14) /* STRPTR */
#define BSVA_Protocol        (BSVA_Dummy + 0x15) /* ULONG BSVP_xxx */
#define BSVA_PSM             (BSVA_Dummy + 0x16) /* ULONG L2CAP PSM (classic services) */
#define BSVA_RFCOMMChannel         (BSVA_Dummy + 0x17) /* ULONG RFCOMM server channel */
#define BSVA_RecordHandle    (BSVA_Dummy + 0x18) /* ULONG SDP record handle */
#define BSVA_StartHandle     (BSVA_Dummy + 0x19) /* ULONG GATT start handle */
#define BSVA_EndHandle       (BSVA_Dummy + 0x1a) /* ULONG GATT end handle */
#define BSVA_Version         (BSVA_Dummy + 0x1b) /* ULONG profile version (SDP) */
#define BSVA_IsPrimary       (BSVA_Dummy + 0x1c) /* BOOL (GATT) */
#define BSVA_Binding         (BSVA_Dummy + 0x1d) /* APTR class binding */
#define BSVA_BindingClass    (BSVA_Dummy + 0x1e) /* APTR BtClass */
#define BSVA_IDString        (BSVA_Dummy + 0x1f) /* STRPTR */
#define BSVA_EndpointList    (BSVA_Dummy + 0x20) /* struct List * of BtEndpoint */
#define BSVA_NumEndpoints    (BSVA_Dummy + 0x21) /* ULONG */
#define BSVA_ServiceClassIDs (BSVA_Dummy + 0x22) /* UWORD * array of 16 bit service class ids, 0 terminated */
#define BSVA_HIDDescriptor   (BSVA_Dummy + 0x23) /* UBYTE * HID report descriptor from SDP (classic HID), or NULL */
#define BSVA_HIDDescriptorLen (BSVA_Dummy + 0x24) /* ULONG length of BSVA_HIDDescriptor */

/* BSVA_Protocol */
#define BSVP_UNKNOWN 0
#define BSVP_L2CAP   1 /* classic service reachable via L2CAP PSM */
#define BSVP_RFCOMM  2 /* classic service reachable via RFCOMM channel */
#define BSVP_ATT     3 /* GATT service */

/* Tags for btGetAttrs(BGA_ENDPOINT,...)
   An endpoint is something a channel can be allocated on: a classic L2CAP
   PSM, a fixed L2CAP channel, an RFCOMM server channel or a GATT
   characteristic. Endpoints are bidirectional; the direction of a transfer
   is chosen per request (BTPR_READ / BTPR_WRITE, see btChannelSetup()). */
#define BEA_Dummy            (BT_TAGBASE + 0x500)
#define BEA_CanRead          (BEA_Dummy + 0x01) /* BOOL: data can flow device -> host (SDUs, notifications) */
#define BEA_CanWrite         (BEA_Dummy + 0x02) /* BOOL: data can flow host -> device */
#define BEA_Service          (BEA_Dummy + 0x10) /* APTR BtService */
#define BEA_Type             (BEA_Dummy + 0x11) /* ULONG BEPT_xxx */
#define BEA_PSM              (BEA_Dummy + 0x12) /* ULONG */
#define BEA_CID              (BEA_Dummy + 0x13) /* ULONG fixed channel id */
#define BEA_RFCOMMChannel    (BEA_Dummy + 0x14) /* ULONG RFCOMM server channel number */
#define BEA_Handle           (BEA_Dummy + 0x15) /* ULONG GATT characteristic value handle */
#define BEA_UUID16           (BEA_Dummy + 0x16) /* ULONG GATT characteristic uuid (16 bit) */
#define BEA_UUID             (BEA_Dummy + 0x17) /* UBYTE * 16 bytes */
#define BEA_Properties       (BEA_Dummy + 0x18) /* ULONG GATT characteristic properties */
#define BEA_MaxPktSize       (BEA_Dummy + 0x19) /* ULONG MTU */
#define BEA_IsOpen           (BEA_Dummy + 0x1a) /* BOOL channel currently open */
#define BEA_Name             (BEA_Dummy + 0x1b) /* STRPTR */
#define BEA_ReportID         (BEA_Dummy + 0x1c) /* ULONG HID Report Reference: report id (0 = none) */
#define BEA_ReportType       (BEA_Dummy + 0x1d) /* ULONG HID Report Reference: 1 input, 2 output, 3 feature (0 = unknown) */
#define BEA_CCCDHandle       (BEA_Dummy + 0x1e) /* ULONG GATT handle of the characteristic's CCCD (0 = unknown) */
#define BEA_ReportRefHandle  (BEA_Dummy + 0x1f) /* ULONG GATT handle of the HID Report Reference descriptor (0 = none) */

/* BEA_Type */
#define BEPT_L2CAP        1 /* connection oriented L2CAP channel (PSM) */
#define BEPT_L2CAP_FIXED  2 /* fixed L2CAP channel (CID) */
#define BEPT_RFCOMM       3 /* RFCOMM DLC */
#define BEPT_GATT_CHAR    4 /* GATT characteristic (read = notify/indicate, write = write) */

/* Tags for btGetAttrs(BGA_CHANNEL,...)
   A BtChannel is the transfer object (allocated with btAllocChannel() on a
   device and an endpoint, or on the device alone for the control channel);
   several channels may be allocated on the same endpoint, they share the
   underlying L2CAP/RFCOMM channel or characteristic. */
#define BCHA_Dummy            (BT_TAGBASE + 0x600)
#define BCHA_Endpoint         (BCHA_Dummy + 0x01) /* APTR BtEndpoint or NULL */
#define BCHA_Error            (BCHA_Dummy + 0x02) /* LONG */
#define BCHA_Actual           (BCHA_Dummy + 0x03) /* ULONG */
#define BCHA_Device           (BCHA_Dummy + 0x04) /* APTR BtDevice */
#define BCHA_Timeout          (BCHA_Dummy + 0x05) /* ULONG ms, 0 = none (settable) */
#define BCHA_AutoConnect      (BCHA_Dummy + 0x06) /* BOOL (settable): connect device if needed */
#define BCHA_MaxPktSize       (BCHA_Dummy + 0x07) /* ULONG channel MTU */
#define BCHA_NoWait           (BCHA_Dummy + 0x08) /* BOOL (settable): reads return what is available */

/* Tags for application binding and btGetAttrs(BGA_APPBINDING,...) */
#define BABA_Dummy           (BT_TAGBASE + 0x700)
#define BABA_ReleaseHook     (BABA_Dummy + 0x01) /* struct Hook * */
#define BABA_Device          (BABA_Dummy + 0x02) /* APTR BtDevice */
#define BABA_UserData        (BABA_Dummy + 0x03) /* IPTR */
#define BABA_Task            (BABA_Dummy + 0x04) /* struct Task * */
#define BABA_ForceRelease    (BABA_Dummy + 0x10) /* BOOL */

/* Tags for btGetAttrs(BGA_ERRORMSG,...) */
#define BEMA_Dummy           (BT_TAGBASE + 0x800)
#define BEMA_Level           (BEMA_Dummy + 0x10) /* ULONG RETURN_xxx */
#define BEMA_Origin          (BEMA_Dummy + 0x11) /* STRPTR */
#define BEMA_Msg             (BEMA_Dummy + 0x12) /* STRPTR */
#define BEMA_DateStamp       (BEMA_Dummy + 0x13) /* struct DateStamp * */

/* Tags for btGetAttrs(BGA_EVENTNOTE,...) */
#define BENA_Dummy           (BT_TAGBASE + 0x900)
#define BENA_EventID         (BENA_Dummy + 0x01) /* ULONG BEHMB_xxx */
#define BENA_Param1          (BENA_Dummy + 0x02) /* APTR */
#define BENA_Param2          (BENA_Dummy + 0x03) /* APTR */

/* Tags for btGetAttrs(BGA_STACKCFG,...) */
#define BGCA_Dummy           (BT_TAGBASE + 0xa00)
#define BGCA_LogInfo         (BGCA_Dummy + 0x01)
#define BGCA_LogWarning      (BGCA_Dummy + 0x02)
#define BGCA_LogError        (BGCA_Dummy + 0x03)
#define BGCA_LogFailure      (BGCA_Dummy + 0x04)
#define BGCA_SubTaskPri      (BGCA_Dummy + 0x10)
#define BGCA_BootDelay       (BGCA_Dummy + 0x11)
#define BGCA_PopupDeviceNew  (BGCA_Dummy + 0x20)
#define BGCA_PopupDeviceGone (BGCA_Dummy + 0x21)
#define BGCA_PopupPairing    (BGCA_Dummy + 0x22)
#define BGCA_PopupCloseDelay (BGCA_Dummy + 0x23)
#define BGCA_PopupActivateWin (BGCA_Dummy + 0x30)
#define BGCA_PopupWinToFront (BGCA_Dummy + 0x31)
#define BGCA_Discoverable    (BGCA_Dummy + 0x40) /* radios discoverable by default */
#define BGCA_Connectable     (BGCA_Dummy + 0x41) /* radios connectable by default */
#define BGCA_DiscoveryTime   (BGCA_Dummy + 0x42) /* default discovery duration in seconds */
#define BGCA_AutoConnect     (BGCA_Dummy + 0x43) /* reconnect registered devices */
#define BGCA_LocalName       (BGCA_Dummy + 0x44) /* STRPTR default local name */
#define BGCA_PrefsVersion    (BGCA_Dummy + 0x70)

/* Tags for btStartDiscoveryA() */
#define BDSA_Dummy           (BT_TAGBASE + 0xb00)
#define BDSA_Duration        (BDSA_Dummy + 0x01) /* ULONG seconds (default BGCA_DiscoveryTime) */
#define BDSA_Classic         (BDSA_Dummy + 0x02) /* BOOL inquiry (default TRUE if supported) */
#define BDSA_LE              (BDSA_Dummy + 0x03) /* BOOL LE scan (default TRUE if supported) */
#define BDSA_ResolveNames    (BDSA_Dummy + 0x04) /* BOOL remote name requests (default TRUE) */
#define BDSA_ClearOld        (BDSA_Dummy + 0x05) /* BOOL drop unregistered devices first */

/* Tags for btPairDeviceA() and btPairingReplyA() */
#define BPRA_Dummy           (BT_TAGBASE + 0xc00)
#define BPRA_Bond            (BPRA_Dummy + 0x01) /* BOOL store keys (default TRUE) */
#define BPRA_MITM            (BPRA_Dummy + 0x02) /* BOOL require MITM protection */
#define BPRA_IOCapability    (BPRA_Dummy + 0x03) /* ULONG BPIO_xxx */
#define BPRA_PINCode         (BPRA_Dummy + 0x10) /* STRPTR */
#define BPRA_Passkey         (BPRA_Dummy + 0x11) /* ULONG */
#define BPRA_Confirm         (BPRA_Dummy + 0x12) /* BOOL */

#define BPIO_DISPLAYONLY     0
#define BPIO_DISPLAYYESNO    1
#define BPIO_KEYBOARDONLY    2
#define BPIO_NOINPUTNOOUTPUT 3
#define BPIO_KEYBOARDDISPLAY 4

/* Additional channel error codes (io_Error / btGetChannelError()), extending the
   BTIOERR_ codes of <devices/bluetoothhci.h> and the exec IOERR_ codes */
#define BTIOERR_NOTREADY      20 /* hardware not ready */
#define BTIOERR_NOTCONNECTED  21 /* device has no link and auto connect is off */
#define BTIOERR_CONNFAILED    22 /* connection attempt failed */
#define BTIOERR_CHANNELFAILED 23 /* channel/service could not be opened */
#define BTIOERR_REFUSED       24 /* remote refused */
#define BTIOERR_SECURITY      25 /* insufficient security / pairing failed */
#define BTIOERR_NOTSUPPORTED  26 /* endpoint type / request unsupported */
#define BTIOERR_REMOTEERROR   27 /* remote protocol error (see BCHA_Actual) */
#define BTIOERR_DISCONNECTED  28 /* link dropped while the request was pending */

/* Requests for btChannelSetup(ch, req, val, idx).
   Endpoint channels: */
#define BTPR_READ             0x00 /* receive one SDU / notification (the default) */
#define BTPR_WRITE            0x80 /* send one SDU / write the characteristic */
/* Control channel (endpoint NULL): */
#define BTPR_REMOTENAME       0x01 /* read remote name into buffer */
#define BTPR_SDPSEARCH        0x02 /* val = uuid16; buffer receives service record handles */
#define BTPR_SDPATTRIBUTES    0x03 /* val = record handle low, idx = high; buffer receives raw attribute list */
#define BTPR_GATTREAD         0x10 /* val = handle; buffer receives value */
#define BTPR_GATTWRITE        0x11 /* val = handle; buffer holds value */
#define BTPR_GATTWRITENORSP   0x12 /* val = handle; write without response */
#define BTPR_L2CAPECHO        0x20 /* buffer is echoed */
#define BTPR_READRSSI         0x21 /* buffer receives one BYTE */

/* NumToStr types */
#define BNTS_IOERR         1 /* channel/hardware io errors */
#define BNTS_HCISTATUS     2 /* HCI status/error codes */
#define BNTS_MANUFACTURER  3 /* company identifiers */
#define BNTS_MAJORCLASS    4 /* class of device major device class */
#define BNTS_MINORCLASS    5 /* class of device (major<<8|minor) */
#define BNTS_UUID16        6 /* 16 bit UUID names */
#define BNTS_LMPVERSION    7 /* LMP/HCI version numbers */
#define BNTS_APPEARANCE    8 /* LE appearance */
#define BNTS_PAIRINGREQ    9 /* BPRT_xxx */
#define BNTS_EVENT        10 /* BEHMB_xxx */
#define BNTS_HWSTATE      11 /* BHS_xxx */

/* Event Handler stuff */
#define BEHMB_ADDHARDWARE     0x01 /* Param1 = bth (task up, bring-up done) */
#define BEHMB_REMHARDWARE     0x02 /* Param1 = bth */
#define BEHMB_ADDDEVICE       0x03 /* Param1 = bd */
#define BEHMB_REMDEVICE       0x04 /* Param1 = bd */
#define BEHMB_ADDCLASS        0x05 /* Param1 = bc */
#define BEHMB_REMCLASS        0x06 /* Param1 = bc */
#define BEHMB_ADDBINDING      0x07 /* Param1 = bd */
#define BEHMB_REMBINDING      0x08 /* Param1 = bd */
#define BEHMB_ADDERRORMSG     0x09 /* Param1 = bem */
#define BEHMB_REMERRORMSG     0x0a /* Param1 = bem */
#define BEHMB_CONFIGCHG       0x0b /* Param1 = void */
#define BEHMB_DEVICEDEAD      0x0c /* Param1 = bd */
#define BEHMB_HARDWAREERROR   0x0d /* Param1 = bth */
#define BEHMB_DISCOVERYSTART  0x0e /* Param1 = bth */
#define BEHMB_DISCOVERYSTOP   0x0f /* Param1 = bth */
#define BEHMB_DEVICEUPDATE    0x10 /* Param1 = bd (name/rssi/services changed) */
#define BEHMB_DEVICEREGISTERED 0x11 /* Param1 = bd */
#define BEHMB_DEVICEUNREGISTERED 0x12 /* Param1 = bd */
#define BEHMB_DEVICECONNECTED 0x13 /* Param1 = bd */
#define BEHMB_DEVICEDISCONNECTED 0x14 /* Param1 = bd, Param2 = (IPTR) HCI reason */
#define BEHMB_PAIRINGREQUEST  0x15 /* Param1 = bd, Param2 = (IPTR) BPRT_xxx */
#define BEHMB_PAIRINGDONE     0x16 /* Param1 = bd, Param2 = (IPTR) 0 = ok, else HCI/SMP status */
#define BEHMB_SERVICESCHG     0x17 /* Param1 = bd */

#define BEHMF_ADDHARDWARE     (1L<<BEHMB_ADDHARDWARE)
#define BEHMF_REMHARDWARE     (1L<<BEHMB_REMHARDWARE)
#define BEHMF_ADDDEVICE       (1L<<BEHMB_ADDDEVICE)
#define BEHMF_REMDEVICE       (1L<<BEHMB_REMDEVICE)
#define BEHMF_ADDCLASS        (1L<<BEHMB_ADDCLASS)
#define BEHMF_REMCLASS        (1L<<BEHMB_REMCLASS)
#define BEHMF_ADDBINDING      (1L<<BEHMB_ADDBINDING)
#define BEHMF_REMBINDING      (1L<<BEHMB_REMBINDING)
#define BEHMF_ADDERRORMSG     (1L<<BEHMB_ADDERRORMSG)
#define BEHMF_REMERRORMSG     (1L<<BEHMB_REMERRORMSG)
#define BEHMF_CONFIGCHG       (1L<<BEHMB_CONFIGCHG)
#define BEHMF_DEVICEDEAD      (1L<<BEHMB_DEVICEDEAD)
#define BEHMF_HARDWAREERROR   (1L<<BEHMB_HARDWAREERROR)
#define BEHMF_DISCOVERYSTART  (1L<<BEHMB_DISCOVERYSTART)
#define BEHMF_DISCOVERYSTOP   (1L<<BEHMB_DISCOVERYSTOP)
#define BEHMF_DEVICEUPDATE    (1L<<BEHMB_DEVICEUPDATE)
#define BEHMF_DEVICEREGISTERED (1L<<BEHMB_DEVICEREGISTERED)
#define BEHMF_DEVICEUNREGISTERED (1L<<BEHMB_DEVICEUNREGISTERED)
#define BEHMF_DEVICECONNECTED (1L<<BEHMB_DEVICECONNECTED)
#define BEHMF_DEVICEDISCONNECTED (1L<<BEHMB_DEVICEDISCONNECTED)
#define BEHMF_PAIRINGREQUEST  (1L<<BEHMB_PAIRINGREQUEST)
#define BEHMF_PAIRINGDONE     (1L<<BEHMB_PAIRINGDONE)
#define BEHMF_SERVICESCHG     (1L<<BEHMB_SERVICESCHG)

/* Configuration stuff */

#if AROS_BIG_ENDIAN
/* As with poseidon.prefs the outermost FORM ID is endian specific: the chunk
 * contents are native structures. */
#define IFFFORM_BTCFG      MAKE_ID('B','T','B','C')
#else
#define IFFFORM_BTCFG      MAKE_ID('B','T','L','C')
#endif
#define IFFFORM_BTSTACKCFG   MAKE_ID('S','T','K','C') /* stack config */
#define IFFFORM_BTDEVICECFG  MAKE_ID('D','E','V','C') /* per remote device */
#define IFFFORM_BTCLASSCFG   MAKE_ID('C','L','S','C') /* per class */
#define IFFFORM_BTHWDEVICE   MAKE_ID('B','H','W','D') /* hardware entry in STKC */
#define IFFFORM_BTCLASS      MAKE_ID('B','C','L','S') /* class entry in STKC */
#define IFFFORM_BTCLASSDATA  MAKE_ID('G','C','P','D') /* class private data (global) */
#define IFFFORM_BTDEVCFGDATA MAKE_ID('D','C','F','G') /* device config data */
#define IFFFORM_BTDEVCLSDATA MAKE_ID('D','C','P','D') /* device class private data */
#define IFFFORM_BTSVCCFGDATA MAKE_ID('S','C','F','G') /* service config data */
#define IFFFORM_BTSVCCLSDATA MAKE_ID('S','C','P','D') /* service class private data */

#define IFFCHNK_OWNER      MAKE_ID('O','W','N','R')
#define IFFCHNK_NAME       MAKE_ID('N','A','M','E')
#define IFFCHNK_UNIT       MAKE_ID('U','N','I','T')
#define IFFCHNK_OFFLINE    MAKE_ID('O','F','F','L')
#define IFFCHNK_GLOBALCFG  MAKE_ID('G','C','F','G')
#define IFFCHNK_DEVID      MAKE_ID('D','V','I','D')
#define IFFCHNK_SVCID      MAKE_ID('S','V','I','D')
#define IFFCHNK_FORCEDBIND MAKE_ID('F','B','N','D')
#define IFFCHNK_POPUP      MAKE_ID('P','O','P','O')
#define IFFCHNK_REGDEVICE  MAKE_ID('D','R','E','G') /* struct BtRegDevCfg */
#define IFFCHNK_DEVNAME    MAKE_ID('D','N','A','M') /* name the device reported (NAME = custom name) */
#define IFFCHNK_KEYS       MAKE_ID('K','E','Y','S') /* bond keys (opaque, versioned) */

/* Public definitions to Private interfaces */
#if !defined(_LIBRARIES_BLUETOOTH_H)
struct BtBase
{
    struct Library      bt_Library;       /* standard */
};

struct BtEventHook
{
    struct Node         beh_Node;         /* Node linkage */
};

struct BtEventNote
{
    struct Message      ben_Msg;          /* Intertask communication message */
};

struct BtErrorMsg
{
    struct Node         bem_Node;         /* Node linkage */
};

struct BtIFFContext
{
    struct Node         bic_Node;         /* Node linkage */
};

struct BtClass
{
    struct Node         bc_Node;          /* Node linkage */
};

struct BtAppBinding
{
    struct Node         bab_Node;         /* Node linkage */
};

struct BtHardware
{
    struct Node         bth_Node;         /* Node linkage */
};

struct BtDevice
{
    struct Node         bd_Node;          /* Node linkage */
};

struct BtService
{
    struct Node         bsv_Node;         /* Node linkage */
};

struct BtEndpoint
{
    struct Node         bep_Node;         /* Node linkage */
};

struct BtChannel
{
    struct Message      bch_Msg;           /* Intertask communication message */
};
#endif /* !_LIBRARIES_BLUETOOTH_H */

#if defined(__GNUC__)
# pragma pack(2)
#endif

/* BGCA_PopupDeviceNew definitions */
#define BGCP_NEVER      0 /* never open a pop-up window */
#define BGCP_ISNEW      1 /* popup, if this is the first time the device is seen */
#define BGCP_NOBINDING  2 /* popup, if a registered device has no binding */
#define BGCP_ALWAYS     3 /* popup always */

struct BtGlobalCfg
{
    ULONG bgc_ChunkID;                    /* ChunkID=IFFCHNK_GLOBALCFG */
    ULONG bgc_Length;                     /* sizeof(struct BtGlobalCfg)-8 */
    BOOL  bgc_LogInfo;                    /* Log normal messages */
    BOOL  bgc_LogWarning;                 /* Log warnings */
    BOOL  bgc_LogError;                   /* Log errors */
    BOOL  bgc_LogFailure;                 /* Log failures */
    ULONG bgc_BootDelay;                  /* boot delay */
    WORD  bgc_SubTaskPri;                 /* Subtask priority */
    UWORD bgc_PopupDeviceNew;             /* New device popup */
    BOOL  bgc_PopupDeviceGone;            /* Device removed popup */
    BOOL  bgc_PopupPairing;               /* Pairing request popup */
    ULONG bgc_PopupCloseDelay;            /* Delay in seconds before closing */
    BOOL  bgc_PopupActivateWin;           /* Activate window on opening */
    BOOL  bgc_PopupWinToFront;            /* Pop window to front on content change */
    BOOL  bgc_Discoverable;               /* Radios discoverable by default */
    BOOL  bgc_Connectable;                /* Radios connectable by default */
    ULONG bgc_DiscoveryTime;              /* Default discovery duration (seconds) */
    BOOL  bgc_AutoConnect;                /* Reconnect registered devices automatically */
    ULONG bgc_PrefsVersion;               /* Reference version of prefs saved */
    UBYTE bgc_LocalName[64];              /* Default local name (empty = driver default) */
};

struct BtPoPoCfg
{
    ULONG bpc_ChunkID;                    /* ChunkID=IFFCHNK_POPUP */
    ULONG bpc_Length;                     /* sizeof(struct BtPoPoCfg)-8 */
    BOOL  bpc_InhibitPopup;               /* Inhibit opening of popup window */
    BOOL  bpc_NoClassBind;                /* Inhibit class scan */
    BOOL  bpc_AutoConnect;                /* Connect on demand / reconnect */
    BOOL  bpc_Trusted;                    /* Accept incoming connections */
};

/* Persistent record of a registered device (inside FORM DEVC) */
struct BtRegDevCfg
{
    ULONG brd_ChunkID;                    /* ChunkID=IFFCHNK_REGDEVICE */
    ULONG brd_Length;                     /* sizeof(struct BtRegDevCfg)-8 */
    UBYTE brd_Address[6];                 /* BD_ADDR */
    UBYTE brd_AddressType;                /* BDAT_xxx */
    UBYTE brd_Flags;                      /* BRDF_xxx */
    ULONG brd_ClassOfDevice;
    UWORD brd_Appearance;
    UWORD brd_VendorIDSource;
    UWORD brd_VendorID;
    UWORD brd_ProductID;
    UWORD brd_ProductVersion;
    UWORD brd_Pad;
};

#define BRDF_CLASSIC 0x01 /* device supports BR/EDR */
#define BRDF_LE      0x02 /* device supports LE */
#define BRDF_BONDED  0x04 /* keys are stored */

#if defined(__GNUC__)
# pragma pack()
#endif

/* *** pluggable controller-firmware loaders ***
 *
 * Some radios (e.g. Realtek RTL8761/8852) answer HCI from a ROM bootloader but
 * cannot transmit/receive until the host downloads a firmware patch. Support for
 * a given family lives in a separate loadable module in DEVS:Bluetooth/FWLoaders/,
 * NOT in bluetooth.library. BTStackLoader opens each module; the module registers
 * a "struct BtFirmwareLoader" with btAddFirmwareLoader(). During a controller's
 * bring-up the stack offers the controller to every registered loader (fwl_Match)
 * and lets the best match run (fwl_Load), giving it a synchronous HCI channel and
 * a log callback via "struct BtFirmwareContext". Firmware BLOBS are read from disk
 * only (never the network).
 *
 * These are in-memory runtime structures (they hold function pointers), so they
 * live OUTSIDE the on-disk pack(2) region above - their pointers must keep the
 * platform's natural alignment. */

struct BtFirmwareContext
{
    /* controller identity (filled in by the library) */
    UWORD   fwc_Manufacturer;             /* Bluetooth SIG company id */
    UWORD   fwc_HCIVersion;
    UWORD   fwc_HCIRevision;
    UWORD   fwc_LMPVersion;
    UWORD   fwc_LMPSubversion;
    /* send one HCI command and block until it completes. opcode is OGF<<10|OCF.
     * status gets the HCI status byte; up to respmax bytes of return parameters
     * (status byte first) are copied to resp with the count in *resplen.
     * returns 0 on a completed, successful command, non-zero otherwise. Any of
     * status/resp/resplen may be NULL. Provided by the library. */
    LONG  (*fwc_HciCommand)(struct BtFirmwareContext *ctx, UWORD opcode,
                            CONST_APTR params, UWORD plen,
                            UBYTE *status, UBYTE *resp, UWORD *resplen, UWORD respmax);
    /* append a line to the bluetooth message log (RETURN_OK/WARN/ERROR/FAIL) */
    void  (*fwc_Log)(struct BtFirmwareContext *ctx, LONG level, CONST_STRPTR fmt, ...);
    APTR    fwc_Private;                  /* library use only */
};

struct BtFirmwareLoader
{
    struct MinNode fwl_Node;              /* linked into the stack's loader list */
    CONST_STRPTR   fwl_Name;              /* human-readable loader name */
    /* return a non-zero priority if this loader handles the controller in ctx,
     * else 0 (identity-only; must not issue HCI). Highest priority wins. */
    ULONG        (*fwl_Match)(struct BtFirmwareLoader *self, struct BtFirmwareContext *ctx);
    /* download/apply firmware; return 0 on success (or if nothing was needed). */
    LONG         (*fwl_Load)(struct BtFirmwareLoader *self, struct BtFirmwareContext *ctx);
    APTR           fwl_UserData;          /* loader's own use */
};

#endif /* LIBRARIES_BLUETOOTH_H */
