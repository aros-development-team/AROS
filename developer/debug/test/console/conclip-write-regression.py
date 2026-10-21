#!/usr/bin/env python3
"""Actual ConClip writer validates input and reports short writes/chunk failures."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[4]
s=(root/'workbench/c/ConClip.c').read_text();s=s[s.index('static void writetextsnapshot('):s.index('static void handleall(void)')]
h=(root/'compiler/include/dos/conclip.h').read_text();h='\n'.join(x for x in h.splitlines() if not x.startswith('#include'))+'\n'
prefix=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef uint32_t ULONG;typedef int32_t LONG;typedef uint8_t UBYTE;typedef int16_t WORD;typedef uintptr_t IPTR;typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define IFFF_WRITE 1
#define ID_FTXT 2
#define ID_FORM 3
#define ID_CHRS 4
#define IFFSIZE_UNKNOWN -1
struct Message{int x;};struct IFFHandle{IPTR iff_Stream;};struct ClipboardHandle{int x;};
static struct IFFHandle handle;static int clipunit,step,fail,handles,clips,opened,pushes,pops,writes;
static int next(void){return ++step==fail;}
static struct IFFHandle*AllocIFF(void){if(next())return NULL;handles++;return &handle;}
static void FreeIFF(struct IFFHandle*h){assert(h==&handle&&handles==1);handles--;}
static struct ClipboardHandle*OpenClipboard(int unit){assert(unit==0);if(next())return NULL;clips++;return (void*)123;}
static void CloseClipboard(struct ClipboardHandle*h){assert(h==(void*)123&&clips==1);clips--;}
static void InitIFFasClip(struct IFFHandle*h){assert(h==&handle);}
static int OpenIFF(struct IFFHandle*h,int mode){assert(h==&handle&&mode==1);if(next())return -1;opened++;return 0;}
static void CloseIFF(struct IFFHandle*h){assert(h==&handle&&opened==1&&pushes==pops);opened--;}
static int PushChunk(struct IFFHandle*h,int type,int id,int size){assert(h==&handle&&type==2&&(id==3||id==4)&&size==-1);if(next())return -1;pushes++;return 0;}
static LONG WriteChunkBytes(struct IFFHandle*h,UBYTE*p,ULONG n){assert(h==&handle&&p&&n);writes++;return next()?(LONG)n-1:(LONG)n;}
static int PopChunk(struct IFFHandle*h){assert(h==&handle&&pops<pushes);pops++;return next()?-1:0;}
'''
main=r'''
int main(void){UBYTE data[1]={42};struct ConClipTextMessage m={0};m.data=data;m.length=m.capacity=1;
for(fail=0;fail<=8;fail++){step=pushes=pops=writes=0;writetextsnapshot(&m);assert(m.status==(fail?CONCLIP_TEXT_ERROR:CONCLIP_TEXT_OK));assert(!handles&&!clips&&!opened);}
fail=0;m.length=0;m.capacity=0;m.data=NULL;step=pushes=pops=writes=0;writetextsnapshot(&m);assert(m.status==CONCLIP_TEXT_OK&&!writes&&!handles&&!clips&&!opened);
for(int kind=0;kind<4;kind++){m.data=data;m.length=m.capacity=1;m.sgw=NULL;if(kind==0)m.sgw=(void*)1;if(kind==1)m.data=NULL;if(kind==2)m.capacity=0;if(kind==3)m.length=m.capacity=CONCLIP_WRITE_MAX+1;step=0;writetextsnapshot(&m);assert(m.status==CONCLIP_TEXT_ERROR&&!step);}
puts("CONCLIP WRITE PASS: validation before I/O, empty text, all allocation/open/write/pop failures, cleanup");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'t.c';e=Path(d)/'t';p.write_text(prefix+h+s+main)
 subprocess.run(['cc','-Wall','-Werror','-fsanitize=address,undefined',str(p),'-o',str(e)],check=True);subprocess.run([str(e)],check=True)
 p.write_text(prefix+h+s.replace('if (PopChunk(iff)) written = FALSE;','PopChunk(iff);')+main)
 subprocess.run(['cc',str(p),'-o',str(e)],check=True);assert subprocess.run([str(e)],capture_output=True).returncode!=0
 print('Negative control rejected: ignored chunk errors')
