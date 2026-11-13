/* Host stand-in for posixc's private base: only the fields __optionallibs.c
   touches, and the base accessors the library normally gets from genmodule. */
#ifndef __POSIXC_INTBASE_H
#define __POSIXC_INTBASE_H
#include <proto/exec.h>
struct PosixCBase { struct Library lib; };
struct PosixCIntBase {
    struct PosixCBase PosixCBase;
    struct Library   *PosixCUserGroupBase;
    void             *PosixCEntropyBase;
    struct Library   *PosixCFDBase;
    struct Library   *PosixCDOS64Base;
};
void *__aros_getbase_PosixCBase(void);
void *__aros_getbase_StdCBase(void);
#endif
