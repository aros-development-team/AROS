#ifndef __AROS_STARTUP_H
#define __AROS_STARTUP_H

#include <setjmp.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>
 
extern char *__argstr;
extern ULONG __argsize;
extern char **__argv;
extern int __argc;
struct WBStartup;
extern struct WBStartup *WBenchMsg;

extern LONG __startup_error;

DECLARESET(PROGRAM_ENTRIES);
void __startup_entries_next(void);

#endif /* !__AROS_STARTUP_H */
