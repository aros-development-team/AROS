/*
** Debugging macros for the Bluetooth prefs (mirrors the Trident debug.h idea,
** kept minimal).
*/

#ifndef PREFS_DEBUG_H
#define PREFS_DEBUG_H

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#include <proto/exec.h>
#include <proto/debug.h>
#define D(x)    x
#define bug     KPrintF
#else
#define D(x)
#endif

#endif /* PREFS_DEBUG_H */
