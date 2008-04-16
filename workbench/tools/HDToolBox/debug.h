#ifndef PDEBUG_H
#define PDEBUG_H

#ifndef __AROS__
#ifdef __AMIGAOS__
#if DEBUG
#define D(x) x
#define bug kprintf
#endif
void kprintf(char *, ...);
#endif
#else /* __AROS__ */
#ifndef DEBUG
#define DEBUG 0
#endif
#include <aros/debug.h>
#endif

#endif /* PDEBUG_H */
