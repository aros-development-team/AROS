/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include "os_aros_support.h"

LONG showText(struct AFSBase *afsbase, const char *format, ...)
{
    LONG ret;

    AROS_SLOWSTACKFORMAT_PRE(format);
    ret = showPtrArgsText(afsbase, format, Req_Cancel, AROS_SLOWSTACKFORMAT_ARG(format));
    AROS_SLOWSTACKFORMAT_POST(format);

    return ret;
}

LONG showRetriableError(struct AFSBase *afsbase, const char *format, ...)
{
    LONG ret;

    AROS_SLOWSTACKFORMAT_PRE(format);
    ret = showPtrArgsText(afsbase, format, Req_RetryCancel, AROS_SLOWSTACKFORMAT_ARG(format));
    AROS_SLOWSTACKFORMAT_POST(format);

    return ret;
}

