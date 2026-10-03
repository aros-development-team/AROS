# Copyright (C) 2026, The AROS Development Team. All rights reserved.
"""Host tests for DOS buffering, with controlled handler results.

Run with Python 3 and a C compiler supporting AddressSanitizer and UBSan.
The actual FRead/FWrite/UnGetC/FGetC function bodies are compiled against
mock I/O callbacks to cover oversized requests and short/failed writes.
"""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[4]
header=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t UBYTE;
typedef uint32_t ULONG;
typedef int32_t LONG;
typedef uintptr_t BPTR;
typedef intptr_t SIPTR;
typedef void *APTR;
typedef const void *CONST_APTR;
#define CONST const
#define TRUE 1
#define FALSE 0
#define BNULL 0
#define BADDR(x) ((void *)(uintptr_t)(x))
#define MKBADDR(x) ((BPTR)(uintptr_t)(x))
#define IOBUFSIZE 256
#define OFFSET_CURRENT 0
#define OFFSET_END 1
#define ERROR_NO_FREE_STORE 103
#define ERROR_SEEK_ERROR 219
#define AROS_LIBFUNC_INIT
#define AROS_LIBFUNC_EXIT
#define AROS_LHA(t,n,r) t n
#define AROS_LH1(t,n,a,bt,bn,l,m) t Dos_##n(a,bt bn)
#define AROS_LH2(t,n,a,b,bt,bn,l,m) t Dos_##n(a,b,bt bn)
#define AROS_LH4(t,n,a,b,c,d,bt,bn,l,m) t Dos_##n(a,b,c,d,bt bn)
#define ASSERT(x) assert(x)
#define ASSERT_VALID_PTR(x) assert(x)
#define ASSERT_VALID_PROCESS(x) assert(x)
#define D(x)
struct DosLibrary {int unused;};
struct Process {SIPTR pr_Result2;};
struct FileHandle {LONG fh_Pos,fh_End;ULONG fh_Flags,fh_BufSize;BPTR fh_Buf,fh_OrigBuf;};
#include "DOS_FHFLAGS_PATH"
struct Stream {struct FileHandle fh;UBYTE data[131072];ULONG pos,len;int reads,writes,flushes,shortwrite,zero_write,fail_write,fail_after,force_eof;LONG read_request,write_request;};
static struct DosLibrary base;
static struct Process process;
static int fail_alloc;
static void *FindTask(void *x) {return &process;}
static void SetIoErr(LONG x) {process.pr_Result2=x;}
static LONG IoErr(void) {return process.pr_Result2;}
static void CopyMem(const void *src,void *dst,ULONG n) {memcpy(dst,src,n);}
static void *vbuf_alloc(struct FileHandle *f,char *buf,ULONG n) {
 if(fail_alloc)return NULL;
 if(n<208)n=208;
 if(!buf){buf=malloc(n);f->fh_OrigBuf=MKBADDR(buf);f->fh_Flags|=FHF_OWNBUF;}
 f->fh_Buf=MKBADDR(buf);f->fh_BufSize=n;f->fh_Flags|=FHF_BUF;f->fh_Pos=0;f->fh_End=(f->fh_Flags&FHF_WRITE)?n:0;return buf;
}
static LONG Read(BPTR file,void *b,LONG n) {
 struct Stream *s=BADDR(file);assert(n>=0);s->reads++;s->read_request=n;
 if(s->force_eof)return 0;
 ULONG k=(ULONG)n;if(k>s->len-s->pos)k=s->len-s->pos;
 memcpy(b,s->data+s->pos,k);s->pos+=k;return k;
}
static LONG Write(BPTR file,const void *b,LONG n) {
 struct Stream *s=BADDR(file);assert(n>=0);s->writes++;s->write_request=n;
 if(s->fail_write || (s->fail_after && s->writes>s->fail_after)){SetIoErr(222);return -1;}
 if(s->zero_write)return 0;
 ULONG k=(ULONG)n;if(s->shortwrite && k>(ULONG)s->shortwrite)k=s->shortwrite;
 assert(s->pos+k<=sizeof(s->data));memcpy(s->data+s->pos,b,k);s->pos+=k;if(s->pos>s->len)s->len=s->pos;return k;
}
static LONG Flush(BPTR file) {
 struct Stream *s=BADDR(file);struct FileHandle *f=&s->fh;s->flushes++;
 if(f->fh_Flags&FHF_WRITE){
  if(f->fh_Flags&FHF_APPEND)s->pos=s->len;
  ULONG done=0;
  while(done<(ULONG)f->fh_Pos){LONG n=Write(file,(UBYTE*)BADDR(f->fh_Buf)+done,f->fh_Pos-done);if(n<=0)return 0;done+=n;}
  f->fh_Pos=0;
 }else if(f->fh_Pos<f->fh_End){s->pos-=f->fh_End-f->fh_Pos;f->fh_Pos=f->fh_End=0;}
 return 1;
}
static LONG Seek(BPTR file,LONG n,LONG mode) {
 struct Stream *s=BADDR(file);struct FileHandle *f=&s->fh;
 if(f->fh_Flags&FHF_WRITE)assert(Flush(file));
 LONG old=s->pos-(f->fh_End-f->fh_Pos);f->fh_Pos=f->fh_End=0;
 s->pos=mode==OFFSET_END?s->len+n:old+n;return old;
}
LONG vbuf_fetch(BPTR,UBYTE*,ULONG,struct DosLibrary*);
LONG FWriteChars(BPTR,const UBYTE*,ULONG,struct DosLibrary*);
static void init(struct Stream *s) {memset(s,0,sizeof(*s));fail_alloc=0;SetIoErr(0);}
static void end(struct Stream *s) {free(BADDR(s->fh.fh_Buf));s->fh.fh_Buf=0;}
'''
sources=''
for name in ('fread.c','fwrite.c','ungetc.c','fgetc.c'):
 s=(root/'rom/dos'/name).read_text();sources+=re.sub(r'^#include[^\n]*\n','',s,flags=re.M)+'\n'
footer=r'''
int main(void) {
 static struct Stream s;static UBYTE buf[65536],pattern[65536];BPTR f=MKBADDR(&s);int tests=0;
 for(unsigned i=0;i<sizeof(pattern);i++)pattern[i]=i&255;
 init(&s);memcpy(s.data,pattern,sizeof(pattern));s.len=sizeof(pattern);
 assert(Dos_FRead(f,buf,1,8192,&base)==8192 && !memcmp(buf,pattern,8192));
 assert(s.reads==1);
 assert(Dos_UnGetC(f,EOF,&base)!=0 && Dos_FGetC(f,&base)==255 && Dos_FGetC(f,&base)==0);end(&s);tests++;
 init(&s);memcpy(s.data,pattern,sizeof(pattern));s.len=sizeof(pattern);
 assert(Dos_FRead(f,buf,1,8192,&base)==8192);assert(Dos_UnGetC(f,'X',&base)=='X');assert(Dos_FGetC(f,&base)=='X' && Dos_FGetC(f,&base)==0);end(&s);tests++;
 init(&s);memcpy(s.data,pattern,sizeof(pattern));s.len=sizeof(pattern);
 assert(Dos_FRead(f,buf,1,17,&base)==17);assert(Dos_FRead(f,buf+17,1,4096,&base)==4096);assert(!memcmp(buf,pattern,4113));assert(Seek(f,0,OFFSET_CURRENT)==4113);end(&s);tests++;
 init(&s);s.force_eof=1;assert(Dos_FRead(f,buf,0x80000000U,1,&base)==0);assert(s.reads==1 && s.read_request==INT32_MAX);assert(Dos_UnGetC(f,EOF,&base)!=0 && Dos_FGetC(f,&base)==EOF);assert(s.reads==1);end(&s);tests++;
 init(&s);s.len=sizeof(pattern);fail_alloc=1;assert(Dos_FRead(f,buf,1,8192,&base)==EOF && IoErr()==ERROR_NO_FREE_STORE && s.reads==0);end(&s);tests++;
 init(&s);s.fh.fh_Buf=MKBADDR(malloc(208));s.fh.fh_OrigBuf=1;s.fh.fh_BufSize=99999;memcpy(s.data,pattern,sizeof(pattern));s.len=sizeof(pattern);assert(Dos_FRead(f,buf,1,208,&base)==208);assert(Dos_UnGetC(f,EOF,&base)!=0 && Dos_FGetC(f,&base)==207);end(&s);tests++;
 init(&s);assert(Dos_FWrite(f,pattern,1,65536,&base)==65536);assert(s.writes==1 && s.write_request==65536 && s.len==65536 && !memcmp(s.data,pattern,65536));end(&s);tests++;
 init(&s);s.shortwrite=7;assert(Dos_FWrite(f,pattern,1,1000,&base)==1000);assert(s.writes>1 && s.len==1000 && !memcmp(s.data,pattern,1000));end(&s);tests++;
 init(&s);s.zero_write=1;assert(Dos_FWrite(f,pattern,1,1024,&base)==EOF && s.writes==1);end(&s);tests++;
 init(&s);s.fail_write=1;assert(Dos_FWrite(f,pattern,0x80000000U,1,&base)==EOF && s.write_request==INT32_MAX && IoErr()==222);end(&s);tests++;
 init(&s);s.fail_write=1;assert(Dos_FWrite(f,pattern,65536,65536,&base)==EOF && s.write_request==2147418112);end(&s);tests++;
 init(&s);memcpy(s.data,"start",5);s.len=5;s.fh.fh_Flags=FHF_APPEND;
 assert(Dos_FWrite(f,"pending",1,7,&base)==7 && s.fh.fh_Pos==7 && s.len==5);
 assert(Dos_FWrite(f,pattern,1,8192,&base)==8192 && s.len==8204 && !memcmp(s.data,"startpending",12) && !memcmp(s.data+12,pattern,8192));end(&s);tests++;
 init(&s);s.fh.fh_Flags=FHF_LINEBUF;assert(Dos_FWrite(f,pattern,1,1024,&base)==1024);assert(Flush(f));assert(s.writes>1 && s.len==1024 && !memcmp(s.data,pattern,1024));end(&s);tests++;
 init(&s);s.fh.fh_Flags=FHF_NOBUF;s.shortwrite=13;assert(Dos_FWrite(f,pattern,1,1024,&base)==1024 && s.len==1024 && !memcmp(s.data,pattern,1024));end(&s);tests++;
 init(&s);s.shortwrite=7;s.fail_after=2;assert(Dos_FWrite(f,pattern,1,1024,&base)==EOF && s.writes==3 && s.len==14 && IoErr()==222);end(&s);tests++;
 printf("%d buffered-I/O cases PASS\n",tests);return 0;
}
'''
header = header.replace("DOS_FHFLAGS_PATH",
                        str(root / "rom/dos/dos_fhflags.h"))
compiler = shutil.which("clang") or shutil.which("cc")
if compiler is None:
    raise SystemExit("A C compiler is required")
with tempfile.TemporaryDirectory(prefix="aros-buffered-io-") as directory:
    p = Path(directory)
    fixture = p / "buffered-io-mock.c"
    fixture.write_text(header + sources + footer)
    subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                    "-Wno-unused-parameter", "-Wno-sign-compare",
                    "-fsanitize=address,undefined", str(fixture),
                    "-o", str(p / "buffered-io-mock")], check=True)
    subprocess.run([str(p / "buffered-io-mock")], check=True)
