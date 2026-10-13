/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: envoy.library - the built-in English strings and the catalog
          lookup (re/spec/envoy-library.md §10).
*/

#include <proto/locale.h>

#include "envoy_intern.h"

static const struct { ULONG id; CONST_STRPTR text; } builtin[] =
{
    { MSG_CANCEL_GAD,            "Cancel" },
    { MSG_OK_GAD,                "OK" },
    { MSG_ELIB_HOST,             "Host:" },
    { MSG_ELIB_HOST_REQUEST,     "Host Request" },
    { MSG_ELIB_USERNAME,         "Name:" },
    { MSG_ELIB_PASSWORD,         "Password:" },
    { MSG_ELIB_LOGIN_REQUEST,    "Login Request" },
    { MSG_ELIB_GROUPTAG,         " <Group>" },
    { MSG_ELIB_GROUPLISTFMT,     "%%-%ld.%lds <Group>" },
    { MSG_ELIB_USER_REQUEST,     "User Request" },
    { MSG_ELIB_REALMS,           "Realms" },
    { MSG_ELIB_PASSWORD_REQUEST, "Password Request" },
    { MSG_ELIB_OLDPASSWORD,      "Old Password:" },
    { MSG_ELIB_PASSWORD1,        "New Password:" },
    { MSG_ELIB_PASSWORD2,        "Repeat it:" },
    { 0, NULL }
};

CONST_STRPTR EnvoyStr(struct EnvoyBase *EnvoyBase, ULONG id)
{
    CONST_STRPTR def = "";
    int i;

    for (i = 0; builtin[i].text; i++)
    {
        if (builtin[i].id == id)
        {
            def = builtin[i].text;
            break;
        }
    }
    if (LocaleBase && EnvoyBase->eb_Catalog)
        return GetCatalogStr(EnvoyBase->eb_Catalog, id, def);
    return def;
}
