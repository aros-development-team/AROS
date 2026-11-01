#!/usr/bin/env python3
"""Compile actual stdio sources against host shims; reject early errno access.

Run with an optional source-tree path. Before the fix mask1 aborts inside
fdopen; after the fix all eight masks initialize three FILE wrappers without
accessing errno, while public fdopen(-1) still reports EBADF when ready.
"""
from pathlib import Path
import re, subprocess, tempfile, sys
r=Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[5]
prefix=r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#undef errno
struct TestFile { int flags, fd; };
#define FILE struct TestFile
#define fdopen aros_fdopen
struct List { int nodes; };
struct Node { int unused; };
typedef struct { struct Node Node; FILE File; } FILENODE;
#define FILENODE2FILE(fn) (&(fn)->File)
#define __POSIXC_STDIO_EOF 1
#define __POSIXC_STDIO_ERROR 2
#define __POSIXC_STDIO_WRITE 4
#define __POSIXC_STDIO_READ 8
#define __POSIXC_STDIO_APPEND 16
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define ERROR_NO_FREE_STORE 103
#define ADD2OPENLIB(a,b)
struct PosixCBase { FILE *_stdin, *_stdout, *_stderr; };
struct PosixCIntBase { struct PosixCBase pub; void *internalpool; struct List stdio_files; };
typedef struct { int flags; } fcb;
typedef struct { fcb *fcb; } fdesc;
static struct PosixCIntBase base;
static fcb blocks[3] = {{O_RDONLY}, {O_WRONLY|O_APPEND}, {O_WRONLY|O_APPEND}};
static fdesc desc[3] = {{blocks}, {blocks+1}, {blocks+2}};
static int mask, ready, error;
static int *errno_access(void) { assert(ready); return &error; }
#define errno (*errno_access())
static void *__aros_getbase_PosixCBase(void) { return &base; }
static fdesc *__getfdesc(int fd) { return fd >= 0 && fd < 3 && !(mask & (1<<fd)) ? &desc[fd] : NULL; }
static void *AllocPooled(void *pool, size_t size) { (void)pool; return calloc(1,size); }
static void AddTail(struct List *list, struct Node *node) { (void)node; list->nodes++; }
#define NEWLIST(list) ((list)->nodes=0)
static void SetIoErr(int e) { (void)e; }
FILE *aros_fdopen(int, const char *);
FILE *__stdio_file(int, int);

int __smode2oflags(const char *);
int __oflags2sflags(int);
'''
source=''
for name in ['fdopen.c','__stdio.c']:
    t=(r/'compiler/crt/posixc'/name).read_text(errors='surrogateescape')
    source+=re.sub(r'^\s*#include[^\n]*', '', t, flags=re.M)+'\n'
main=r'''
int main(void) {
  for(mask=0; mask<8; mask++) {
    memset(&base,0,sizeof(base)); ready=0;
    assert(__init_stdio(&base)==1);
    assert(base.stdio_files.nodes==3);
    assert(base.pub._stdin && base.pub._stdin->fd==0 && (base.pub._stdin->flags&8));
    assert(base.pub._stdout && base.pub._stdout->fd==1 && (base.pub._stdout->flags&4));
    assert(base.pub._stderr && base.pub._stderr->fd==2 && (base.pub._stderr->flags&4));
    ready=1; error=0; assert(aros_fdopen(-1,"r")==NULL && error==EBADF);
    free((char*)base.pub._stdin - __builtin_offsetof(FILENODE,File));
    free((char*)base.pub._stdout - __builtin_offsetof(FILENODE,File));
    free((char*)base.pub._stderr - __builtin_offsetof(FILENODE,File));
  }
  return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    c=Path(d)/'test.c'; c.write_text(prefix+source+main, errors='surrogateescape')
    subprocess.run(['cc','-std=c99','-Wall','-Wextra',str(c),'-o',str(Path(d)/'test')],check=True)
    x=subprocess.run([str(Path(d)/'test')]); print('actual stdio source result',x.returncode); sys.exit(x.returncode != 0)
