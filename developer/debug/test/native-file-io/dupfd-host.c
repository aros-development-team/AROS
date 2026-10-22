/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
   Licensed under the AROS Public License (APL), Version 1.1. */
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
typedef long LONG;
#define STDERR_FILENO 2
#define FD_OWNER_POSIXC 1
struct Library { int unused; };
typedef struct { unsigned int opencount; pthread_mutex_t lock; } fcb;
typedef struct { fcb *fcb; int fdflags; } fdesc;
struct PosixCIntBase { struct Library *PosixCFDBase; };
static struct PosixCIntBase base;
static pthread_mutex_t table_lock;
static fcb control;
static fdesc original, *table[128];
static int registry[128], fail_copy, fail_reserve, fail_grow, copies;
static struct PosixCIntBase *__aros_getbase_PosixCBase(void) { return &base; }
#define FD_LOCK(b) ((void)(b), pthread_mutex_lock(&table_lock))
#define FD_UNLOCK(b) ((void)(b), pthread_mutex_unlock(&table_lock))
static void __fcb_lock(fcb *p) { assert(!pthread_mutex_lock(&p->lock)); }
static void __fcb_unlock(fcb *p) { assert(!pthread_mutex_unlock(&p->lock)); }
static void __set_errno(struct PosixCIntBase *b, int e) { (void)b; errno=e; }
static fdesc *__getfdesc(int fd) { return fd>=0 && fd<128 ? table[fd] : NULL; }
static fdesc *__alloc_fdesc(void) { if(fail_copy) return NULL; ++copies; return calloc(1,sizeof(fdesc)); }
static void __free_fdesc(fdesc *p) { --copies; free(p); }
static int __getfirstfd(int first) { while(first<128 && table[first]) ++first; return first; }
static int __fdlib_available(struct PosixCIntBase *b) { (void)b; return 1; }
static LONG test_alloc(LONG start, LONG *out) {
    if(fail_reserve) return EMFILE;
    while(start<128 && registry[start]) ++start;
    if(start>=128) return EMFILE;
    registry[start]=1; *out=start; return 0;
}
#define FD_Alloc(start,owner,data,out) ((void)(FDBase), test_alloc((start),(out)))
static LONG test_free(int fd) { assert(fd>=3 && fd<128 && registry[fd]); registry[fd]=0; return 0; }
#define FD_Free(fd,owner) ((void)(FDBase), test_free(fd))
static int __getfdslot(int slot) { assert(slot>=0 && slot<128 && !table[slot]); return fail_grow ? -1 : slot; }
static void __setfdesc(int fd,fdesc *p) { assert(!table[fd]); table[fd]=p; }
#include "production-dupfd.c"
static void release_copy(int fd) {
    assert(!pthread_mutex_lock(&table_lock));
    assert(table[fd] && table[fd]!=&original);
    __fcb_lock(&control); assert(control.opencount>1); --control.opencount;
    __fcb_unlock(&control); __free_fdesc(table[fd]); table[fd]=NULL;
    if(fd>=3) test_free(fd);
    assert(!pthread_mutex_unlock(&table_lock));
}
static void *worker(void *unused) {
    (void)unused;
    for(int i=0;i<256;++i) {
        int fd=__dupfd(7,20,FD_CLOEXEC); assert(fd>=20);
        assert(table[fd]->fdflags==FD_CLOEXEC && table[fd]->fcb==&control);
        release_copy(fd);
    }
    return NULL;
}
int main(void) {
    pthread_mutexattr_t attr; assert(!pthread_mutexattr_init(&attr));
    assert(!pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE));
    assert(!pthread_mutex_init(&table_lock,&attr));
    assert(!pthread_mutex_init(&control.lock,&attr)); pthread_mutexattr_destroy(&attr);
    original.fcb=&control; original.fdflags=FD_CLOEXEC; control.opencount=1;
    table[7]=&original; registry[7]=1;
    assert(__dupfd(-1,0,0)==-1 && errno==EBADF);
    assert(__dupfd(7,-1,0)==-1 && errno==EINVAL);
    assert(__dupfd(7,INT_MAX,0)==-1 && errno==EINVAL);
    assert(__dupfd(7,0,~FD_CLOEXEC)==-1 && errno==EINVAL);
    fail_copy=1; assert(__dupfd(7,20,0)==-1 && errno==ENOMEM); fail_copy=0;
    fail_reserve=1; assert(__dupfd(7,20,0)==-1 && errno==EMFILE); fail_reserve=0;
    fail_grow=1; assert(__dupfd(7,20,0)==-1 && errno==ENOMEM && !registry[20]); fail_grow=0;
    control.opencount=UINT_MAX;
    assert(__dupfd(7,20,0)==-1 && errno==EOVERFLOW); control.opencount=1;
    registry[20]=1; int fd=__dupfd(7,20,FD_CLOEXEC);
    assert(fd==21 && original.fdflags==FD_CLOEXEC); release_copy(fd); registry[20]=0;
    fd=__dupfd(7,0,0); assert(fd==0 && table[fd]->fdflags==0); release_copy(fd);
    pthread_t threads[8];
    for(int i=0;i<8;++i) assert(!pthread_create(&threads[i],NULL,worker,NULL));
    for(int i=0;i<8;++i) assert(!pthread_join(threads[i],NULL));
    assert(!copies && control.opencount==1);
    for(int i=0;i<128;++i) assert(registry[i]==(i==7));
    pthread_mutex_destroy(&control.lock); pthread_mutex_destroy(&table_lock);
    return 0;
}
