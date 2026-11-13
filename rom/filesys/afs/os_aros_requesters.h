/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Shared implementation for the inline and external requester wrappers.
    Include after os_aros_support.h with AFS_REQUESTER_LINKAGE selected.
*/

AFS_REQUESTER_LINKAGE LONG showText(struct AFSBase *afsbase, const char *format, ...)
{
    LONG ret;

    AROS_SLOWSTACKFORMAT_PRE(format);
    ret = showPtrArgsText(afsbase, format, Req_Cancel, AROS_SLOWSTACKFORMAT_ARG(format));
    AROS_SLOWSTACKFORMAT_POST(format);

    return ret;
}

AFS_REQUESTER_LINKAGE LONG showRetriableError(struct AFSBase *afsbase, const char *format, ...)
{
    LONG ret;

    AROS_SLOWSTACKFORMAT_PRE(format);
    ret = showPtrArgsText(afsbase, format, Req_RetryCancel, AROS_SLOWSTACKFORMAT_ARG(format));
    AROS_SLOWSTACKFORMAT_POST(format);

    return ret;
}
