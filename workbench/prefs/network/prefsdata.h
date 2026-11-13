/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _PREFSDATA_H_
#define _PREFSDATA_H_

#include <exec/types.h>
#include <exec/lists.h>

#include "netprefs_module.h"    /* struct MountedShare - the shares page model */

#define PREFS_PATH_ENV              "ENV:AROSTCP"
#define PREFS_PATH_ENVARC           "ENVARC:AROSTCP"
#define AROSTCP_PACKAGE_VARIABLE    "SYS/Packages/AROSTCP"

#define IPBUFLEN (15 + 1)
#define IP6BUFLEN (39 + 1)
#define NAMEBUFLEN 128

#define IPCHARS "0123456789."
#define IP6CHARS "0123456789abcdefABCDEF:"
#define NAMECHARS "0123456789abcdefghijklmnopqrstuvwxyz-"

#define MAXINTERFACES 15
#define MAXHOSTS 30
#define MAXATCOMMANDS 5

#define DEFAULTNAME "net0"
#define DEFAULTTUNNELNAME "sit0"
#define DEFAULTIP "192.168.0.188"
#define DEFAULTMASK "255.255.255.0"
#define DEFAULTGATE "192.168.0.1"
#define DEFAULTDNS "192.168.0.1"
#define DEFAULTDEVICE "DEVS:networks/pcnet32.device"
#define DEFAULTHOST "arosbox"
#define DEFAULTDOMAIN "arosnet"

#define MAXNETWORKS 100

#define WIRELESS_PATH_ENV              "ENV:Sys"
#define WIRELESS_PATH_ENVARC           "ENVARC:Sys"

#define MOBILEBB_PATH_ENV              "ENV:"
#define MOBILEBB_PATH_ENVARC           "ENVARC:"

/* Mounted network shares: one DOS Mountfile per share, owned by whichever
 * registered netprefs filesystem module claims it (CIFS, Envoy FS, ...).
 * Saved copies go to storage; the in-use session copies live in ENV:. */
#define MOUNT_PATH_STORAGE "SYS:Storage/DOSDrivers"
#define MOUNT_PATH_ENV     "ENV:DOSDrivers"
#define AUTOMOUNT_VARIABLE "AROSTCP/ServerAutoMounts"

#define SSIDBUFLEN (32 + 1)
#define KEYBUFLEN (64 + 1)

enum ErrorCode
{
    ALL_OK,
    UNKNOWN_ERROR,
    NOT_SAVED_PREFS_ENV,
    NOT_SAVED_PREFS_ENVARC,
    NOT_COPIED_FILES_ENV,
    NOT_COPIED_FILES_ENVARC,
    NOT_RESTARTED_STACK,
    NOT_RESTARTED_WIRELESS,
    NOT_RESTARTED_MOBILE,
    MULTIPLE_IFACES
};

enum IPMode
{
    IP_MODE_DHCP   = 0,   /* Obtain address automatically via DHCP        */
    IP_MODE_AUTO   = 1,   /* Auto: zeroconf (IPv4) / link-local (IPv6)    */
    IP_MODE_MANUAL = 2    /* Manually configured static address            */
};

struct Interface
{
    TEXT name[NAMEBUFLEN];
    /* Protocol objects the interface carries: ProtocolAddress nodes, each
     * tagged in ln_Type with the owning plugin's id.  An interface may hold
     * any set of these - IPv4 only, IPv6 only, both, or none.  The core never
     * looks inside a node; it only routes each back to its plugin. */
    struct List protoAddrs;
    TEXT device[NAMEBUFLEN];
    LONG unit;
    BOOL up;
    /* DEFER ("Connect once available"): the device may not be there when the
     * stack starts (hot-plugged, wireless, Bluetooth); the stack keeps trying
     * in the background instead of asking what to do.  See AROSTCP
     * SSC_TEMPLATE. */
    BOOL defer;
    /* 6in4 (SIT) tunnel pseudo-interface.  When isTunnel is TRUE the interface
     * has no SANA-II device; the outer IPv4 endpoints below carry the tunnel,
     * and the inner IPv6 is just another protocol object on protoAddrs.
     * See AROSTCP SSC_TEMPLATE (TUNNEL/TSRC/TDST/TTL). */
    BOOL isTunnel;
    TEXT tunnelRemote[IPBUFLEN]; /* TDST - outer IPv4 remote endpoint (required) */
    TEXT tunnelLocal[IPBUFLEN];  /* TSRC - outer IPv4 local endpoint (optional)  */
    LONG tunnelTTL;              /* TTL  - outer IPv4 TTL (0 = default)           */
};

struct Host
{
    TEXT address[IPBUFLEN];
    TEXT names[NAMEBUFLEN];
};

struct Network
{
    TEXT name[NAMEBUFLEN];
    TEXT key[KEYBUFLEN];
    UWORD encType;
    BOOL hidden;
    BOOL adHoc;
    BOOL keyIsHex;
};

struct MobileBroadBand
{
    TEXT devicename[NAMEBUFLEN];
    LONG unit;
    TEXT atcommand[MAXATCOMMANDS][NAMEBUFLEN];
    TEXT username[NAMEBUFLEN];
    TEXT password[NAMEBUFLEN];
    LONG timeout;
    BOOL autostart;
};

struct TCPPrefs
{
    struct Interface interface[MAXINTERFACES];
    LONG interfacecount;
    BOOL DHCP;
    TEXT DNS[2][IPBUFLEN];
    TEXT host[NAMEBUFLEN];
    TEXT domain[NAMEBUFLEN];
    BOOL autostart;
    struct Host hosts[MAXHOSTS];
    LONG hostCount;
    struct Network networks[MAXNETWORKS];
    LONG networkCount;
    struct MobileBroadBand mobile;
    STRPTR wirelessDevice;
    LONG wirelessUnit;
    struct List mountedShares;      /* struct MountedShare nodes (netprefs_module.h) */
};

void InitNetworkPrefs(CONST_STRPTR directory, BOOL use, BOOL save);
void InitInterface(struct Interface *iface);
void InitTunnel(struct Interface *iface);
enum ErrorCode SaveNetworkPrefs();
enum ErrorCode UseNetworkPrefs();

struct Interface * GetInterface(LONG index);
STRPTR GetName(struct Interface *iface);
struct List *GetProtoAddrs(struct Interface *iface); /* interface's protocol objects */
STRPTR GetDevice(struct Interface *iface);
LONG   GetUnit(struct Interface *iface);
BOOL   GetUp(struct Interface *iface);
BOOL   GetDefer(struct Interface *iface);
BOOL   GetIsTunnel(struct Interface *iface);
STRPTR GetTunnelRemote(struct Interface *iface);
STRPTR GetTunnelLocal(struct Interface *iface);
LONG   GetTunnelTTL(struct Interface *iface);

BOOL   GetDHCP(void);
STRPTR GetDNS(LONG m);
STRPTR GetHostname(void);
STRPTR GetDomain(void);
LONG   GetInterfaceCount(void);
BOOL   GetAutostart(void);

void SetName(struct Interface *iface, STRPTR w);
void SetDevice(struct Interface *iface, STRPTR w);
void SetUnit(struct Interface *iface, LONG w);
void SetUp(struct Interface *iface, BOOL w);
void SetDefer(struct Interface *iface, BOOL w);
void SetIsTunnel(struct Interface *iface, BOOL w);
void SetTunnelRemote(struct Interface *iface, STRPTR w);
void SetTunnelLocal(struct Interface *iface, STRPTR w);
void SetTunnelTTL(struct Interface *iface, LONG w);

void SetDHCP(BOOL w);
void SetDNS(LONG m, STRPTR w);
void SetHostname(STRPTR w);
void SetDomain(STRPTR w);
void SetInterfaceCount(LONG w);
void SetAutostart(BOOL w);

void InitHost(struct Host *host);
void InitNetwork(struct Network *net);

/* ------------------------------------------------------------------------
 * Mounted shares - generic, module-backed (see netprefs_module.h).
 * ------------------------------------------------------------------------ */
struct MountedShare *GetShare(LONG index);
LONG GetShareCount(void);
struct MountedShare *AddShare(UBYTE fshID);     /* alloc + AddTail to prefs */
void ClearShares(void);                         /* empty the prefs share list */
void FreeShares(struct List *list);
BOOL ReadMounts(void);
BOOL WriteMounts(CONST_STRPTR destdir, CONST_STRPTR envdir);
BOOL MountShares(void);

struct Host *GetHost(LONG index);
STRPTR GetHostNames(struct Host *host);
STRPTR GetHostAddress(struct Host *host);
LONG GetHostCount(void);

struct Network *GetNetwork(LONG index);
STRPTR GetNetworkName(struct Network *net);
STRPTR GetKey(struct Network *net);
UWORD GetEncType(struct Network *net);
BOOL GetHidden(struct Network *net);
BOOL GetAdHoc(struct Network *net);

LONG GetNetworkCount(void);
STRPTR GetWirelessDevice(void);
LONG GetWirelessUnit(void);

BOOL GetMobile_Autostart(void);
STRPTR GetMobile_atcommand(ULONG i);
STRPTR GetMobile_devicename(void);
STRPTR GetMobile_username(void);
STRPTR GetMobile_password(void);
LONG GetMobile_unit(void);
LONG GetMobile_timeout(void);
LONG GetMobile_atcommandcount(void);

void SetHost
(
    struct Host *host, STRPTR name, STRPTR address
);
void SetHostNames(struct Host *host, STRPTR w);
void AddHostName(struct Host *host, STRPTR w);
void SetHostAddress(struct Host *host, STRPTR w);

void SetHostCount(LONG w);

void SetNetwork
(
    struct Network *net, STRPTR name, UWORD encType, STRPTR key,
    BOOL keyIsHex, BOOL hidden, BOOL adHoc
);
void SetNetworkName(struct Network *net, STRPTR w);
void SetKey(struct Network *net, STRPTR w, BOOL keyIsHex);
void SetEncType(struct Network *net, UWORD w);
void SetHidden(struct Network *net, BOOL w);
void SetAdHoc(struct Network *net, BOOL w);

void SetNetworkCount(LONG w);
void SetWirelessDevice(STRPTR w);
void SetWirelessUnit(LONG w);

void SetMobile_Autostart(BOOL w);
void SetMobile_atcommand(ULONG i,STRPTR w);
void SetMobile_devicename(STRPTR w);
void SetMobile_username(STRPTR w);
void SetMobile_password(STRPTR w);
void SetMobile_unit(LONG w);
void SetMobile_timeout(LONG w);

/* ------------------------------------------------------------------------
 * Managed network services (db/services.d/<name>) - the Services tab.
 * ------------------------------------------------------------------------ */
#define MAX_NETSERVICES     32

struct NetSvcEntry
{
    char  nse_Name[32];         /* service name = config file name */
    char  nse_FriendlyName[64]; /* Name= - shown in the UI (falls back to nse_Name) */
    char  nse_Path[256];        /* Path= */
    char  nse_Order[8];         /* Order= - launch priority, higher starts earlier */
    char  nse_StopSig[8];       /* StopSig= (kept verbatim) */
    char  nse_Policy[16];       /* Policy= (kept verbatim) */
    char  nse_ConfigTool[256];  /* ConfigTool= - the service's prefs editor */
    BOOL  nse_Enabled;          /* Enabled= (yes/no) - what the tab toggles */
};

extern struct NetSvcEntry netservices[MAX_NETSERVICES];
extern int netserviceCount;

BOOL ReadNetServices(void);                         /* ENV: then ENVARC: then install default */
BOOL WriteNetServices(CONST_STRPTR prefspath);      /* <prefspath>/db/services.d/<name> */


#endif /* _PREFSDATA_H_ */
