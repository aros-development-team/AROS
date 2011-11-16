/*
 * Copyright (C) 2011, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <aros/config.h>

void DebugInit(void);
int DebugPutChar(register int chr);
int DebugMayGetChar(void);

#if AROS_SERIAL_DEBUG
void DebugPutStr(register const char *buff);
void DebugPutHex(const char *what, ULONG val);
void DebugPutDec(const char *what, ULONG val);
void DebugPutHexVal(ULONG val);

#define DEBUGPUTS(x) do { DebugPutStr x; } while(0)
#define DEBUGPUTD(x) do { DebugPutDec x; } while(0)
#define DEBUGPUTHEX(x) do { DebugPutHex x; } while(0)
#else
#define DEBUGPUTS(x) do { } while (0)
#define DEBUGPUTD(x) do { } while (0)
#define DEBUGPUTHEX(x) do { } while (0)
#endif

#endif /* DEBUG_H */
