/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    ipv4_intern.h - Internal structures for the IPv4 protocol module.
*/

#ifndef _NETPREFS_IPV4_INTERN_H_
#define _NETPREFS_IPV4_INTERN_H_

#include <exec/libraries.h>
#include <libraries/mui.h>

#include "netprefs_intern.h"
#include "netprefs_module.h"

struct NetPrefsIPv4Base
{
    struct Library              npv4_Lib;
    struct NetPrefsBase        *npv4_NetPrefsBase;
    struct NetPrefsModule       npv4_Module;
    struct MUI_CustomClass     *npv4_WinClass;
};

#endif /* _NETPREFS_IPV4_INTERN_H_ */
