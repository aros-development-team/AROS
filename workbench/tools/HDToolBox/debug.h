#ifndef PDEBUG_H
#define PDEBUG_H

#ifdef __AMIGAOS__
#if DEBUG
#	define D(x) x
#	define bug kprintf
#endif
void kprintf(char *, ...);
#else
#include <aros/debug.h>
#endif

#endif

