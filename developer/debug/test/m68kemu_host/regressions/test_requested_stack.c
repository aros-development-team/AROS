/* Copyright (C) 2026, The AROS Development Team. All rights reserved.
   Recording standalone fixture adapted from test_memheader.c. Production
   constructors, heap functions, accessors and task-shadow writer are included
   verbatim. Native operations and context fields are controlled stand-ins;
   this is not a native ABI, lock, CPU or launch-translation-unit test. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef uint8_t UBYTE;
typedef uint16_t UWORD;
typedef uint32_t ULONG;
typedef unsigned char *STRPTR;
typedef const unsigned char *CONST_STRPTR;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define CACRF_EnableI 1
#define CACRF_EnableD 2
#define CACRF_CopyBack 4
static void fixture_bug(const char *format,...){(void)format;}
#define bug(...) fixture_bug(__VA_ARGS__)
#include "constants.inc"

struct M68KEmuLibBase { unsigned unused; };
struct M68KHeapRegion { ULONG start,end,free,avail,mh; };
struct Task { unsigned id; };
struct M68KEmuContext {
    UBYTE *mem; ULONG mem_size;
    struct M68KEmuLibBase *emuBase;
    ULONG stack_top,stack_bottom,heap_start,heap_end,cacr;
    struct M68KHeapRegion rgn[2];
    unsigned more_tasks,gfx_owned;
    struct { signed char semsig; } orphan;
    BOOL classic_borders,low_guard,trace_calls;
};
static struct { struct { UWORD lib_Version,lib_Revision; } LibNode; } execbase={{50,7}};
#define SysBase (&execbase)
static unsigned checks,failures,alloc_calls,free_calls,fail_at,gil_calls,hold_calls,list_calls,var_calls;
static struct { void *p; ULONG size; } blocks[8];
static unsigned live;
static struct Task current_task={1},other_task={2};
static unsigned find_calls;
static void check(int ok,const char *label) {
    ++checks;if(!ok){if(failures<32)printf("FAIL: %s\n",label);++failures;}
}
static void *record_alloc(ULONG size,ULONG flags) {
    void *p;unsigned i;++alloc_calls;
    if(alloc_calls==fail_at)return NULL;
    p=(flags&MEMF_CLEAR)?calloc(1,size):malloc(size);assert(p);
    for(i=0;i<8&&blocks[i].p;i++){}
    assert(i<8);blocks[i].p=p;blocks[i].size=size;++live;return p;
}
static void record_free(void *p,ULONG size) {
    unsigned i;for(i=0;i<8&&blocks[i].p!=p;i++){}
    assert(i<8&&blocks[i].size==size&&live);blocks[i].p=NULL;--live;++free_calls;free(p);
}
#define AllocMem(size,flags) record_alloc((size),(flags))
#define FreeMem(p,size) record_free((p),(size))
#define NEWLIST(p) ((void)(p),++list_calls)
static int GetVar(CONST_STRPTR name,STRPTR value,ULONG length,ULONG flags) {
    (void)name;(void)value;(void)length;(void)flags;++var_calls;return -1;
}
static void m68k_gil_init(struct M68KEmuContext *c){(void)c;++gil_calls;}
static void m68k_context_hold(struct M68KEmuContext *c){(void)c;++hold_calls;}
static struct Task *FindTask(const void *name){(void)name;++find_calls;return &current_task;}
static void m68k_low_write_trap(struct M68KEmuContext *c,ULONG a,const char *s){(void)c;(void)a;(void)s;}
#include "accessors.inc"
#include "production.inc"

static void reset(void) {
    assert(!live);memset(blocks,0,sizeof(blocks));
    alloc_calls=free_calls=fail_at=gil_calls=hold_calls=list_calls=var_calls=find_calls=0;
}
/* The baseline has no sized constructor. Calling its real constructor while
   ignoring the request reproduces its launch behavior without editing it. */
static struct M68KEmuContext *requested(struct M68KEmuLibBase *b,ULONG bytes) {
#if HAVE_SIZED_CONTEXT
    return M68KEmu_CreateContextWithStack(b,bytes);
#else
    (void)bytes;return M68KEmu_CreateContext(b);
#endif
}
/* Fixture cleanup only; production destructor/lock semantics are not tested. */
static void cleanup(struct M68KEmuContext *c) {
    if(c){record_free(c->mem,c->mem_size);record_free(c,(ULONG)sizeof(*c));}
    check(!live,"no host allocation left by fixture cleanup");
}
static void geometry(struct M68KEmuContext *c,ULONG expected) {
    const ULONG starts[]={M68KEMU_CHIP_END,M68KEMU_CIA_END,M68KEMU_CUSTOM_END,M68KEMU_ROM_END};
    ULONG ends[]={M68KEMU_CIA_BASE,M68KEMU_CUSTOM_BASE,M68KEMU_ROM_BASE,M68KEMU_MEM_SIZE-expected};
    uint64_t total=0;unsigned i;
    check(c->stack_top==33554432u&&c->stack_bottom==33554432u-expected,"requested stack bounds");
    check(c->heap_end==33554432u-expected&&c->rgn[RGN_FAST].end==c->heap_end,"heap and fast region end below requested stack");
    check(!(c->stack_bottom&3u),"heap boundary stays longword-aligned");
    check(c->rgn[RGN_CHIP].start==M68KEMU_HEAP_BASE&&c->rgn[RGN_CHIP].end==M68KEMU_CHIP_END,"chip region unchanged");
    check(c->rgn[RGN_CHIP].free==M68KEMU_HEAP_BASE&&m68k_read32(c,M68KEMU_HEAP_BASE+FB_NEXT)==0,"chip free list unchanged");
    check(c->rgn[RGN_CHIP].avail==M68KEMU_CHIP_END-M68KEMU_HEAP_BASE,"chip free bytes unchanged");
    check(c->rgn[RGN_FAST].free==starts[0],"fast region starts at its first chunk");
    for(i=0;i<4;i++) {
        check(m68k_read32(c,starts[i]+FB_NEXT)==(i==3?0:starts[i+1]),"fast links skip reserved regions");
        check(m68k_read32(c,starts[i]+FB_SIZE)==ends[i]-starts[i],"fast chunk ends before hole or stack");
        total+=(uint64_t)ends[i]-starts[i];
    }
    check(c->rgn[RGN_FAST].avail==total,"exact fast free-byte accounting");
    check(c->stack_bottom>=M68KEMU_ROM_END+16u,"ROM and terminal free chunk remain below stack");
    check(m68k_read32(c,M68KEMU_ROM_BASE)==0x11144ef9u&&m68k_read32(c,M68KEMU_ROM_END-20)==M68KEMU_ROM_SIZE,"ROM header retained");
}
static void heap_and_shadow(struct M68KEmuContext *c,ULONG expected) {
    UBYTE rom[16],stack[64],task[80];ULONG fast=c->rgn[RGN_FAST].avail,chip=c->rgn[RGN_CHIP].avail,p,q;
    memcpy(rom,c->mem+M68KEMU_ROM_BASE,16);memset(c->mem+c->stack_bottom,0xA5,64);memcpy(stack,c->mem+c->stack_bottom,64);
    p=M68KEmu_HeapAlloc(c,17,MEMF_FAST|MEMF_CLEAR);q=M68KEmu_HeapAlloc(c,1,MEMF_CHIP);
    check(p&&q,"normal fast/chip allocations still succeed");
    check(p&&p>=M68KEMU_CHIP_END&&(uint64_t)p+17<=c->stack_bottom,"fast payload stays below stack");
    check(q&&q<M68KEMU_CHIP_END,"chip request stays in chip memory");
    if(p){unsigned i;for(i=0;i<17;i++)check(c->mem[p+i]==0,"MEMF_CLEAR payload is zero");}
    check(!M68KEmu_HeapAllocAbs(c,8,c->stack_bottom),"AllocAbs cannot enter root stack");
    check(!M68KEmu_HeapAllocAbs(c,8,M68KEMU_ROM_BASE),"AllocAbs cannot enter ROM");
    M68KEmu_HeapFree(c,c->stack_bottom+8,8);
    if(p)M68KEmu_HeapFree(c,p,17);
    if(q)M68KEmu_HeapFree(c,q,1);
    check(c->rgn[RGN_FAST].avail==fast&&c->rgn[RGN_CHIP].avail==chip,"allocation/free restores both region counts");
    check(!memcmp(rom,c->mem+M68KEMU_ROM_BASE,16)&&!memcmp(stack,c->mem+c->stack_bottom,64),"heap operations preserve ROM and stack sentinel");
    memset(c->mem+0x8000,0xCC,80);memcpy(task,c->mem+0x8000,80);
    m68k_task_stack(c,0x8000,&other_task);check(!memcmp(task,c->mem+0x8000,80),"other task shadow is not overwritten");
    m68k_task_stack(c,0x8000,&current_task);
    check(m68k_read32(c,0x8000+54)==33554432u-4&&m68k_read32(c,0x8000+58)==33554432u-expected&&m68k_read32(c,0x8000+62)==33554432u,"actual task shadow publishes requested bounds");
}
static void legacy(void) {
    struct M68KEmuLibBase b={0};struct M68KEmuContext *c;unsigned before=failures;
    reset();c=M68KEmu_CreateContext(&b);check(c!=NULL,"legacy constructor succeeds");
    if(c){geometry(c,65536u);check(alloc_calls==2&&gil_calls==1&&hold_calls==1&&list_calls==2&&var_calls==1,"legacy initialization unchanged");cleanup(c);}
    printf("GROUP legacy: %u failed\n",failures-before);
}
static void requests(void) {
    const ULONG cases[][2]={{0,65536},{1,65536},{65535,65536},{65536,65536},{65537,65540},
      {1048576,1048576},{1048577,1048580},{1048578,1048580},{1048579,1048580},{1048580,1048580},
      {16777197,16777200},{16777200,16777200}};
    struct M68KEmuLibBase b={0};unsigned i,before=failures;
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);i++) {
        struct M68KEmuContext *c;reset();c=requested(&b,cases[i][0]);check(c!=NULL,"valid requested stack creates a context");
        if(!c)continue;
        geometry(c,cases[i][1]);heap_and_shadow(c,cases[i][1]);
        check(c->emuBase==&b&&c->classic_borders&&c->orphan.semsig==-1,"non-stack constructor state preserved");cleanup(c);
    }
    printf("GROUP requests: %u failed\n",failures-before);
}
static void invalids(void) {
    const ULONG bad[]={16777201u,16777203u,16777216u,16777217u,33554432u,0x7FFFFFFFu,0x80000000u,0xFFFFFFFDu,0xFFFFFFFEu,0xFFFFFFFFu};
    struct M68KEmuLibBase b={0};unsigned i,before=failures;
    for(i=0;i<sizeof(bad)/sizeof(bad[0]);i++) {
        struct M68KEmuContext *c;reset();c=requested(&b,bad[i]);
        check(!c,"invalid requested stack refused");
        check(!alloc_calls&&!gil_calls&&!hold_calls&&!list_calls&&!var_calls,"invalid stack rejected before native allocations and hooks");cleanup(c);
    }
    printf("GROUP invalids: %u failed\n",failures-before);
}
static void allocation_failures(void) {
    struct M68KEmuLibBase b={0};unsigned n,before=failures;
    for(n=1;n<=2;n++) {
        struct M68KEmuContext *c;reset();fail_at=n;c=requested(&b,1048576u);
        check(!c&&alloc_calls==n&&free_calls==n-1&&!live,"failed arena/context allocation rolls back");
        check(!gil_calls&&!hold_calls&&!list_calls&&!var_calls,"failed allocation cannot initialize context state");
    }
    printf("GROUP allocation-failures: %u failed\n",failures-before);
}
int main(void) {
    check(sizeof(ULONG)==4,"guest byte count is exactly32-bit");
    check(M68KEMU_MEM_SIZE==33554432u&&M68KEMU_ROM_END==16777216u&&M68KEMU_STACK_SIZE==65536u&&M68KEMU_HEAP_MIN_SPLIT==16,"fixture policy values match actual source");
    legacy();requests();invalids();allocation_failures();check(!live,"no live host allocations at test end");
    printf("requested root stack: %u checks, %u failed\n",checks,failures);return failures?1:0;
}
