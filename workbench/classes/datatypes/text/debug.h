#ifndef __DEBUG_H
#define __DEBUG_H

/* Debug Macros */

#if defined(__AROS__) && !defined(__MORPHOS__)

#undef DEBUG

#ifdef MYDEBUG
#define DEBUG 1
#else
#define DEBUG 0
#endif
#include <aros/debug.h>

#else /* __AROS__ */

#undef bug
#undef D(x)
#undef kprintf

#define bug kprintf

#ifdef MYDEBUG
void kprintf(char *string, ...);
#define D(x) {/*bug("%s/%ld (%s): ", __FILE__, __LINE__, FindTask(NULL)->tc_Node.ln_Name);*/ {(x);}};
#else
#define D(x) ;

#endif /* MYDEBUG */

#endif /*__AROS__ */

#endif /* __DEBUG_H */
