/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */

#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <exec/libraries.h>
#include <exec/types.h>
#include <proto/exec.h>

#include <string.h>

#include <libraries/aros_resource.h>
#include <libraries/speechcore.h>
#include "translator_intern.h"

#include LC_LIBDEFS_FILE

#define RESOURCE_PATH "DEVS:Speech/speech.iff"

static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR base)
{
    base->tb_DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 0);
    memset(&base->tb_Resource, 0, sizeof(base->tb_Resource));
    base->tb_ResourceMemory = LoadSpeechResource(base->tb_DOSBase,
                                                    RESOURCE_PATH,
                                                    &base->tb_Resource);
    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)

AROS_LH4(LONG, Translate,
    AROS_LHA(CONST_STRPTR, inputString, A0),
    AROS_LHA(LONG, inputLength, D0),
    AROS_LHA(STRPTR, outputBuffer, A1),
    AROS_LHA(LONG, outputLength, D1),
    struct TranslatorBase *, TranslatorBase, 5, Translator)
{
    AROS_LIBFUNC_INIT

    if (inputLength < 0 || outputLength <= 0 || outputBuffer == NULL ||
        (inputString == NULL && inputLength != 0))
        return -1;

    return SCTranslateWith(TranslatorBase->tb_Resource.translator != NULL
            ? TranslatorBase->tb_Resource.translator : &SCDefaultTranslator,
        inputString, (size_t)inputLength,
        outputBuffer, (size_t)outputLength);

    AROS_LIBFUNC_EXIT
}
