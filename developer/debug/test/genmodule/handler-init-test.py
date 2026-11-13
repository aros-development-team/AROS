#!/usr/bin/env python3
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
"""Execute emitted handler initialization with instrumented OS services."""
import argparse
import json
from pathlib import Path
import shlex
import subprocess
import tempfile

PREFIX = r'''
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef uint32_t ULONG;
typedef int32_t LONG;
typedef int8_t BYTE;
typedef uint8_t UBYTE;
typedef uintptr_t BPTR;
typedef intptr_t SIPTR;
typedef intptr_t IPTR;
typedef uintptr_t BSTR;
typedef void *APTR;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define BNULL 0
#define MEMF_CLEAR 1
#define FSEF_SEGLIST 1
#define FSEF_GLOBALVEC 2
#define FSEF_PRIORITY 4
#define FSEF_STACKSIZE 8
#define FSEF_HANDLER 16
#define VERSION_STRING "test 9.7"
#define MAJOR_VERSION 9
#define MINOR_VERSION 7
#define CMD_SYSTEM -2
#define GM_UNIQUENAME(x) test_##x
#ifdef TEST_FAST_BSTR
#define AROS_CONST_BSTR(s) ((BSTR)(s))
#define AROS_BSTR_ADDR(s) ((const char *)(s))
#else
#define AROS_CONST_BSTR(s) ({ static const struct { UBYTE len; char str[sizeof(s)]; } b __attribute__((aligned(4))) = {sizeof(s)-1,s}; (BSTR)((uintptr_t)&b>>2); })
#define AROS_BSTR_ADDR(s) ((const char *)((uintptr_t)(s)<<2)+1)
#endif
struct ExecBase {int unused;};
struct Library {int unused;};
struct DosLibrary {int unused;};
struct Node {char *ln_Name; BYTE ln_Pri;};
struct List {int unused;};
struct FileSysResource {struct List fsr_FileSysEntries;};
struct FileSysEntry {struct Node fse_Node; ULONG fse_DosType,fse_Version,fse_PatchFlags,fse_StackSize; BSTR fse_Handler; LONG fse_Priority; BPTR fse_SegList,fse_GlobalVec,fse_Startup;};
struct DeviceNode {ULONG dn_StackSize; BPTR dn_SegList,dn_Startup,dn_GlobalVec; LONG dn_Priority;};
static struct FileSysResource resource;
static struct DosLibrary dos;
static struct Library expansion;
static int missing_resource,missing_segment,missing_dos,fail_allocation;
static int allocations,forbids,permits,opens,closes,segments;
static struct FileSysEntry entries[32];
static char names[32][64];
static int count;
static void test_Handler(void) {}
static APTR OpenResource(const char *name) {return missing_resource ? NULL : &resource;}
static BPTR CreateSegList(void (*entry)(void)) {return missing_segment ? BNULL : (BPTR)123;}
static APTR AllocMem(size_t size,ULONG flags) {return ++allocations==fail_allocation ? NULL : calloc(1,size);}
static void Forbid(void) {forbids++;}
static void Permit(void) {permits++;}
static void Enqueue(struct List *list,struct Node *node) {
 struct FileSysEntry *entry=(struct FileSysEntry *)node;
 entries[count]=*entry;
 if(entry->fse_Handler)snprintf(names[count],sizeof(names[count]),"%s",AROS_BSTR_ADDR(entry->fse_Handler));
 count++;
 free(entry);
}
static APTR OpenLibrary(const char *name,int version) {
 if(!strcmp(name,"expansion.library"))return &expansion;
 opens++;return missing_dos ? NULL : &dos;
}
static void CloseLibrary(APTR base) {if(base==&dos)closes++;}
static void AddSegment(const char *name,BPTR seg,LONG mode) {segments++;}
static struct DeviceNode *MakeDosNode(APTR p) {return calloc(1,sizeof(struct DeviceNode));}
static void AddBootNode(LONG priority,ULONG flags,struct DeviceNode *node,APTR config) {free(node);}
'''
SUFFIX = r'''
int main(int argc,char **argv) {
 struct ExecBase exec;
 missing_resource=atoi(argv[1]);missing_segment=atoi(argv[2]);missing_dos=atoi(argv[3]);fail_allocation=atoi(argv[4]);
 int result=test_InitHandler(&exec);
 printf("{\"result\":%d,\"allocations\":%d,\"forbids\":%d,\"permits\":%d,\"opens\":%d,\"closes\":%d,\"segments\":%d,\"entries\":[",result,allocations,forbids,permits,opens,closes,segments);
 for(int i=0;i<count;i++) {
  struct FileSysEntry *e=&entries[i];
  printf("%s{\"id\":%u,\"detect\":%d,\"version\":%u,\"flags\":%u,\"stack\":%u,\"priority\":%d,\"startup\":%lld,\"seg\":%llu,\"name\":\"%s\"}",i?",":"",e->fse_DosType,e->fse_Node.ln_Pri,e->fse_Version,e->fse_PatchFlags,e->fse_StackSize,e->fse_Priority,(long long)(SIPTR)e->fse_Startup,(unsigned long long)e->fse_SegList,names[i]);
 }
 puts("]}");return 0;
}
'''


def emitted_initialization(source):
    marker='static inline BOOL GM_UNIQUENAME(InitHandler)'
    pos=source.index(marker)
    begin=pos
    for candidate in ('static BSTR GM_UNIQUENAME(HandlerName','static const struct GM_UNIQUENAME(HandlerInfo)'):
        if candidate in source:begin=min(begin,source.index(candidate))
    brace=source.index('{',pos)
    depth=1;end=brace+1
    while depth:
        depth += (source[end]=='{')-(source[end]=='}');end+=1
    return source[begin:end]


def check_registration(genmodule,cc,config,expected,root):
    config_path=root/'handler.conf';config_path.write_text('##begin config\nversion 9.7\nhandler_func test_Handler\n##end config\n##begin handler\n'+config+'\n##end handler\n')
    subprocess.run([genmodule,'-c',str(config_path),'-d',str(root),'writefiles','sample','handler'],check=True,capture_output=True)
    emitted=emitted_initialization((root/'sample_start.c').read_text())
    for fast in (False,True):
        src=root/'generated-test.c';src.write_text(PREFIX+emitted+SUFFIX)
        exe=root/('test-fast' if fast else 'test-counted')
        subprocess.run(shlex.split(cc)+['-std=gnu99','-w',*(['-DTEST_FAST_BSTR'] if fast else []),str(src),'-o',str(exe)],check=True,capture_output=True)
        for no_resource,no_segment,no_dos,fail_alloc in ((0,0,0,0),(1,0,0,0),(0,1,0,0),(0,0,1,0),(0,0,0,1),(0,0,0,2)):
            result=json.loads(subprocess.check_output([str(exe),str(no_resource),str(no_segment),str(no_dos),str(fail_alloc)],text=True))
            wanted=[] if no_resource or no_segment else [entry for i,entry in enumerate(expected,1) if i!=fail_alloc]
            assert result['entries']==wanted,(fast,result['entries'],wanted)
            assert result['result']==(0 if no_resource else 1)
            assert result['forbids']==result['permits']==len(wanted)
            named=sum(bool(e['name']) for e in expected)
            if not no_resource and not no_segment:
                assert result['allocations']==len(expected)
                assert result['opens']==named
                assert result['segments']==(0 if no_dos else named)
                assert result['closes']==(0 if no_dos else named)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--genmodule',required=True)
    parser.add_argument('--cc',default='cc')
    args=parser.parse_args()
    def entry(id,detect,stack,priority,startup,name):
        return dict(id=id,detect=detect,version=(9<<16)|7,flags=7+(8 if stack else 0)+(16 if name else 0),stack=stack,priority=priority,startup=startup,seg=123,name=name)
    cases=[
      ('.autodetect 90\nDosType=0x444f5301\nResident=SharedFS\nDosType=0x444f5300',
       [entry(0x444f5300,88,0,10,0,''),entry(0,89,0,10,0,'SharedFS'),entry(0x444f5301,90,0,10,0,'')]),
      ('.autodetect 50\n.priority -5\n.stacksize 65536\n.startup -9\nDosType=0x444f5301\nResident=VarFS\n.priority 20\n.stacksize 0\n.startup 123\nDosType=0x444f5300',
       [entry(0x444f5300,48,0,20,123,''),entry(0,49,65536,-5,-9,'VarFS'),entry(0x444f5301,50,65536,-5,-9,'')]),
      ('.autodetect 5\n.stacksize 8192\n.priority 2\n.startup 7\nResident=SingleFS',
       [entry(0,5,8192,2,7,'SingleFS')]),
    ]
    with tempfile.TemporaryDirectory() as tmp:
        for config,expected in cases:check_registration(args.genmodule,args.cc,config,expected,Path(tmp))
    print('GENMODULE HANDLER TEST: 36 registration/failure cases passed (counted and fast BSTR)')


if __name__=='__main__':main()
