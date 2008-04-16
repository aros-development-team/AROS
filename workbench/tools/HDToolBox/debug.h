#ifndef PDEBUG_H
#define PDEBUG_H

#ifndef DEBUG
#define DEBUG 1
#endif

#ifndef __AROS__
#ifdef __AMIGAOS__
#if DEBUG > 0
#define D(x) x
#define bug kprintf
#else
#define D(x)
#endif
void kprintf(char *, ...);
#endif
#else /* __AROS__ */
#include <aros/debug.h>
#endif

#endif /* PDEBUG_H */
