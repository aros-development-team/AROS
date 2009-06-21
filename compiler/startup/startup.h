#ifndef __AROS_STARTUP_H
#define __AROS_STARTUP_H

#include <setjmp.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>
 
extern char *__argstr;
extern ULONG __argsize;
extern char **__argv;
extern int argc;

struct aros_startup
{
    jmp_buf as_startup_jmp_buf;
    LONG    as_startup_error;
};

extern struct aros_startup __aros_startup;

DECLARESET(PROGRAM_ENTRIES);
void __startup_entries_next(void);

#endif /* !__AROS_STARTUP_H */
