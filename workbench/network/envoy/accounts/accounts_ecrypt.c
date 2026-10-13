/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: accounts.library - ECrypt(): the 11-character Envoy password hash
          (re/spec/services-accounts.md §7). Test vector: user "Admin",
          password "Admin" -> "jutNGZGRdOV".
*/

#include <string.h>
#include "accounts_intern.h"

STRPTR AccECrypt(STRPTR buffer, CONST_STRPTR password, CONST_STRPTR username)
{
    LONG v[12];
    ULONG plen = password ? strlen(password) : 0;
    ULONG ulen = username ? strlen(username) : 0;
    int i, k;

    for (i = 0; i < 12; i++)
    {
        LONG p = (ULONG)i < plen ? (UBYTE)password[i] : i;
        LONG u = i;
        if ((ULONG)i < ulen)
        {
            u = (UBYTE)username[i];
            if (u >= 'A' && u <= 'Z')
                u += 'a' - 'A';
        }
        v[i] = 65 + p + u;
    }
    for (i = 0; i < 12; i++)
        for (k = 0; k < 12; k++)
            v[i] = (v[i] + v[11 - k]) % 53;
    for (i = 0; i < 11; i++)
        buffer[i] = (char)(65 + v[i]);
    buffer[11] = '\0';
    return buffer;
}
