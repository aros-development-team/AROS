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
    BOOL DHCP;
    TEXT IP[IPBUFLEN];
    TEXT mask[IPBUFLEN];
    TEXT device[NAMEBUFLEN];
    LONG unit;
};

struct TCPPrefs
{
    struct Interface interface[MAXINTERFACES];
    LONG interfacecount;
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
BOOL   GetDHCP(struct Interface *iface);
STRPTR GetIP(struct Interface *iface);
STRPTR GetMask(struct Interface *iface);
STRPTR GetDevice(struct Interface *iface);
LONG   GetUnit(struct Interface *iface);

STRPTR GetGate(void);
STRPTR GetDNS(LONG m);
STRPTR GetHost(void);
STRPTR GetDomain(void);
LONG   GetInterfaceCount(void);
BOOL   GetAutostart(void);

void SetInterface
(
    struct Interface *iface, STRPTR name, BOOL dhcp, STRPTR IP,
    STRPTR mask, STRPTR device, LONG unit
);
void SetName(struct Interface *iface, STRPTR w);
void SetDHCP(struct Interface *iface, BOOL w);
void SetIP(struct Interface *iface, STRPTR w);
void SetMask(struct Interface *iface, STRPTR w);
void SetDevice(struct Interface *iface, STRPTR w);
void SetUnit(struct Interface *iface, LONG w);

void SetGate(STRPTR w);
void SetDNS(LONG m, STRPTR w);
void SetHost(STRPTR w);
void SetDomain(STRPTR w);
void SetInterfaceCount(LONG w);
void SetAutostart(BOOL w);
