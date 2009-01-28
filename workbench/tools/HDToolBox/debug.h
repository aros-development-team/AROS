#ifndef PDEBUG_H
#define PDEBUG_H

// AROS: Comment the following line out to enable debug to the shell
#define AROS_DEFAULT_DEBUG

#ifndef DEBUG
#define DEBUG 0
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
#ifndef AROS_DEFAULT_DEBUG
#define D(x) x
#define bug printf
#include <stdio.h>
#else
#include <aros/debug.h>
#endif
#endif

#endif /* PDEBUG_H */
