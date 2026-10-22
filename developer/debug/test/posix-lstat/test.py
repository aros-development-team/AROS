#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Distributed under the AROS Public License 1.1.
from pathlib import Path
import subprocess,json,re,argparse
HERE=Path(__file__).resolve().parent;CORE=HERE.parents[3]
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--before-ref',default='HEAD^');a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);root=CORE/'compiler/crt/posixc'
strip=lambda s: '\n'.join(l for l in s.splitlines() if not l.startswith('#include'))
mock=r'''
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
typedef uintptr_t BPTR; typedef uintptr_t BSTR; typedef intptr_t SIPTR; typedef intptr_t IPTR; typedef int32_t LONG;
#define FALSE 0
#define BNULL 0
#define SHARED_LOCK -2
#define MEMF_ANY 0
#define ERROR_IS_SOFT_LINK 233
#define ERROR_OBJECT_IN_USE 202
#define ERROR_OBJECT_NOT_FOUND 205
#define ERROR_NO_FREE_STORE 103
#define ERROR_LINE_TOO_LONG 120
#define ACTION_LOCATE_OBJECT 8
#ifdef AROS_FAST_BSTR
#define BADDR(p) ((void*)(p))
#define MKBADDR(p) ((uintptr_t)(p))
#else
#define BADDR(p) ((void*)((uintptr_t)(p)<<2))
#define MKBADDR(p) ((uintptr_t)(p)>>2)
#endif
struct FileLock {void *fl_Task;}; static struct FileLock parent_obj={ (void*)17 },file_obj;
struct stat {int mode;};struct stat64 {int mode;};
struct PosixCIntBase {int doupath;};static struct PosixCIntBase base={1};
static void *__aros_getbase_PosixCBase(void){return &base;}
static LONG io;static int allocations,locks,packets,fail_alloc; static char last_parent[128],last_leaf[128];
static LONG IoErr(void){return io;}static void SetIoErr(LONG e){io=e;}
static const char *__path_u2a(const char*p){if(!p){errno=EFAULT;return NULL;}return p;}
static const char *FilePart(const char*p){const char*r=p;for(;*p;p++)if(*p=='/'||*p==':')r=p+1;return r;}
static void *AllocVec(size_t n,int flags){(void)flags;if(fail_alloc)return NULL;void*p=malloc(n);if(p)allocations++;return p;}
static void FreeVec(void*p){if(p){allocations--;free(p);}io=999;}
static BPTR Lock(const char*p,LONG mode){(void)mode;snprintf(last_parent,sizeof(last_parent),"%s",p);
if(strstr(p,"missing-parent")){io=ERROR_OBJECT_NOT_FOUND;return 0;}
if(strstr(p,"dangling")){io=ERROR_OBJECT_NOT_FOUND;return 0;}
locks++;io=0;return MKBADDR(strstr(p,"valid-link")?&file_obj:&parent_obj);}
static void UnLock(BPTR p){assert(p);locks--;io=998;}
static BPTR DoPkt(void*port,LONG action,BPTR parent,SIPTR name,LONG mode,LONG z,LONG zz){
(void)mode;(void)z;(void)zz;assert(port==(void*)17&&action==ACTION_LOCATE_OBJECT&&parent);
#ifdef AROS_FAST_BSTR
const char*leaf=(const char*)(uintptr_t)name;
#else
const char*leaf=(const char*)BADDR(name)+1;
#endif
snprintf(last_leaf,sizeof(last_leaf),"%s",leaf);packets++;
if(strstr(leaf,"link")){io=ERROR_IS_SOFT_LINK;return 0;}
if(!strcmp(leaf,"missing")){io=ERROR_OBJECT_NOT_FOUND;return 0;}
locks++;io=0;return MKBADDR(&file_obj);}
static int __stdc_ioerr2errno(LONG e){return e==ERROR_NO_FREE_STORE?ENOMEM:e==ERROR_OBJECT_NOT_FOUND?ENOENT:EIO;}
static int __stat(BPTR l,struct stat*s,int h){(void)l;(void)h;s->mode=1;return 0;}
static int __stat64(BPTR l,struct stat64*s,int h){(void)l;(void)h;s->mode=1;return 0;}
static int __stat_from_path(const char*p,struct stat*s){assert(strstr(p,"link"));s->mode=2;return 0;}
static int __stat64_from_path(const char*p,struct stat64*s){assert(strstr(p,"link"));s->mode=2;return 0;}
'''
tail=r'''
int main(void){struct stat s;struct stat64 t;
const char*names[]={"RAM:valid-link","RAM:dangling-link","ASSIGN:parent-link/valid-link","RAM:raw-\xe9-link","valid-link"};
for(unsigned i=0;i<sizeof(names)/sizeof(*names);i++){assert(lstat(names[i],&s)==0&&s.mode==2);assert(lstat64(names[i],&t)==0&&t.mode==2);assert(!locks&&!allocations);}
assert(lstat("RAM:ordinary",&s)==0&&s.mode==1);assert(!strcmp(last_parent,"RAM:"));assert(!strcmp(last_leaf,"ordinary"));
assert(lstat("RAM:",&s)==0&&s.mode==1);assert(lstat64("",&t)==-1&&errno==ENOENT);assert(lstat(NULL,&s)==-1&&errno==EFAULT);
assert(lstat("RAM:missing",&s)==-1&&errno==ENOENT);assert(lstat("RAM:missing-parent/entry",&s)==-1&&errno==ENOENT);
fail_alloc=1;assert(lstat("RAM:ordinary",&s)==-1&&errno==ENOMEM);fail_alloc=0;
assert(!locks&&!allocations);puts("PASS production lstat/lstat64 nofollow, parent/root/raw/errno/resources; DOS metadata enumeration modeled");}
'''
results=[]
for version in ['before','after']:
 if version=='before':sources=[subprocess.check_output(['git','-C',str(root),'show',a.before_ref+':compiler/crt/posixc/'+n],text=True) for n in ['lstat.c','lstat64.c']];helper=''
 else:sources=[(root/n).read_text() for n in ['lstat.c','lstat64.c']];helper=strip((root/'__lstat_lock.h').read_text())
 # Both legacy files had the same private helper; compile as separately named copies.
 sources=[re.sub(r'\b__lock\b','__lock_'+str(i),strip(s)) for i,s in enumerate(sources)]
 for fast in [False,True]:
  name=version+('-fast' if fast else '-classic');p=out/(name+'.c');p.write_text(mock+helper+'\n'.join(sources)+tail);binary=out/name
  cmd=['clang','-std=c11','-g','-O1','-fsanitize=address,undefined',str(p),'-o',str(binary)]+(['-DAROS_FAST_BSTR'] if fast else [])
  r=subprocess.run(cmd,capture_output=True,text=True);(out/(name+'-compile.log')).write_text(r.stdout+r.stderr);assert r.returncode==0,r.stderr
  r=subprocess.run([str(binary)],capture_output=True,text=True);(out/(name+'.log')).write_text(r.stdout+r.stderr);results.append({'version':version,'fast_bstr':fast,'rc':r.returncode});assert (r.returncode==0)==(version=='after'),r.stderr
(out/'host-result.json').write_text(json.dumps({'result':'PASS','results':results,'scope':'Actual production lstat/lstat64 plus no-follow helper; DOS lock, packet and directory enumeration modeled'},indent=2)+'\n');print(results)
