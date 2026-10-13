/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Filesystem Exports - strings. Built-in English; a catalog
          System/Prefs/Envoy/FilesystemExports.catalog is used if present.
*/

#include <exec/types.h>
#include <proto/locale.h>

#include "locale.h"

#define CATALOG_NAME     "System/Prefs/Envoy/FilesystemExports.catalog"
#define CATALOG_VERSION  1

static const char *const builtin[MSG_COUNT] =
{
    "Filesystem Exports",
    "Exports",
    "Add",
    "Remove",
    "Export name:",
    "Local path:",
    "Security:",
    "Mounts",
    "Mounts & Files",
    "None",
    "Read only:",
    "Removable medium:",
    "Emulate ExAll():",
    "Allow volume snapshot:",
    "Allow left-out icons:",
    "Users and groups allowed to mount",
    "Add...",
    "Remove",
    "User",
    "Group",
    "New",
    "(no name)",
    "Allow access for",
    "OK",
    "accounts.library is not available: users and groups are shown by number.",
    "envoy.library is not available.",
    "Mounts: users in the list may mount; files are not checked.\n"
    "Mounts & Files: as Mounts, and every file access is checked\n"
    "against the file's owner and protection bits.\n"
    "None: anyone may mount, without a login."
};

static struct Catalog *catalog;

CONST_STRPTR _(ULONG id)
{
    if (id >= MSG_COUNT)
        return "";
    if (LocaleBase != NULL && catalog != NULL)
        return GetCatalogStr(catalog, id, builtin[id]);
    return builtin[id];
}

VOID Locale_Initialize(VOID)
{
    catalog = LocaleBase ? OpenCatalog(NULL, CATALOG_NAME, OC_Version, CATALOG_VERSION, TAG_DONE) : NULL;
}

VOID Locale_Deinitialize(VOID)
{
    if (LocaleBase != NULL && catalog != NULL)
        CloseCatalog(catalog);
    catalog = NULL;
}
