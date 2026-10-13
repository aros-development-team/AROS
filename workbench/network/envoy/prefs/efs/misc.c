/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include "locale.h"
#include "misc.h"

VOID ShowMessage(CONST_STRPTR msg)
{
    struct EasyStruct es;

    if (!msg)
        return;
    if (Cli())
    {
        PutStr(_(MSG_WINTITLE));
        PutStr(": ");
        PutStr(msg);
        PutStr("\n");
        return;
    }
    es.es_StructSize   = sizeof(es);
    es.es_Flags        = 0;
    es.es_Title        = _(MSG_WINTITLE);
    es.es_TextFormat   = msg;
    es.es_GadgetFormat = _(MSG_OK);
    EasyRequestArgs(NULL, &es, NULL, NULL);
}
