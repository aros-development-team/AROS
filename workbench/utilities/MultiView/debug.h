#ifndef __DEBUG_H
#define __DEBUG_H

/* Debug Macros */

#ifdef _AROS

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

#else /* _AROS */

#define bug kprintf

#ifdef MYDEBUG
void kprintf(char *string, ...);
#define D(x) {kprintf("%s/%ld (%s): ", __FILE__, __LINE__, FindTask(NULL)->tc_Node.ln_Name);(x);};
#else
#define D(x) ;

#endif /* MYDEBUG */

#endif /*_AROS */

#endif /* __DEBUG_H */
