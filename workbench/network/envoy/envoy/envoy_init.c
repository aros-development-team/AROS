/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: envoy.library initialisation. As in the original, the library
          needs dos, utility and intuition; locale and the catalog are
          optional. Zune, gadtools, nipc and accounts are opened per call.
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/locale.h>

#include "envoy_intern.h"

static int Envoy_Init(struct EnvoyBase *EnvoyBase)
{
    InitSemaphore(&EnvoyBase->eb_Sem);
    if (!(EnvoyBase->eb_DOSBase = OpenLibrary("dos.library", 36)))
        return FALSE;
    if (!(EnvoyBase->eb_UtilityBase = OpenLibrary("utility.library", 36)))
        return FALSE;
    if (!(EnvoyBase->eb_IntuitionBase = OpenLibrary("intuition.library", 36)))
        return FALSE;
    if ((EnvoyBase->eb_LocaleBase = OpenLibrary("locale.library", 36)))
    {
        struct TagItem tags[] = { { OC_BuiltInLanguage, (IPTR)"english" }, { TAG_DONE, 0 } };
        EnvoyBase->eb_Catalog = OpenCatalogA(NULL, "Sys/envoyprefs.catalog", tags);
    }
    return TRUE;
}

static int Envoy_Expunge(struct EnvoyBase *EnvoyBase)
{
    if (EnvoyBase->eb_LocaleBase)
    {
        if (EnvoyBase->eb_Catalog)
            CloseCatalog(EnvoyBase->eb_Catalog);
        CloseLibrary(EnvoyBase->eb_LocaleBase);
    }
    if (EnvoyBase->eb_IntuitionBase)
        CloseLibrary(EnvoyBase->eb_IntuitionBase);
    if (EnvoyBase->eb_UtilityBase)
        CloseLibrary(EnvoyBase->eb_UtilityBase);
    if (EnvoyBase->eb_DOSBase)
        CloseLibrary(EnvoyBase->eb_DOSBase);
    return TRUE;
}

ADD2INITLIB(Envoy_Init, 0)
ADD2EXPUNGELIB(Envoy_Expunge, 0)
