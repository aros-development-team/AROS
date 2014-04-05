/*
    Copyright © 2009-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

#define PREFS_PATH_ENV              "ENV:AROSTCP"
#define PREFS_PATH_ENVARC           "ENVARC:AROSTCP"
#define AROSTCP_PACKAGE_VARIABLE    "SYS/Packages/AROSTCP"

#define IPBUFLEN (15 + 1)
#define NAMEBUFLEN 128
#define SMBBUFLEN (16 + 1)

#define IPCHARS "0123456789."
#define NAMECHARS "0123456789abcdefghijklmnopqrstuvwxyz-"

#define MAXINTERFACES 15
#define MAXHOSTS 30
#define MAXATCOMMANDS 5

#define DEFAULTNAME "net0"
#define DEFAULTIP "192.168.0.188"
#define DEFAULTMASK "255.255.255.0"
#define DEFAULTGATE "192.168.0.1"
#define DEFAULTDNS "192.168.0.1"
#define DEFAULTDEVICE "DEVS:networks/pcnet32.device"
#define DEFAULTHOST "arosbox"
#define DEFAULTDOMAIN "arosnet"

#define MAXNETWORKS 100
#define MAXSERVERS 10

#define WIRELESS_PATH_ENV              "ENV:Sys"
#define WIRELESS_PATH_ENVARC           "ENVARC:Sys"

#define MOBILEBB_PATH_ENV              "ENV:"
#define MOBILEBB_PATH_ENVARC           "ENVARC:"

#define SERVER_PATH_STORAGE "SYS:Storage/DOSDrivers"
#define SERVER_PATH_ENV     "ENV:SMB"
#define AUTOMOUNT_VARIABLE  "AROSTCP/ServerAutoMounts"
#define SERVER_HANDLER      "smb-handler"

#define SSIDBUFLEN (32 + 1)
#define KEYBUFLEN (64 + 1)

#define DEFAULTSERVERDEV "SMB0"

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

struct Interface
{
    TEXT name[NAMEBUFLEN];
    BOOL ifDHCP;
    TEXT IP[IPBUFLEN];
    TEXT mask[IPBUFLEN];
    TEXT device[NAMEBUFLEN];
    LONG unit;
    BOOL up;
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

struct Server
{
    TEXT device[SMBBUFLEN];
    TEXT host[NAMEBUFLEN];
    TEXT service[SMBBUFLEN];
    TEXT user[SMBBUFLEN];
    TEXT group[SMBBUFLEN];
    TEXT pass[SMBBUFLEN];
    BOOL active;
};

struct TCPPrefs
{
    struct Interface interface[MAXINTERFACES];
    LONG interfacecount;
    BOOL DHCP;
    TEXT gate[IPBUFLEN];
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
    struct Server servers[MAXSERVERS];
    LONG serverCount;
};

void InitNetworkPrefs(CONST_STRPTR directory, BOOL use, BOOL save);
void InitInterface(struct Interface *iface);
enum ErrorCode SaveNetworkPrefs();
enum ErrorCode UseNetworkPrefs();

struct Interface * GetInterface(LONG index);
STRPTR GetName(struct Interface *iface);
BOOL   GetIfDHCP(struct Interface *iface);
STRPTR GetIP(struct Interface *iface);
STRPTR GetMask(struct Interface *iface);
STRPTR GetDevice(struct Interface *iface);
LONG   GetUnit(struct Interface *iface);
BOOL   GetUp(struct Interface *iface);

BOOL   GetDHCP(void);
STRPTR GetGate(void);
STRPTR GetDNS(LONG m);
STRPTR GetHostname(void);
STRPTR GetDomain(void);
LONG   GetInterfaceCount(void);
BOOL   GetAutostart(void);

void SetInterface
(
    struct Interface *iface, STRPTR name, BOOL dhcp, STRPTR IP,
    STRPTR mask, STRPTR device, LONG unit, BOOL up
);
void SetName(struct Interface *iface, STRPTR w);
void SetIfDHCP(struct Interface *iface, BOOL w);
void SetIP(struct Interface *iface, STRPTR w);
void SetMask(struct Interface *iface, STRPTR w);
void SetDevice(struct Interface *iface, STRPTR w);
void SetUnit(struct Interface *iface, LONG w);
void SetUp(struct Interface *iface, BOOL w);

void SetDHCP(BOOL w);
void SetGate(STRPTR w);
void SetDNS(LONG m, STRPTR w);
void SetHostname(STRPTR w);
void SetDomain(STRPTR w);
void SetInterfaceCount(LONG w);
void SetAutostart(BOOL w);

void InitHost(struct Host *host);
void InitNetwork(struct Network *net);
void InitServer(struct Server *server, char *workgroup);

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

struct Server *GetServer(LONG index);
STRPTR GetServerDevice(struct Server *server);
STRPTR GetServerHost(struct Server *server);
STRPTR GetServerService(struct Server *server);
STRPTR GetServerUser(struct Server *server);
STRPTR GetServerGroup(struct Server *server);
STRPTR GetServerPass(struct Server *server);
BOOL GetServerActive(struct Server *server);

LONG GetServerCount(void);

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

void SetServer
(
    struct Server *server, STRPTR device, STRPTR host, STRPTR service,
    STRPTR user, STRPTR group, STRPTR pass, BOOL active
);
void SetServerDevice(struct Server *server, STRPTR w);
void SetServerHost(struct Server *server, STRPTR w);
void SetServerService(struct Server *server, STRPTR w);
void SetServerUser(struct Server *server, STRPTR w);
void SetServerGroup(struct Server *server, STRPTR w);
void SetServerPass(struct Server *server, STRPTR w);
void SetServerActive(struct Server *net, BOOL w);

void SetServerCount(LONG w);

