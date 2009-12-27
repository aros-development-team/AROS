/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <exec/types.h>

#define PREFS_PATH_ENV              "ENV:AROSTCP"
#define PREFS_PATH_ENVARC           "ENVARC:AROSTCP"
#define AROSTCP_PACKAGE_VARIABLE    "SYS/Packages/AROSTCP"

#define IPBUFLEN 64
#define NAMEBUFLEN 512

#define IPCHARS "0123456789."
#define NAMECHARS "0123456789abcdefghijklmnopqrstuvwxyz-"

#define MAXINTERFACES 15

#define DEFAULTNAME "eth0"
#define DEFAULTIP "192.168.0.188"
#define DEFAULTMASK "255.255.255.0"
#define DEFAULTGATE "192.168.0.1"
#define DEFAULTDNS "192.168.0.1"
#define DEFAULTDEVICE "DEVS:networks/pcnet32.device"
#define DEFAULTHOST "arosbox"
#define DEFAULTDOMAIN "arosnet"

enum ErrorCode
{
    ALL_OK,
    UNKNOWN_ERROR,
    NOT_SAVED_PREFS_ENV,
    NOT_SAVED_PREFS_ENVARC,
    NOT_COPIED_FILES_ENV,
    NOT_COPIED_FILES_ENVARC,
    NOT_RESTARTED_STACK
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
STRPTR GetHost(void);
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
void SetHost(STRPTR w);
void SetDomain(STRPTR w);
void SetInterfaceCount(LONG w);
void SetAutostart(BOOL w);
