#pragma once
#include <cstdlib>
#include <cstring>
struct aros_alloc_t { void *p; template<class T> operator T*() const { return (T*)p; } };
#define AllocMem(size, flags) (aros_alloc_t{calloc(1, size)})
#define FreeMem(ptr, size) free(ptr)
#define AllocVec(size, flags) (aros_alloc_t{calloc(1, size)})
#define FreeVec(ptr) free(ptr)
#define CopyMem(src, dst, n) memcpy(dst, src, n)
#include <exec/libraries.h>
#include <exec/ports.h>
struct ExecBase { struct Library LibNode; };
extern struct ExecBase *SysBase_stub;
#define SysBase SysBase_stub
#define NEWLIST(l) do { struct List *_l = (struct List *)(l); _l->lh_Head = (struct Node *)(void *)&_l->lh_Tail; _l->lh_Tail = 0; _l->lh_TailPred = (struct Node *)(void *)&_l->lh_Head; } while (0)
#define IsListEmpty(l) (((struct List *)(l))->lh_TailPred == (struct Node *)(l))
static inline struct Node *RemHead(struct List *l) { struct Node *n = l->lh_Head; if (!n->ln_Succ) return 0; l->lh_Head = n->ln_Succ; n->ln_Succ->ln_Pred = (struct Node *)(void *)&l->lh_Head; return n; }
#define DeleteIORequest(r) ((void)0)
#define CloseDevice(r) ((void)0)
#define CACRF_EnableI 1
#define CACRF_EnableD 2
#define CACRF_CopyBack 4
#define OpenLibrary(name, ver) ((struct Library *)0)
#define CloseLibrary(lib) ((void)0)
#define GetVar(name, buf, len, flags) (-1)
#define IsMinListEmpty(l) (((struct MinList *)(l))->mlh_TailPred == (struct MinNode *)(l))
