from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[4];s=(r/'workbench/c/ConClip.c').read_text();fn=s[s.index('static void readtextsnapshot('):s.index('/* Optional large-copy transport;')]
code=r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define IFFERR_MANGLED -10
#define IFFERR_NOMEM -11
#define IFFERR_EOF -12
#define CopyMem(s,d,n) memcpy(d,s,n)
typedef uint32_t ULONG;typedef int32_t LONG;typedef uintptr_t IPTR;typedef unsigned char UBYTE;
#define MEMF_PUBLIC 1
#define CONCLIP_TEXT_ERROR 2
#define CONCLIP_TEXT_OK 1
#define IFFF_READ 0
#define ID_FTXT 1
#define ID_CHRS 2
#define IFFPARSE_SCAN 0
struct ConClipTextMessage {int status;UBYTE*data;ULONG length,capacity;};
struct IFFHandle {IPTR iff_Stream;};struct ContextNode {int cn_Type,cn_ID;LONG cn_Size;};struct ClipboardHandle {int x;};
static struct IFFHandle handle;static struct ContextNode chunk;static struct ClipboardHandle clip;
static int fail,live,closed,freed,clipclosed,parsed;static ULONG allocation,clipunit;
static struct IFFHandle *AllocIFF(void){return fail==1?0:&handle;}
static void FreeIFF(struct IFFHandle*p){assert(p==&handle);freed++;}
static struct ClipboardHandle *OpenClipboard(ULONG u){assert(!u);return fail==2?0:&clip;}
static void CloseClipboard(struct ClipboardHandle*p){assert(p==&clip);clipclosed++;}
static void InitIFFasClip(struct IFFHandle*p){assert(p==&handle);}
static int OpenIFF(struct IFFHandle*p,int f){(void)p;(void)f;return fail==3;}
static void CloseIFF(struct IFFHandle*p){assert(p==&handle);closed++;}
static int StopChunk(struct IFFHandle*p,int t,int id){(void)p;(void)t;(void)id;return fail==4;}
static int ParseIFF(struct IFFHandle*p,int f){(void)p;(void)f;return fail==5?1:parsed++?IFFERR_EOF:0;}
static struct ContextNode *CurrentChunk(struct IFFHandle*p){(void)p;return fail==6?0:&chunk;}
static void *AllocMem(ULONG n,int f){assert(f==MEMF_PUBLIC);allocation=n;if(fail==7||n>2097152)return 0;live++;return malloc(n);}
static void FreeMem(void*p,ULONG n){assert(n==allocation);live--;free(p);}
static LONG ReadChunkBytes(struct IFFHandle*p,void*d,LONG n){(void)p;memset(d,'x',n);return fail==8?n-1:n;}
'''+fn+r'''
int main(void){LONG sizes[]={-1,0,1,16384,32768,65536,1048576,0x7fffffff-12,0x7fffffff};int count=0;
 for(fail=0;fail<9;fail++)for(unsigned i=0;i<sizeof(sizes)/sizeof(*sizes);i++){
  struct ConClipTextMessage msg={0};chunk=(struct ContextNode){ID_FTXT,ID_CHRS,sizes[i]};live=closed=freed=clipclosed=0;allocation=0;parsed=0;
  readtextsnapshot(&msg);int ok=!fail&&sizes[i]>=0&&sizes[i]<=1048576;
  assert(msg.status==(ok?CONCLIP_TEXT_OK:CONCLIP_TEXT_ERROR));assert(freed==(fail!=1));assert(clipclosed==(fail!=1&&fail!=2));assert(closed==(fail!=1&&fail!=2&&fail!=3));
  if(ok){assert(msg.length==(ULONG)sizes[i]&&msg.capacity>=msg.length+12&&msg.capacity>=256);for(ULONG j=0;j<msg.length;j++)assert(msg.data[j]=='x');FreeMem(msg.data,msg.capacity);}else assert(!msg.data&&!msg.length&&!msg.capacity);
  assert(!live);count++;
 }
 return count!=81;}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(code)
 subprocess.run(['cc','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS: 81 ConClip snapshot length/failure cases through1MiB, exact ownership/cleanup and short-read rejection')
