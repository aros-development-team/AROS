from pathlib import Path
import subprocess, tempfile, resource
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
temporary = tempfile.TemporaryDirectory(prefix='aros-raw-paste-')
p=Path(temporary.name)
s=(Path(__file__).resolve().parent.parent / 'con_handler.c').read_text();s=s[s.index('/* A native paste request shares'):s.index('static void close_con(')] + s[s.index('static void pump_raw_input('):s.index('\nstatic void stopwait')]
pre=r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef unsigned char UBYTE; typedef long LONG; typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define TIMERNAME "timer.device"
#define UNIT_MICROHZ 0
#define TR_ADDREQUEST 9
struct MsgPort { int live; };
struct IORequest { int io_Command; };
struct timerequest { struct IORequest tr_node; struct { int tv_secs,tv_micro; } tr_time; };
static int fault, ports, requests, opened, sent, waited, aborted;
static struct MsgPort *CreateMsgPort(void){if(fault==1)return NULL;ports++;return calloc(1,sizeof(struct MsgPort));}
static void DeleteMsgPort(struct MsgPort *p){assert(p);ports--;free(p);}
static void *CreateIORequest(struct MsgPort *p,size_t n){assert(p);if(fault==2)return NULL;requests++;return calloc(1,n);}
static void DeleteIORequest(struct IORequest *p){requests--;free(p);}
static int OpenDevice(const char *n,int u,struct IORequest *p,int f){(void)n;(void)u;(void)p;(void)f;if(fault==3)return 1;opened++;return 0;}
static void CloseDevice(struct IORequest *p){(void)p;opened--;}
static void SendIO(struct IORequest *p){struct timerequest *t=(void*)p;assert(t->tr_time.tv_secs==0&&t->tr_time.tv_micro==50000);sent++;}
static void AbortIO(struct IORequest *p){assert(p);aborted++;}
static void WaitIO(struct IORequest *p){assert(p);waited++;}
#define INPUTBUFFER_SIZE 256
#define PASTEBUFSIZE 16384
#define FHFLG_DEVICEMODE 1
#define CopyMem(s,d,n) memcpy(d,s,n)
#define FreeMem(p,n) free(p)
struct filehandle {int flags,conbufferpos,conbuffersize,inputsize,inputpos,inputstart,pastebuffersize,pastebufferpos,pastebuffercapacity; UBYTE rawpasteprefix; BOOL rawprefixpending,rawprefixexpired; struct MsgPort *rawprefixmp; struct timerequest *rawprefixreq; char devname[64]; UBYTE consolebuffer[1024],inputbuffer[258]; char *pastebuffer;};
static unsigned char output[2200000], clip[1048576]; static size_t used,cliplen; static int pending,limit,pastes;
static void HandlePendingReads(struct filehandle *f){ if(pending && f->inputsize){int n=f->inputsize<limit?f->inputsize:limit; memcpy(output+used,f->inputbuffer,n);used+=n;memmove(f->inputbuffer,f->inputbuffer+n,f->inputsize-n);f->inputsize-=n;pending=0;}}
static void do_paste(struct filehandle *f){assert(!f->pastebuffer);f->pastebuffercapacity=cliplen?cliplen:1;f->pastebuffer=malloc(f->pastebuffercapacity);memcpy(f->pastebuffer,clip,cliplen);f->pastebuffersize=cliplen;f->pastebufferpos=0;pastes++;}
'''
post=r'''
static void cleanup(struct filehandle *f){stop_raw_prefix_timer(f);if(f->rawprefixreq){CloseDevice((void*)f->rawprefixreq);DeleteIORequest((void*)f->rawprefixreq);}if(f->rawprefixmp)DeleteMsgPort(f->rawprefixmp);assert(!ports&&!requests&&!opened);}
static void expire(struct filehandle *f){assert(f->rawprefixpending);WaitIO((void*)f->rawprefixreq);f->rawprefixpending=0;f->rawprefixexpired=1;pump_raw_input(f);}
static void prefixes(void){
 const unsigned char prefix[]={0x9b,'0',' '};
 for(int fail=0;fail<4;fail++)for(int n=1;n<=3;n++){
  struct filehandle f={0};f.flags=1;strcpy(f.devname,"consoleng.device");fault=fail;used=0;pending=1;limit=256;
  memcpy(f.consolebuffer,prefix,n);f.conbuffersize=n;pump_raw_input(&f);
  if(!fail){assert(!used&&f.rawpasteprefix==n&&f.rawprefixpending);expire(&f);}
  assert(used==(size_t)n&&!memcmp(output,prefix,n)&&!f.rawpasteprefix);cleanup(&f);
 }
 fault=0;
 /* Expiry with a full application buffer must retain the prefix in order. */
 struct filehandle f={0};f.flags=1;strcpy(f.devname,"consoleng.device");used=0;pending=0;limit=256;
 memcpy(f.consolebuffer,prefix,3);f.conbuffersize=3;pump_raw_input(&f);
 memset(f.inputbuffer,'x',256);f.inputsize=256;expire(&f);
 assert(f.rawpasteprefix==3&&f.rawprefixexpired&&f.inputsize==256);
 pending=1;HandlePendingReads(&f);pending=1;pump_raw_input(&f);
 assert(used==259&&!memcmp(output+256,prefix,3)&&!f.rawpasteprefix);cleanup(&f);
 /* Closing a handle with a retained prefix cancels and reaps its request. */
 memset(&f,0,sizeof(f));f.flags=1;strcpy(f.devname,"consoleng.device");f.consolebuffer[0]=0x9b;f.conbuffersize=1;pending=0;pump_raw_input(&f);
 int a=aborted,w=waited;cleanup(&f);assert(aborted==a+1&&waited==w+1);
 puts("PASS 14 prefix timeout/resource failure/backpressure/close cases");
}
static void drain(struct filehandle *f){int guard=0;while(f->inputsize||f->pastebuffer||f->conbufferpos<f->conbuffersize){assert(++guard<3000000);pending=1;HandlePendingReads(f);pump_raw_input(f);}}
static void run(int native,int chunk,int readsize,int clipsize){struct filehandle f={0};f.flags=1;strcpy(f.devname,native?"consoleng.device":"serial.device");used=pastes=0;limit=readsize;cliplen=clipsize;for(int i=0;i<clipsize;i++)clip[i]=(unsigned char)(i%256);unsigned char in[]={3,27,'[','A',0x9b,'A',0x9b,'0','X',0x9b,'0',' ','v','z',0x9b,'0',' ','v',4}; unsigned char expected[2200000];size_t n=0;for(size_t i=0;i<sizeof(in);){if(native&&i+4<=sizeof(in)&&!memcmp(in+i,"\x9b" "0 v",4)){memcpy(expected+n,clip,cliplen);n+=cliplen;i+=4;}else expected[n++]=in[i++];}for(size_t i=0;i<sizeof(in);){size_t k=sizeof(in)-i;if(k>(size_t)chunk)k=chunk;memcpy(f.consolebuffer,in+i,k);f.conbufferpos=0;f.conbuffersize=k;i+=k;pending=0;pump_raw_input(&f);drain(&f);}assert(used==n);assert(!memcmp(output,expected,n));assert(pastes==(native?2:0));assert(!f.rawpasteprefix);cleanup(&f);}
int main(void){prefixes();int count=0;int sizes[]={0,1,2,255,256,257,4096,16384};for(int native=0;native<2;native++)for(int chunk=1;chunk<20;chunk++)for(int r=1;r<=256;r*=2)for(int c=0;c<8;c++){run(native,chunk,r,sizes[c]);count++;}for(int r=1;r<=256;r*=16){run(1,3,r,65536);run(1,7,r,1048576);count+=2;}printf("PASS %d fragmentation/backpressure cases\n",count);}
'''
(p/'pump-test.c').write_text(pre+s+post)
subprocess.run(['cc','-fsanitize=address,undefined','-g','-Wall','-Wextra',str(p/'pump-test.c'),'-o',str(p/'pump-test')],check=True)
subprocess.run([str(p/'pump-test')],check=True)

# Prove that the fixture rejects both indefinite retention and missed cancellation.
for name, old, new in [
    ('missing-timeout', 'fh->rawprefixpending = TRUE;\n    SendIO', 'return;\n    SendIO'),
    ('missing-cancel', 'AbortIO((struct IORequest *)fh->rawprefixreq);', '(void)fh;'),
]:
    assert old in s
    source = p / (name + '.c')
    source.write_text(pre + s.replace(old, new, 1) + post)
    binary = p / name
    subprocess.run(['cc', '-g', '-Wall', '-Wextra', str(source), '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    assert result.returncode != 0, name + ' unexpectedly passed'
print('PASS: two negative controls reject missing timeout and cancellation')
