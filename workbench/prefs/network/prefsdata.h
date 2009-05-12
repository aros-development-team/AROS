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
    TEXT interf[4096];
    TEXT host[1000];
    TEXT domain[1000];
    TEXT config[8192]; 
};

void SetDefaultValues();
void ReadTCPPrefs();
int WriteTCPPrefs(STRPTR DestDir);

STRPTR GetIP();
STRPTR GetMask();
STRPTR GetGate();
STRPTR GetDNS(LONG m);
BOOL GetDHCP();
STRPTR GetInterf();
STRPTR GetHost();
STRPTR GetDomain();
STRPTR GetConfig();

void SetIP(STRPTR w);
void SetMask(STRPTR w);
void SetGate(STRPTR w);
void SetDNS(LONG m, STRPTR w);
void SetDHCP(BOOL w);
void SetInterf(STRPTR w);
void SetHost(STRPTR w);
void SetDomain(STRPTR w);
void SetConfig(STRPTR w);
