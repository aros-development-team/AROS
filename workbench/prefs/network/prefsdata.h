/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <exec/types.h>

struct TCPPrefs
{
    TEXT IP[64];
    TEXT mask[64];
    TEXT gate[64];
    TEXT DNS[2][64];
    BOOL DHCP;
    TEXT device[512];
    TEXT host[512];
    TEXT domain[512];
};

void SetDefaultValues();
void ReadNetworkPrefs();
BOOL SaveNetworkPrefs();
BOOL UseNetworkPrefs();


STRPTR GetIP();
STRPTR GetMask();
STRPTR GetGate();
STRPTR GetDNS(LONG m);
BOOL GetDHCP();
STRPTR GetDevice();
STRPTR GetHost();
STRPTR GetDomain();

void SetIP(STRPTR w);
void SetMask(STRPTR w);
void SetGate(STRPTR w);
void SetDNS(LONG m, STRPTR w);
void SetDHCP(BOOL w);
void SetDevice(STRPTR w);
void SetHost(STRPTR w);
void SetDomain(STRPTR w);
