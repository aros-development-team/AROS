#!/usr/bin/env python3
"""Compile the actual duplicate helper against a deterministic table harness.
A competing global owner takes the selected slot immediately before reserve;
lookup/publication mocks also require the spanning table guard.
"""
from pathlib import Path
import subprocess, tempfile
root = Path(__file__).resolve().parents[5]
source = (root / "compiler/crt/posixc/__fdesc.c").read_text()
helper = source[source.index("static int __reserve_freefd("):source.index("int __getfdslot(")]
shim = r'''#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
typedef struct {int refs;} fcb;
typedef struct {int fdflags;fcb *fcb;} fdesc;
struct Library {int unused;};
struct PosixCIntBase {struct Library *PosixCFDBase;fdesc **array;int slots;};
static struct Library lib;
static struct PosixCIntBase base;
static int held, occupied[128], inject=1, fail_alloc, reserves, releases;
static fcb object={1};
static fdesc original={1,&object};
#define FD_OWNER_POSIXC 1
#define FD_LOCK(b) do {assert(!held);held=1;} while(0)
#define FD_UNLOCK(b) do {assert(held);held=0;} while(0)
#define FD_SLOTS(b) ((b)->slots)
#define FD_ARRAY(b) ((b)->array)
#define FD_POOL(b) (b)
static void *__aros_getbase_PosixCBase(void){return &base;}
static fdesc *__getfdesc(int fd){assert(held);return fd>=0&&fd<base.slots?base.array[fd]:NULL;}
struct fd_hooks;
static const struct fd_hooks *__getfdhooks(int fd,void *p){(void)fd;(void)p;return NULL;}
static int __getfirstfd(int minimum){return minimum;}
static int __fdlib_available(void *b){(void)b;return 1;}
#define FD_Reserve(fd,owner,data) reserve(fd)
static int reserve(int fd){assert(held);if(inject){occupied[fd]=1;inject=0;}if(occupied[fd])return EBUSY;occupied[fd]=1;reserves++;return 0;}
#define FD_Free(fd,owner) release(fd)
static void release(int fd){assert(held);assert(occupied[fd]);occupied[fd]=0;releases++;}
static void *AllocPooled(void *pool,size_t n){(void)pool;assert(held);return fail_alloc?NULL:calloc(1,n);}
#define CopyMem(a,b,n) memcpy(b,a,n)
#define FreePooled(pool,p,n) free(p)
static fdesc *__alloc_fdesc(void){assert(held);return calloc(1,sizeof(fdesc));}
static void __free_fdesc(fdesc *f){assert(held);free(f);}
static void __fcb_addref(fcb *f){assert(held);f->refs++;}
static void __setfdesc(int fd,fdesc *f){assert(held);assert(!base.array[fd]);assert(f->fdflags==0||f->fdflags==1);base.array[fd]=f;}
static void __set_errno(void *b,int e){(void)b;errno=e;}
'''
main = r'''int main(void){
 base.PosixCFDBase=&lib;base.slots=16;base.array=calloc(16,sizeof(fdesc*));base.array[3]=&original;occupied[3]=1;
 int fd=__dupfd(3,8,1);assert(fd==9);assert(occupied[8]);assert(base.array[8]==NULL);assert(base.array[9]->fdflags==1);assert(original.fdflags==1);assert(object.refs==2);assert(!held);
 fd=__dupfd(3,10,0);assert(fd==10);assert(base.array[10]->fdflags==0);
 fail_alloc=1;int before=reserves;errno=0;assert(__dupfd(3,20,1)==-1);assert(errno==ENOMEM);assert(reserves==before+1);assert(releases==1);assert(!occupied[20]);assert(!held);
 errno=0;assert(__dupfd(3,-1,1)==-1&&errno==EINVAL);assert(!held);
 fail_alloc=0;occupied[0]=1;int oldres=reserves;fd=__dupfd(3,0,1);assert(fd==0);assert(reserves==oldres);assert(base.array[0]->fdflags==1);
 fdesc *pipe_end=calloc(1,sizeof(fdesc));pipe_end->fcb=&object;pipe_end->fdflags=0;inject=1;
 fd=__publish_newfd(pipe_end,11);assert(fd==12);assert(occupied[11]);assert(base.array[11]==NULL);assert(base.array[12]==pipe_end);assert(!held);
 puts("atomic duplicate/publish helpers: interleaved reservation, guarded publication, flags and rollback passed");return 0;}
'''
with tempfile.TemporaryDirectory(prefix="aros-atomic-dup-") as tmp:
    c=Path(tmp)/"test.c";exe=Path(tmp)/"test"
    c.write_text(shim+helper+main)
    subprocess.run(["cc","-std=c99","-Wall","-Wextra","-Werror",str(c),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
