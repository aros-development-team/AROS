#!/usr/bin/env python3
"""Execute the actual ConClip snapshot reader with fragmented and failing IFF streams."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[4]
s=(root/'workbench/c/ConClip.c').read_text();s=s[s.index('static void readtextsnapshot('):s.index('/* Optional large-copy transport')]
h=(root/'compiler/include/dos/conclip.h').read_text();h='\n'.join(x for x in h.splitlines() if not x.startswith('#include'))+'\n'
prefix=r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint32_t ULONG;typedef int32_t LONG;typedef uint8_t UBYTE;typedef int16_t WORD;typedef uintptr_t IPTR;typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define IFFF_READ 1
#define IFFPARSE_SCAN 1
#define ID_FTXT 2
#define ID_CHRS 4
#define IFFERR_EOF -1
#define IFFERR_MANGLED -2
#define IFFERR_NOMEM -3
#define MEMF_PUBLIC 1
struct Message{int x;};struct IFFHandle{IPTR iff_Stream;};struct ClipboardHandle{int x;};
struct ContextNode{LONG cn_Type,cn_ID,cn_Size;};
static struct IFFHandle handle;static struct ContextNode node;
static int clipunit,handles,clips,opened,live,alloc_calls,fail_alloc,setup_fail,scan_fail,read_fail,bad_node;
static unsigned count,chunk_index;static LONG sizes[256];
static struct IFFHandle*AllocIFF(void){if(setup_fail==1)return NULL;handles++;return &handle;}
static void FreeIFF(struct IFFHandle*h){assert(h==&handle&&handles==1);handles--;}
static struct ClipboardHandle*OpenClipboard(int unit){assert(unit==0);if(setup_fail==2)return NULL;clips++;return (void*)123;}
static void CloseClipboard(struct ClipboardHandle*h){assert(h==(void*)123&&clips==1);clips--;}
static void InitIFFasClip(struct IFFHandle*h){assert(h==&handle);}
static int OpenIFF(struct IFFHandle*h,int mode){assert(h==&handle&&mode==1);if(setup_fail==3)return -2;opened++;return 0;}
static void CloseIFF(struct IFFHandle*h){assert(h==&handle&&opened==1);opened--;}
static LONG StopChunk(struct IFFHandle*h,int type,int id){assert(h==&handle&&type==ID_FTXT&&id==ID_CHRS);return setup_fail==4?-2:0;}
static LONG ParseIFF(struct IFFHandle*h,int mode){assert(h==&handle&&mode==IFFPARSE_SCAN);if(scan_fail==(int)chunk_index+1)return -2;if(chunk_index==count)return IFFERR_EOF;node=(struct ContextNode){ID_FTXT,ID_CHRS,sizes[chunk_index++]};return 0;}
static struct ContextNode*CurrentChunk(struct IFFHandle*h){assert(h==&handle);if(bad_node==1)return NULL;if(bad_node==2)node.cn_Type=99;if(bad_node==3)node.cn_ID=99;return &node;}
static LONG ReadChunkBytes(struct IFFHandle*h,UBYTE*p,ULONG n){assert(h==&handle&&p&&n==(ULONG)sizes[chunk_index-1]);assert(n<=1048576);memset(p,chunk_index,n);return read_fail==(int)chunk_index?-1:(LONG)n;}
static void*AllocMem(ULONG n,int flags){assert(flags==MEMF_PUBLIC&&n<=2097152);if(++alloc_calls==fail_alloc)return NULL;ULONG*p=malloc(n+sizeof(ULONG));assert(p);*p=n;live++;return p+1;}
static void FreeMem(void*p,ULONG n){assert(p&&live);ULONG*b=(ULONG*)p-1;assert(*b==n);live--;free(b);}
#define CopyMem(a,b,n) memcpy(b,a,n)
'''
main=r'''
static void run(int success){struct ConClipTextMessage m={0};m.data=(void*)42;m.length=7;m.capacity=8;chunk_index=alloc_calls=0;readtextsnapshot(&m);
 assert(!handles&&!clips&&!opened);assert(m.status==(success?CONCLIP_TEXT_OK:CONCLIP_TEXT_ERROR));
 if(success){ULONG total=0;for(unsigned i=0;i<count;i++){for(LONG j=0;j<sizes[i];j++)assert(m.data[total++]==(UBYTE)(i+1));}assert(m.length==total&&m.capacity>=total+12);memset(m.data+total,0,12);FreeMem(m.data,m.capacity);}
 else assert(!m.data&&!m.length&&!m.capacity);
 assert(!live);
}
int main(void){
 for(count=1;count<=128;count++){for(unsigned i=0;i<count;i++)sizes[i]=i%5?i*17:0;run(1);}
 count=128;for(unsigned i=0;i<count;i++)sizes[i]=8192;run(1);
 /* Empty CHRS is a successful empty clipboard; absent CHRS is an error. */
 count=1;sizes[0]=0;run(1);count=0;run(0);
 count=3;sizes[0]=4;sizes[1]=1024;sizes[2]=65536;
 for(setup_fail=1;setup_fail<=4;setup_fail++)run(0);setup_fail=0;
 for(scan_fail=1;scan_fail<=4;scan_fail++)run(0);scan_fail=0;
 for(read_fail=1;read_fail<=3;read_fail++)run(0);read_fail=0;
 for(fail_alloc=1;fail_alloc<=3;fail_alloc++)run(0);fail_alloc=0;
 for(bad_node=1;bad_node<=3;bad_node++)run(0);bad_node=0;
 for(int at=0;at<3;at++){LONG old=sizes[at];sizes[at]=-1;run(0);sizes[at]=0x7fffffff;run(0);sizes[at]=old;}
 puts("CONCLIP READ PASS:129 multi-chunk streams through1MiB, empty/no text, all setup/parse/read/grow failures, invalid chunks, atomic ownership and spare framing bytes");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'t.c';e=Path(d)/'t';p.write_text(prefix+h+s+main)
 subprocess.run(['cc','-Wall','-Werror','-Wno-misleading-indentation','-fsanitize=address,undefined',str(p),'-o',str(e)],check=True);subprocess.run([str(e)],check=True)
 for label,broken in [('first chunk only',s.replace('saw_text = TRUE;', 'saw_text = TRUE; error = IFFERR_EOF; break;')),('lost previous chunks',s.replace('if (used) CopyMem(data, replacement, used);','/* lost prefix */')),('partial success',s.replace('error == IFFERR_EOF && saw_text','saw_text'))]:
  p.write_text(prefix+h+broken+main);subprocess.run(['cc','-Wall','-Werror','-Wno-misleading-indentation',str(p),'-o',str(e)],check=True)
  assert subprocess.run([str(e)],capture_output=True).returncode!=0
  print('Negative control rejected:',label)
