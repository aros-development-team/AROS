/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
 * Licensed under the AROS Public License (APL), Version 1.1. */
/* Compile the production context admission function against modeled Exec. */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
typedef void VOID;
typedef uint16_t UWORD;
typedef int BOOL;
#define TRUE 1
#define MEMF_CLEAR 1
#define MEMF_PUBLIC 0
#define SIGBREAKF_CTRL_C 4096
#define API_SHOWN 1
#define API_HIDDEN 2
struct Node { struct Node *next; };
struct List { struct Node *first; };
struct SocketBase {
    struct Node node;
    VOID *errnoPtr, *thisTask;
    int defErrno, errnoSize, sigIntrMask, *hErrnoPtr, defHErrno, res_socket;
    BOOL hookBase, timer;
    int EventLock;
    struct List EventList;
};
struct Library { UWORD lib_OpenCnt; };
static struct Library master;
static struct Library *MasterSocketBase = &master;
static struct List socketBaseList;
static int baselist_semaphore, api_state, allocations, timers;
static int fail_alloc, fail_timer, held;
static VOID *AllocMem(size_t size, int flags)
{
    (void)flags;
    if (fail_alloc) return NULL;
    VOID *p = calloc(1, size);
    assert(p);
    allocations++;
    return p;
}
static VOID FreeMem(VOID *p, size_t size)
{ (void)size; allocations--; free(p); }
static VOID *FindTask(VOID *unused) { (void)unused; return &master; }
static VOID InitSemaphore(int *s) { *s = 0; }
static VOID NewList(struct List *list) { list->first = NULL; }
static VOID ObtainSemaphore(int *s) { (void)s; assert(!held++); }
static VOID ReleaseSemaphore(int *s) { (void)s; assert(held-- == 1); }
static VOID AddTail(struct List *list, struct Node *node)
{ assert(held && !list->first); list->first = node; }
static BOOL openSleepTimer(struct SocketBase *p)
{ if (fail_timer) return 0; p->timer = 1; timers++; return 1; }
static VOID closeSleepTimer(struct SocketBase *p)
{ if (p->timer) { timers--; p->timer = 0; } }
#include "api_hookbase.c"
static VOID dispose(struct SocketBase *p)
{
    assert(socketBaseList.first == (struct Node *)p);
    socketBaseList.first = NULL;
    master.lib_OpenCnt--;
    closeSleepTimer(p);
    FreeMem(p, sizeof(*p));
}
int main(void)
{
    api_state = API_SHOWN;
    master.lib_OpenCnt = UINT16_MAX;
    assert(!api_hookbase_open());
    assert(master.lib_OpenCnt == UINT16_MAX && !socketBaseList.first);
    assert(!allocations && !timers && !held);
    master.lib_OpenCnt--;
    struct SocketBase *p = api_hookbase_open();
    assert(p && master.lib_OpenCnt == UINT16_MAX && allocations == 1 && timers == 1);
    assert(p->thisTask == &master && p->hookBase && p->res_socket == -1);
    assert(p->errnoPtr == &p->defErrno && p->hErrnoPtr == &p->defHErrno);
    dispose(p);
    master.lib_OpenCnt = 10;
    api_state = API_HIDDEN;
    p = api_hookbase_open();
    assert(p && master.lib_OpenCnt == 11);
    dispose(p);
    api_state = 0;
    assert(!api_hookbase_open() && master.lib_OpenCnt == 10);
    api_state = API_SHOWN;
    MasterSocketBase = NULL;
    assert(!api_hookbase_open());
    MasterSocketBase = &master;
    fail_alloc = 1;
    assert(!api_hookbase_open());
    fail_alloc = 0;
    fail_timer = 1;
    assert(!api_hookbase_open());
    assert(master.lib_OpenCnt == 10 && !socketBaseList.first);
    assert(!allocations && !timers && !held);
    puts("PASS actual context admission: open-count boundary, hidden/shown, absent master, allocation/timer rollback");
    return 0;
}
