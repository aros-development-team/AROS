/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    ipv6_intern.h - Internal structures for the IPv6 protocol module.
*/

#ifndef _NETPREFS_IPV6_INTERN_H_
#define _NETPREFS_IPV6_INTERN_H_

#include <exec/libraries.h>
#include <libraries/mui.h>

#include "netprefs_intern.h"
#include "netprefs_module.h"

struct NetPrefsIPv6Base
{
    struct Library              npv6_Lib;
    struct NetPrefsBase        *npv6_NetPrefsBase;
    struct NetPrefsModule       npv6_Module;
    struct MUI_CustomClass     *npv6_WinClass;
};

#endif /* _NETPREFS_IPV6_INTERN_H_ */
