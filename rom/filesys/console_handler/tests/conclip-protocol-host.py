from pathlib import Path
import subprocess,tempfile
s=(Path(__file__).resolve().parent.parent/'support.c').read_text();fn=s[s.index('#define CD_GETPASTEMODE'):s.index('/****************************************************************************************/',s.index('void do_paste('))]
code=r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef uint32_t ULONG;typedef int BOOL;typedef unsigned char UBYTE;typedef char*STRPTR;typedef int16_t WORD;
#define TRUE 1
#define FALSE 0
#define D(x)
#define CMD_NONSTD 9
#define FHFLG_RAW 1
#define FHFLG_DEVICEMODE 2
#define NT_MSGPORT 4
#define NT_MESSAGE 5
#define PA_SIGNAL 0
#define SIGB_SINGLE 4
#define SIGF_SINGLE 16
#define MEMF_CLEAR 1
#define MEMF_ANY 2
#define EO_BIGCHANGE 1
#define CODE_PASTE 'V'
#define PASTEBUFSIZE 16384
#define CONCLIP_PORTNAME "ConClip.rendezvous"
#define CONCLIP_READ_TEXT 0x100
#define CONCLIP_TEXT_VERSION 1
#define CONCLIP_TEXT_UNSUPPORTED 0
#define CONCLIP_TEXT_OK 1
#define CONCLIP_TEXT_ERROR 2
#define SetMem(p,v,n) memset(p,v,n)
#define CopyMem(a,b,n) memcpy(b,a,n)
#define NEWLIST(l) ((void)(l))
struct Node {int ln_Type;};struct List {int unused;};
struct MsgPort {struct Node mp_Node;int mp_Flags,mp_SigBit;void*mp_SigTask;struct List mp_MsgList;};
struct Message {struct Node mn_Node;struct MsgPort*mn_ReplyPort;unsigned mn_Length;};
struct StringInfo {WORD MaxChars;};struct SGWork {char*WorkBuffer;WORD Code,EditOp,BufferPos;struct StringInfo*StringInfo;};
struct MyEditHookMsg {struct Message msg;struct SGWork*sgw;WORD code;};
struct ConClipTextMessage {struct Message msg;struct SGWork*sgw;WORD code;ULONG version,status;UBYTE*data;ULONG length,capacity;};
struct IOStdReq {unsigned io_Command;void*io_Data;ULONG io_Length,io_Actual;};struct IORequest {int unused;};
struct filehandle {int flags;char devname[32];struct IOStdReq conwriteio;char*pastebuffer;ULONG pastebuffersize,pastebufferpos,pastebuffercapacity;};
static int peer,enabled,live,extensions,legacy,failalloc;static ULONG length;static struct MsgPort port;
static void *AllocMem(ULONG n,int flags){(void)flags;if(failalloc)return 0;size_t*p=calloc(1,n+sizeof(*p));assert(p);*p=n;live++;return p+1;}
static void FreeMem(void*p,ULONG n){size_t*h=(size_t*)p-1;assert(*h==n);live--;free(h);}
static struct MsgPort*FindPort(const char*n){(void)n;return &port;}
static void*FindTask(void*n){(void)n;return &port;}
static void SetSignal(int a,int b){(void)a;(void)b;}
static void WaitPort(struct MsgPort*p){(void)p;}
static void GetMsg(struct MsgPort*p){(void)p;}
static int DoIO(struct IORequest*r){struct IOStdReq*q=(void*)r;*(ULONG*)q->io_Data=enabled;q->io_Actual=4;return 0;}
static void PutMsg(struct MsgPort*p,struct Message*m){(void)p;struct MyEditHookMsg*base=(void*)m;
 if(base->code==CONCLIP_READ_TEXT){extensions++;struct ConClipTextMessage*t=(void*)m;assert(t->version==1&&t->msg.mn_Length==sizeof(*t)&&!t->sgw);if(!peer)return;if(peer==1){t->status=2;return;}t->status=1;t->length=length;t->capacity=length+12;t->data=AllocMem(t->capacity,0);assert(t->data);memset(t->data,'x',length);}
 else{legacy++;assert(base->code=='V');struct SGWork*w=base->sgw;ULONG n=length;if(n>(ULONG)w->StringInfo->MaxChars-1)n=w->StringInfo->MaxChars-1;memset(w->WorkBuffer,'x',n);w->BufferPos=n;}
}
'''+fn+r'''
int main(void){ULONG sizes[]={0,1,16384,32768,65536,1048576};int count=0;
 for(peer=0;peer<3;peer++)for(enabled=0;enabled<2;enabled++)for(int raw=0;raw<2;raw++)for(unsigned k=0;k<6;k++){
 length=sizes[k];extensions=legacy=live=0;struct filehandle f={0};f.flags=2|raw;strcpy(f.devname,"consoleng.device");f.pastebuffer=AllocMem(4,0);f.pastebuffercapacity=f.pastebuffersize=4;char*old=f.pastebuffer;
 do_paste(&f);assert(extensions==raw);
 if(raw&&peer==1){assert(f.pastebuffer==old&&f.pastebuffersize==4&&!legacy);}
 else{ULONG n=length;if(!raw||!peer){ULONG cap=raw&&enabled?16371:16383;if(n>cap)n=cap;assert(legacy==1&&f.pastebuffercapacity==16384);}else assert(!legacy&&f.pastebuffercapacity==length+12);
 assert(f.pastebuffersize==n+(raw&&enabled?12:0));ULONG start=raw&&enabled?6:0;for(ULONG j=0;j<n;j++)assert(f.pastebuffer[start+j]=='x');if(start){assert(!memcmp(f.pastebuffer,"\033[200~",6));assert(!memcmp(f.pastebuffer+6+n,"\033[201~",6));}}
 FreeMem(f.pastebuffer,f.pastebuffercapacity);assert(!live);count++;
 }
 peer=0;enabled=0;length=10;struct filehandle failed={0};failed.flags=3;strcpy(failed.devname,"consoleng.device");failed.pastebuffer=AllocMem(4,0);failed.pastebuffercapacity=failed.pastebuffersize=4;char*before=failed.pastebuffer;legacy=extensions=0;failalloc=1;
 do_paste(&failed);assert(failed.pastebuffer==before&&failed.pastebuffersize==4&&extensions==1&&!legacy);failalloc=0;FreeMem(before,4);assert(!live);
 return count!=72;}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(code)
 subprocess.run(['cc','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS:72 handler snapshot/error/legacy cases through1MiB; exact framing/capacity, old-peer fallback and legacy allocation failure')
