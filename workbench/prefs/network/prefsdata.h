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

struct TCPPrefs
{
    TEXT IP[IPBUFLEN];
    TEXT mask[IPBUFLEN];
    TEXT gate[IPBUFLEN];
    TEXT DNS[2][IPBUFLEN];
    BOOL DHCP;
    TEXT device[NAMEBUFLEN];
    TEXT host[NAMEBUFLEN];
    TEXT domain[NAMEBUFLEN];
    BOOL autostart;
};

void InitNetworkPrefs(CONST_STRPTR directory, BOOL use, BOOL save);
enum ErrorCode SaveNetworkPrefs();
enum ErrorCode UseNetworkPrefs();


STRPTR GetIP();
STRPTR GetMask();
STRPTR GetGate();
STRPTR GetDNS(LONG m);
BOOL GetDHCP();
STRPTR GetDevice();
STRPTR GetHost();
STRPTR GetDomain();
BOOL GetAutostart();

void SetIP(STRPTR w);
void SetMask(STRPTR w);
void SetGate(STRPTR w);
void SetDNS(LONG m, STRPTR w);
void SetDHCP(BOOL w);
void SetDevice(STRPTR w);
void SetHost(STRPTR w);
void SetDomain(STRPTR w);
void SetAutostart(BOOL w);
