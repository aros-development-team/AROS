"""Exercise actual handler read boundaries and accounting under sanitizers."""
from pathlib import Path
import subprocess
import tempfile

source = (Path(__file__).resolve().parent.parent / 'support.c').read_text()
answer = source[source.index('ULONG answer_read_request('):source.index('\nvoid HandlePendingReads(')]
read = source[source.index('void con_read('):]
prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef unsigned char UBYTE;
typedef unsigned long ULONG;
#define FHFLG_RAW 1
#define FHFLG_DEVICEMODE 2
#define FHFLG_CANREAD 4
#define FHFLG_EOF 8
#define FHFLG_READPENDING 16
#define CopyMem(s,d,n) memmove(d,s,n)
struct List { int unused; };
struct Node { int unused; };
struct DosPacket { uintptr_t dp_Arg2; ULONG dp_Arg3; void *dp_Link; long result; };
struct filehandle {
    int flags, inputsize, inputpos, inputstart, canreadsize;
    char devname[64]; UBYTE inputbuffer[258]; struct List pendingReads;
};
static void replypkt2(struct DosPacket *p, long n, long err) { assert(!err); p->result=n; }
static void AddTail(struct List *l, struct Node *n) { (void)l; (void)n; assert(0); }
'''
suffix = r'''
static void test(int raw, int device, const char *name, int chunk) {
    const char *text="Echo FIRST\nEcho SECOND\nthird\n";
    struct filehandle f={0};
    unsigned char output[256], all[256]; size_t used=0;
    f.flags=FHFLG_CANREAD|(raw?FHFLG_RAW:0)|(device?FHFLG_DEVICEMODE:0);
    strcpy(f.devname,name); strcpy((char*)f.inputbuffer,text);
    f.inputsize=f.inputpos=f.inputstart=f.canreadsize=strlen(text);
    while(f.flags & FHFLG_CANREAD) {
        size_t remaining=strlen(text)-used;
        size_t expected=remaining<(size_t)chunk?remaining:(size_t)chunk;
        if(!raw && device && !strcmp(name,"consoleng.device")) {
            const char *nl=memchr(text+used,'\n',expected);
            if(nl) expected=(size_t)(nl-(text+used))+1;
        }
        struct DosPacket p={(uintptr_t)output,(ULONG)chunk,0,-1};
        con_read(&f,&p);
        assert(p.result==(long)expected && expected);
        memcpy(all+used,output,expected); used+=expected;
        assert(f.canreadsize==(int)(strlen(text)-used));
        assert(f.inputsize==f.canreadsize && f.inputpos==f.inputsize && f.inputstart==f.inputsize);
    }
    assert(used==strlen(text) && !memcmp(all,text,used));
}
int main(void) {
    int count=0;
    for(int raw=0;raw<2;raw++) for(int dev=0;dev<2;dev++)
    for(int name=0;name<3;name++) for(int size=1;size<=256;size++) {
        test(raw,dev,name==0?"consoleng.device":name==1?"console.device":"serial.device",size);
        count++;
    }
    printf("PASS %d cooked/raw/device read boundary and accounting cases\n",count);
}
'''
with tempfile.TemporaryDirectory(prefix='aros-cooked-read-') as temp:
    path = Path(temp)
    (path / 'test.c').write_text(prefix + answer + read + suffix)
    subprocess.run(['cc', '-Wall', '-Wextra', '-g', '-fsanitize=address,undefined',
                    str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
