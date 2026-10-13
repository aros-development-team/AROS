/* Host stand-in for stdc's private base: the exit lists only. */
#ifndef __STDC_INTBASE_H
#define __STDC_INTBASE_H
#include <exec/lists.h>
struct StdCIntBase {
    struct MinList atexit_list;
    struct MinList quick_exit_list;
};
int *__stdc_get_errorptr(void);
void *__aros_getbase_StdCBase(void);
#endif
