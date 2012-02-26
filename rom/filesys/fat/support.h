/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2008 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#ifndef FAT_HANDLER_SUPPORT_H
#define FAT_HANDLER_SUPPORT_H

#include <exec/types.h>
#include <dos/dosextens.h>

void SendEvent(LONG event);
void ErrorMessageArgs(char *fmt, IPTR *args);

#define ErrorMessage(fmt, ...)		\
{					\
    IPTR __args[] = {__VA_ARGS__};	\
    ErrorMessageArgs(fmt, __args);	\
}

int ilog2(ULONG data);
#define log2 ilog2

#endif

