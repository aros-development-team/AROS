/* Host stand-in for <proto/exec.h>: the exec calls __optionallibs.c makes,
   provided by the test so that each one can be observed or made to fail. */
#ifndef PROTO_EXEC_H
#define PROTO_EXEC_H
#include <exec/lists.h>
#define TRUE 1
struct Library {
    struct Node lib_Node;
    unsigned short lib_OpenCnt;
};
struct ExecBase { struct List LibList; };
extern struct ExecBase *SysBase;
struct Library *OpenLibrary(const char *name, unsigned long version);
void CloseLibrary(struct Library *lib);
void *OpenResource(const char *name);
void Forbid(void);
void Permit(void);
struct Node *FindName(struct List *list, const char *name);
/* The symbol set macro registers a CLOSELIB handler in AROS; the test calls
   __optionallibs_close directly instead. */
#define ADD2CLOSELIB(func, pri)
#endif
