/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#ifdef __AROS__
#define DEBUG 1
#include <aros/debug.h>
#else
#define kprintf(x,...)
#endif

int __abox__ = 1;

void handler(void);

void startup(void)
{ 
    kprintf("[fat] starting up\n");

	handler();
	return;
}
