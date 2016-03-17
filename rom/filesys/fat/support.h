/*
 * fat-handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007-2015 The AROS Development Team
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

void SendEvent(LONG event, struct Globals *glob);
LONG ErrorMessageArgs(struct Globals *glob, char *options, CONST_STRPTR format, ...);

#define ErrorMessage(fmt, options, ...) \
    ErrorMessageArgs(glob, options, fmt, __VA_ARGS__)

int ilog2(ULONG data);
#define log2 ilog2

#endif
