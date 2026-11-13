#ifndef __AROS_STARTUP_H
#define __AROS_STARTUP_H

#include <setjmp.h>
#include <proto/exec.h>
#include <aros/symbolsets.h>

/* The AROS startup support code.

   The following variables are made available to programs by the AROS startup
   code.
   There is also a PROGRAM_ENTRIES symbolset that allows to register a function
   that can be used to do initialization before the main() function is called
   and clean-up after it has run.
   This PROGRAM_ENTRIES set is ignored by the startup code of shared libraries
   and other modules. So if your code uses this PROGRAM_ENTRIES set and can be
   linked in modules one should handle the case of a non-executed
   PROGRAM_ENTRIES function gracefully.
*/
extern char *__argstr;
extern ULONG __argsize;
extern char **__argv;
extern int __argc;
struct WBStartup;
extern struct WBStartup *WBenchMsg;

extern LONG __startup_error;

DECLARESET(PROGRAM_ENTRIES);
#define __startup_entries_next() ___startup_entries_next(SysBase)
void ___startup_entries_next(struct ExecBase *SysBase);

#endif /* !__AROS_STARTUP_H */
