#!/usr/bin/env python3
"""Actual handler teardown: final close succeeds; startup/error exits stay errors."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[4];s=(root/'rom/filesys/console_handler/con_handler.c').read_text()
assert 'struct filehandle *fh = NULL;' in s
body=s[s.index('\nend:\n')+6:]
# Handler entry is the last function in this source.
assert body.rstrip().endswith('}')
prefix=r'''
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
typedef long LONG;typedef int BOOL;
#define D(x) do{}while(0)
#define ACTION_END 1007
#define DOSTRUE -1
#define DOSFALSE 0
#define FHFLG_DEVICEMODE 1
#define FHFLG_ASYNCCONSOLEREAD 2
#define LDF_DEVICES 1
#define LDF_WRITE 2
#define ioReq(x) (x)
struct MsgPort{int x;};struct DosPacket{int dp_Type;};
struct DosList{struct MsgPort*dn_Task;};
struct filehandle{unsigned flags;struct DosList*dn;void*conreadio;};
static struct DosList head,node;static int stops,aborts,waits,closes,replies,deletes,locked;static LONG result,result_error;
static struct DosList*LockDosList(int f){(void)f;assert(!locked++);return &head;}
static struct DosList*NextDosEntry(struct DosList*p,int f){(void)f;return p==&head?&node:NULL;}
static void UnLockDosList(int f){(void)f;assert(locked--==1);}
static void stopread(struct filehandle*f,struct DosPacket*p){(void)f;(void)p;assert(!closes&&!replies);stops++;}
static void AbortIO(void*p){(void)p;assert(stops==1&&!waits&&!closes);aborts++;}
static void WaitIO(void*p){(void)p;assert(aborts==1&&!closes);waits++;}
static void close_con(struct filehandle*f){if(f->flags&FHFLG_ASYNCCONSOLEREAD)assert(waits==1);assert(stops==1&&!replies);closes++;}
static void replypkt2(struct DosPacket*p,LONG value,LONG error){assert(p&&!deletes);replies++;result=value;result_error=error;}
static void DeleteMsgPort(struct MsgPort*p){assert(p);deletes++;}
static int teardown(struct filehandle*fh,struct DosPacket*dp,struct MsgPort*mp,LONG error){struct DosPacket*waitingdp=NULL;
'''
main=r'''
int main(void){int cases=0;
for(int present=0;present<2;present++)for(int flags=0;flags<4;flags++)for(int packet=0;packet<3;packet++)for(int error=0;error<2;error++){
 struct MsgPort port={0};struct DosPacket p={packet==1?ACTION_END:42};struct filehandle f={flags,&node,(void*)1};node.dn_Task=&port;
 stops=aborts=waits=closes=replies=deletes=locked=0;result=result_error=999;
 assert(!teardown(present?&f:NULL,packet?&p:NULL,&port,error?103:0));
 assert(stops==present&&closes==present&&deletes==1&&!locked);
 assert(aborts==(present&&!!(flags&2))&&waits==aborts);
 assert(replies==!!packet);
 if(packet)assert(result==(packet==1&&!error?DOSTRUE:DOSFALSE)&&result_error==(error?103:0));
 if(present&&(flags&1))assert(!node.dn_Task);
 cases++;
}
stops=aborts=waits=closes=replies=deletes=locked=0;struct DosPacket startup={42};assert(!teardown(NULL,&startup,NULL,103));assert(!result&&result_error==103&&!deletes&&!closes);
printf("Handler close PASS: %d exit/packet/error/cleanup cases plus failed port creation\n",cases);
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'t.c';exe=Path(d)/'t';p.write_text(prefix+body+main)
 subprocess.run(['cc','-Wall','-Werror','-fsanitize=address,undefined',str(p),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
 p.write_text(prefix+body.replace('dp->dp_Type == ACTION_END && !error ? DOSTRUE : DOSFALSE','DOSFALSE')+main)
 subprocess.run(['cc','-Wall','-Werror',str(p),'-o',str(exe)],check=True);assert subprocess.run([str(exe)],capture_output=True).returncode!=0
 print('Negative control rejected: successful final close reported as failure')
