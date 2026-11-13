/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Envoy Services preferences - strings.
*/

#include <exec/types.h>
#include <proto/locale.h>

#include "locale.h"

#define CATALOG_NAME    "System/Prefs/Envoy/Services.catalog"
#define CATALOG_VERSION 1

static const CONST_STRPTR builtin[MSG_COUNT] =
{
    "Envoy Services",
    "Configures the network services offered by this host",
    "Services offered to other hosts",
    "Service",
    "Library",
    "Active",
    "Add",
    "Remove",
    "Service name:",
    "Library:",
    "Active:",
    "Start the Envoy servers at boot",
    "New Service",
    "Yes",
    "No",
    "Select the service library"
};

static struct Catalog *catalog;

CONST_STRPTR _(ULONG id)
{
    if (id >= MSG_COUNT)
        return "";
    if (LocaleBase && catalog)
        return GetCatalogStr(catalog, id, builtin[id]);
    return builtin[id];
}

VOID Locale_Initialize(VOID)
{
    catalog = LocaleBase ? OpenCatalog(NULL, CATALOG_NAME, OC_Version, CATALOG_VERSION, TAG_DONE) : NULL;
}

VOID Locale_Deinitialize(VOID)
{
    if (LocaleBase && catalog)
        CloseCatalog(catalog);
}
