from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parent.parent;s=(r/'support.c').read_text();fn=s[s.index('#define CD_GETPASTEMODE'):s.index('\nvoid do_paste(')]
code=r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uint32_t ULONG;typedef unsigned char UBYTE;
#define CopyMem(a,b,n) memcpy(b,a,n)
typedef int BOOL;
#define FALSE 0
#define CMD_NONSTD 9
#define FHFLG_RAW 1
#define FHFLG_DEVICEMODE 2
struct IOStdReq {unsigned io_Command;void *io_Data;ULONG io_Length,io_Actual;};
struct IORequest {int unused;};
struct filehandle {int flags;char devname[32];struct IOStdReq conwriteio;};
static int calls,error;static ULONG value,actual;
static int DoIO(struct IORequest *r){struct IOStdReq *q=(void*)r;calls++;assert(q->io_Command==34&&q->io_Length==4&&!q->io_Actual);memcpy(q->io_Data,&value,4);q->io_Actual=actual;return error;}

'''+fn+r'''
int main(void){
 for(int flags=0;flags<4;flags++)for(int native=0;native<2;native++)for(int err=0;err<2;err++)for(unsigned v=0;v<3;v++)for(unsigned a=0;a<8;a++){
  struct filehandle f={0};f.flags=flags;strcpy(f.devname,native?"consoleng.device":"serial.device");struct IOStdReq before=f.conwriteio;
  calls=0;error=err;value=v;actual=a;int enabled=bracketed_paste(&f);
  assert(enabled==(flags==3&&native&&!err&&v==1&&a==4));assert(calls==(flags==3&&native));assert(!memcmp(&before,&f.conwriteio,sizeof(before)));
 }

 UBYTE buffer[16386],expected[16384];
 for(unsigned n=0;n<=16372;n++){
  memset(buffer,0xa5,sizeof(buffer));unsigned k=6;
  memcpy(expected,"\033[200~",6);
  for(unsigned j=0;j<n;j++){UBYTE b=(j*37+n)%256;buffer[j+1]=b;if(b!=27&&b!=3)expected[k++]=b;}
  memcpy(expected+k,"\033[201~",6);k+=6;
  unsigned out=frame_paste(buffer+1,n);assert(out==k);assert(!memcmp(buffer+1,expected,k));
  assert(buffer[0]==0xa5&&buffer[16385]==0xa5);
 }
 return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(code)
 subprocess.run(['cc','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS 16373 paste lengths, all byte values, ESC/ETX filtering and exact envelopes')
