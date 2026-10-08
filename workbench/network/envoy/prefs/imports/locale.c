/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Filesystem Imports - strings. Built-in English; a catalog
          System/Prefs/Envoy/FilesystemImports.catalog is used if present.
*/

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/locale.h>

#include "locale.h"

#define CATALOG_NAME     "System/Prefs/Envoy/FilesystemImports.catalog"
#define CATALOG_VERSION  1

static const char *const builtin[MSG_COUNT] =
{
    "Filesystem Imports",
    "Host:",
    "Select Host...",
    "List Volumes",
    "Available Volumes",
    "Connect Mode:",
    "Temporary",
    "Permanent",
    "Storage",
    "Connect...",
    "Quit",
    "Login to %s",
    "Choose a host and list its volumes.",
    "Enter or select a host first.",
    "Asking %s for its volumes...",
    "%ld volume(s) on %s.",
    "No volumes on %s for this user (wrong password?).",
    "Select a volume first.",
    "Mounting %s...",
    "%s is mounted as %s:",
    "%s: is already mounted.",
    "%s: could not be mounted: %s",
    "Could not write the mount file %s.",
    "%s is not available.",
    "unknown host",
    "the host offers no Envoy filesystem service",
    "the host cannot start its filesystem service",
    "no answer from the host (timeout)",
    "the host cannot be reached",
    "the host refused the mount (wrong user or password, or no access)",
    "out of memory or network resources",
    "error %ld"
};

static struct Catalog *catalog;     /* LocaleBase comes from the auto-open code */

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
    catalog = LocaleBase ? OpenCatalog(NULL, (STRPTR)CATALOG_NAME, OC_Version, CATALOG_VERSION, TAG_DONE) : NULL;
}

VOID Locale_Deinitialize(VOID)
{
    if (LocaleBase && catalog)
        CloseCatalog(catalog);
    catalog = NULL;
}
