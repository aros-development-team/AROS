/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Guest argument APIs: the actual GetArgStr and SetArgStr thunks, the
    Process lookup they use, the shadow registry and the memory accessors of
    rom/m68kemu, extracted by run_argument_apis.py, with controlled native
    collaborators.
*/
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
typedef uint8_t UBYTE;
typedef uint16_t UWORD;
typedef uint32_t ULONG;
typedef int32_t LONG;
typedef uintptr_t IPTR;
typedef uint64_t BPTR;
typedef int BOOL;
typedef void *APTR;
typedef char *STRPTR;
typedef const char *CONST_STRPTR;
#define TRUE 1
#define FALSE 0
#include "constants.inc"
struct Node { UBYTE ln_Type; };
struct Task { struct Node tc_Node; };
struct MsgPort { int unused; };
struct Process { struct Task pr_Task; BPTR pr_CLI; struct MsgPort pr_MsgPort; const char *pr_Arguments; };
struct M68KStructLayout { UWORD shadow_type; };
struct M68KEmuContext {
    UBYTE *mem; ULONG mem_size,m68k_sysbase,m68k_argptr,m68k_argsize,m68k_proc,seg_start; BOOL low_guard,shadow_full_logged;
    struct Task *emu_task; UWORD num_shadows; char program_name[32];
    struct { ULONG m68k_addr; void *native_ptr; UWORD type; UBYTE streams_valid; BPTR stream_native[3]; ULONG stream_handle[3]; } shadow_map[M68KEMU_MAX_SHADOWS];
};
static ULONG cpu_d[8];
#define THUNK_D(n) (((ULONG *)cpu)[n])
static unsigned checks,failures,native_gets,native_sets,low_traps,logs,heap_allocs,heap_frees;
static LONG io_error;
static BOOL missing_task_layout,missing_port_layout,heap_fail;
static ULONG heap_next,heap_freed_addr,heap_freed_size;
static char native_initial[]="native args are separate";
static char native_child[]="native child args";
static const char *native_args;
static struct Process root,child,other,third;
static struct Task plain;
static struct Task *current;
static struct M68KStructLayout task_layout={1},port_layout={4};
static LONG SetIoErr(LONG n) { LONG old=io_error;io_error=n;return old; }
static void m68k_low_write_trap(struct M68KEmuContext *ctx,ULONG addr,const char *why) { (void)ctx;(void)addr;(void)why;++low_traps; }
static void m68k_log(struct M68KEmuContext *ctx,const char *fmt,...) { (void)ctx;(void)fmt;++logs; }
#include "accessors.inc"
#include "registry.inc"
static const struct M68KStructLayout *shadow_find_layout(const char *name) {
    if(!strcmp(name,"Task"))return missing_task_layout?NULL:&task_layout;
    assert(!strcmp(name,"MsgPort"));
    return missing_port_layout?NULL:&port_layout;
}
static ULONG M68KEmu_HeapAlloc(struct M68KEmuContext *ctx,ULONG n,ULONG flags) {
    ULONG a=heap_next;(void)flags;++heap_allocs;if(heap_fail)return 0;
    n=(n+3)&~3u;assert(a+n<=0xF00&&a+n<=ctx->mem_size);heap_next+=n;return a;
}
static void M68KEmu_HeapFree(struct M68KEmuContext *ctx,ULONG a,ULONG n) { (void)ctx;++heap_frees;heap_freed_addr=a;heap_freed_size=n; }
/* The Task field sync, the stack bounds and the stream fields are not under test here. */
static void shadow_sync(struct M68KEmuContext *ctx,const struct M68KStructLayout *layout,ULONG a,void *native) { (void)ctx;(void)layout;(void)a;(void)native; }
static void m68k_task_stack(struct M68KEmuContext *ctx,ULONG a,struct Task *task) { (void)ctx;(void)a;(void)task; }
static void m68k_process_streams(struct M68KEmuContext *ctx,ULONG a,struct Task *task) { (void)ctx;(void)a;(void)task; }
static struct Task *FindTask(const void *name) { (void)name;return current; }
#include "process.inc"
#include "lifetime.inc"
static char *native_get(struct M68KEmuContext *ctx,void *cpu) { (void)ctx;(void)cpu;++native_gets;return (char *)native_args; }
static char *native_set(const char *s,void *cpu) { const char *old=native_args;(void)cpu;++native_sets;native_args=s;return (char *)old; }
#define GetArgStr() native_get(ctx,cpu)
#define SetArgStr(s) native_set((s),cpu)
#include "api.inc"
/* Every failed check is printed: the limit is above the number of checks. */
static void check(int ok,const char *label) { ++checks;if(!ok){if(failures<200)printf("FAIL: %s\n",label);++failures;} }
static void raw32(struct M68KEmuContext *ctx,ULONG a,ULONG v) { assert(a<=ctx->mem_size-4);ctx->mem[a]=(UBYTE)(v>>24);ctx->mem[a+1]=(UBYTE)(v>>16);ctx->mem[a+2]=(UBYTE)(v>>8);ctx->mem[a+3]=(UBYTE)v; }
/* The root Process is named both ways, as the launch names it: registered
   for the current native task and published in the guest ThisTask. */
static void reset_at(struct M68KEmuContext *ctx,ULONG proc) {
    memset(ctx->mem,0,ctx->mem_size);ctx->m68k_sysbase=0x400;ctx->m68k_argptr=0x600;ctx->m68k_argsize=8;ctx->low_guard=TRUE;
    memset(ctx->shadow_map,0,sizeof(ctx->shadow_map));ctx->num_shadows=0;ctx->shadow_full_logged=FALSE;ctx->seg_start=0;ctx->program_name[0]=0;
    memset(&root,0,sizeof(root));memset(&child,0,sizeof(child));memset(&other,0,sizeof(other));memset(&third,0,sizeof(third));
    root.pr_Task.tc_Node.ln_Type=child.pr_Task.tc_Node.ln_Type=other.pr_Task.tc_Node.ln_Type=third.pr_Task.tc_Node.ln_Type=NT_PROCESS;
    plain.tc_Node.ln_Type=1;root.pr_Arguments=native_initial;child.pr_Arguments=native_child;
    current=&root.pr_Task;ctx->emu_task=current;ctx->m68k_proc=proc;
    if(shadow_register(ctx,proc,&root.pr_Task,task_layout.shadow_type)!=proc)abort();
    raw32(ctx,0x400+M68K_EXECBASE_THISTASK,proc);if(proc<=ctx->mem_size-(M68K_PR_ARGUMENTS+4))raw32(ctx,proc+M68K_PR_ARGUMENTS,0x600);
    memcpy(ctx->mem+0x600,"--help \n",9);memcpy(ctx->mem+0x700,"changed",8);
    native_args=native_initial;native_gets=native_sets=low_traps=logs=heap_allocs=heap_frees=0;io_error=73;memset(cpu_d,0,sizeof(cpu_d));
    missing_task_layout=missing_port_layout=heap_fail=FALSE;heap_next=0xA00;heap_freed_addr=heap_freed_size=0;
}
static void reset(struct M68KEmuContext *ctx) { reset_at(ctx,0x800); }
static IPTR get(struct M68KEmuContext *ctx) { return API_GET(ctx,cpu_d); }
static IPTR set(struct M68KEmuContext *ctx,ULONG s) {cpu_d[1]=s;return API_SET(ctx,cpu_d);}
static void test_semantics(struct M68KEmuContext *ctx) {
    unsigned before=failures;
    reset(ctx);check(get(ctx)==0x600,"GetArgStr returns guest Process pointer");check(io_error==73,"successful getter preserves IoErr");
    raw32(ctx,0x800+M68K_PR_ARGUMENTS,0x700);check(get(ctx)==0x700,"getter sees direct guest Process mutation");
    check(set(ctx,0x600)==0x700,"setter returns exact old guest pointer");check(m68k_read32(ctx,0x800+M68K_PR_ARGUMENTS)==0x600,"setter publishes new guest pointer");
    check(get(ctx)==0x600,"get after set preserves identity");check(set(ctx,0x600)==0x600,"aliased replacement returns same pointer");
    check(set(ctx,0)==0x600,"NULL replacement returns old pointer");check(get(ctx)==0,"NULL Process argument is legal");check(io_error==73,"successful setters preserve IoErr");
    check(set(ctx,0x700)==0,"setter from NULL returns NULL");check(get(ctx)==0x700,"setter from NULL publishes value");
    ctx->mem[4095]=0;check(set(ctx,4095)==0x700,"last byte empty string accepted");check(get(ctx)==4095,"last byte pointer identity retained");
    reset_at(ctx,4096-(M68K_PR_ARGUMENTS+4));
    check(get(ctx)==0x600,"Process argument field may end exactly at arena boundary");check(set(ctx,0x700)==0x600&&m68k_read32(ctx,4092)==0x700,"boundary setter writes one full field");
    printf("GROUP semantics: %u failed\n",failures-before);
}
static void test_bounds(struct M68KEmuContext *ctx) {
    unsigned before=failures,i;static const ULONG invalid[]={4096,UINT32_MAX,4095};
    for(i=0;i<3;i++) {
        reset(ctx);ctx->mem[4095]='x';raw32(ctx,0x800+M68K_PR_ARGUMENTS,invalid[i]);
        check(get(ctx)==0&&io_error==ERROR_BAD_NUMBER,"invalid or unterminated old string rejected");
        io_error=73;check(set(ctx,0x700)==0&&io_error==ERROR_BAD_NUMBER,"setter rejects invalid old string");
        check(m68k_read32(ctx,0x800+M68K_PR_ARGUMENTS)==invalid[i],"invalid-old rejection does not mutate Process");
        reset(ctx);ctx->mem[4095]='x';check(set(ctx,invalid[i])==0&&io_error==ERROR_BAD_NUMBER,"invalid or unterminated new string rejected");
        check(m68k_read32(ctx,0x800+M68K_PR_ARGUMENTS)==0x600,"invalid-new rejection leaves old pointer");
    }
    /* The guest ExecBase and its ThisTask do not select the Process: the
       calling native task does. A bad value in either changes nothing. */
    for(i=0;i<5;i++) {
        reset(ctx);
        if(i==0)ctx->m68k_sysbase=0;
        if(i==1)ctx->m68k_sysbase=UINT32_MAX-100;
        if(i==2)raw32(ctx,0x400+M68K_EXECBASE_THISTASK,0);
        if(i==3)raw32(ctx,0x400+M68K_EXECBASE_THISTASK,UINT32_MAX-100);
        if(i==4)ctx->m68k_sysbase=4096-M68K_EXECBASE_THISTASK-3;
        check(get(ctx)==0x600&&io_error==73,"bad SysBase or ThisTask does not matter: getter returns the caller's own argument");
        check(set(ctx,0x700)==0x600&&m68k_read32(ctx,0x800+M68K_PR_ARGUMENTS)==0x700&&io_error==73,"bad SysBase or ThisTask does not matter: setter writes the caller's own field");
        check(!low_traps,"bad SysBase or ThisTask never reaches low-write trap");
    }
    reset_at(ctx,4096-M68K_PR_ARGUMENTS-3);
    check(get(ctx)==0&&io_error==ERROR_BAD_NUMBER,"bad Process range rejected");
    io_error=73;check(set(ctx,0x700)==0&&io_error==ERROR_BAD_NUMBER,"setter validates entire Process field before addition");
    check(!low_traps,"invalid range never reaches low-write trap");
    reset_at(ctx,0x300-M68K_PR_ARGUMENTS);
    check(set(ctx,0x700)==0&&io_error==ERROR_BAD_NUMBER,"protected low-memory field rejected before mutation");
    check(m68k_read32(ctx,0x300)==0x600&&!low_traps,"low-memory rejection retains data without a trap");
    printf("GROUP bounds: %u failed\n",failures-before);
}
static void test_ownership(struct M68KEmuContext *ctx) {
    unsigned before=failures,i;static const ULONG replacements[]={0x700,0,UINT32_MAX};
    for(i=0;i<3;i++) {
        reset(ctx);(void)get(ctx);(void)set(ctx,replacements[i]);
        check(native_args==native_initial,"guest setter never replaces native Process argument pointer");
        check(!native_gets&&!native_sets,"guest APIs never expose or call native argument accessors");
        check(ctx->m68k_argptr==0x600&&ctx->m68k_argsize==8,"startup registers remain immutable");
        check(!strcmp((char *)ctx->mem+0x600,"--help \n"),"startup argument bytes retain lifetime");
    }
    printf("GROUP ownership: %u failed\n",failures-before);
}
/* A process made as create_proc() makes one: by its parent, inside the
   parent's call, before the new process runs. */
static ULONG create_process(struct M68KEmuContext *ctx,struct Process *p) {
#if HAVE_PROCESS_FORGET
    m68k_process_forget(ctx,&p->pr_Task);
#endif
    return m68k_process_of(ctx,&p->pr_Task);
}
static ULONG guest_of(struct M68KEmuContext *ctx,struct Task *task) {
    unsigned i;
    for(i=0;i<ctx->num_shadows;i++)if(ctx->shadow_map[i].native_ptr==task&&ctx->shadow_map[i].type==task_layout.shadow_type)return ctx->shadow_map[i].m68k_addr;
    return 0;
}
static ULONG arg_of(struct M68KEmuContext *ctx,ULONG proc) { return m68k_read32(ctx,proc+M68K_PR_ARGUMENTS); }
static IPTR get_as(struct M68KEmuContext *ctx,struct Task *task) { current=task;return get(ctx); }
static IPTR set_as(struct M68KEmuContext *ctx,struct Task *task,ULONG s) { current=task;return set(ctx,s); }
static void test_tasks(struct M68KEmuContext *ctx) {
    static const unsigned char order[6][3]={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
    const ULONG B=0x740,C=0x760,D=0x780;
    unsigned before=failures,i,k,wrong=0,shadows;ULONG pc,po,pt,p2,want[3];IPTR r;struct Task *who[3];
    /* a parent and two children, each with its own argument field */
    reset(ctx);memcpy(ctx->mem+B,"child-b",8);memcpy(ctx->mem+C,"root-c",7);memcpy(ctx->mem+D,"other-d",8);
    check(get_as(ctx,&root.pr_Task)==0x600&&io_error==73,"root reads its launch argument");
    pc=create_process(ctx,&child);
    check(pc&&pc!=0x800&&shadow_lookup(ctx,pc)==&child.pr_Task,"a child gets a guest Process of its own");
    check(pc&&arg_of(ctx,pc)==0,"a child's argument field starts NULL");
    r=get_as(ctx,&child.pr_Task);
    check(r==0&&io_error==73,"a child reads its own NULL argument, not its parent's");
    check(get_as(ctx,&root.pr_Task)==0x600,"the parent still reads its own argument");
    r=set_as(ctx,&child.pr_Task,B);
    check(r==0&&pc&&arg_of(ctx,pc)==B,"a child's setter returns its own old NULL and writes its own field");
    check(arg_of(ctx,0x800)==0x600,"a child's setter leaves its parent's field");
    r=set_as(ctx,&root.pr_Task,C);
    check(r==0x600&&arg_of(ctx,0x800)==C,"the parent's setter returns and replaces its own argument");
    check(pc&&arg_of(ctx,pc)==B&&get_as(ctx,&child.pr_Task)==B,"the parent's setter leaves its child's field");
    if(pc)raw32(ctx,pc+M68K_PR_ARGUMENTS,0x700);
    check(get_as(ctx,&child.pr_Task)==0x700&&get_as(ctx,&root.pr_Task)==C,"a direct write into a child's field is seen by that child only");
    if(pc)raw32(ctx,pc+M68K_PR_ARGUMENTS,B);
    po=create_process(ctx,&other);
    check(po&&po!=pc&&po!=0x800&&arg_of(ctx,po)==0,"a second child has its own NULL field");
    r=set_as(ctx,&other.pr_Task,D);
    check(r==0&&po&&arg_of(ctx,po)==D&&pc&&arg_of(ctx,pc)==B&&arg_of(ctx,0x800)==C,"three processes keep three arguments");
    who[0]=&root.pr_Task;who[1]=&child.pr_Task;who[2]=&other.pr_Task;want[0]=C;want[1]=B;want[2]=D;
    for(i=0;i<6;i++)for(k=0;k<3;k++)if(get_as(ctx,who[order[i][k]])!=want[order[i][k]])++wrong;
    check(!wrong,"each process reads its own argument in every order of calls");
    r=set_as(ctx,&child.pr_Task,C);
    check(r==B&&get_as(ctx,&child.pr_Task)==C&&get_as(ctx,&root.pr_Task)==C,"two processes may name the same string");
    r=set_as(ctx,&root.pr_Task,0x700);
    check(r==C&&get_as(ctx,&child.pr_Task)==C&&get_as(ctx,&root.pr_Task)==0x700,"changing one of them leaves the other");
    r=set_as(ctx,&other.pr_Task,0);
    check(r==D&&get_as(ctx,&other.pr_Task)==0&&get_as(ctx,&child.pr_Task)==C&&get_as(ctx,&root.pr_Task)==0x700,"NULL set in one process leaves the others");
    /* the guest ThisTask may name any Process: the calling task decides */
    if(pc)raw32(ctx,0x400+M68K_EXECBASE_THISTASK,pc);
    check(get_as(ctx,&root.pr_Task)==0x700,"ThisTask naming a child: the root still reads its own argument");
    r=set_as(ctx,&root.pr_Task,0x600);
    check(r==0x700&&arg_of(ctx,0x800)==0x600&&pc&&arg_of(ctx,pc)==C,"ThisTask naming a child: the root still writes its own field");
    raw32(ctx,0x400+M68K_EXECBASE_THISTASK,0x800);
    check(get_as(ctx,&child.pr_Task)==C,"ThisTask naming the root: a child still reads its own argument");
    /* a task that the registry sees for the first time inside the call */
    reset(ctx);shadows=ctx->num_shadows;
    r=get_as(ctx,&third.pr_Task);pt=guest_of(ctx,&third.pr_Task);
    check(r==0&&io_error==73&&pt&&pt!=0x800&&heap_allocs==1,"a task first seen inside the call gets one guest Process");
    (void)get_as(ctx,&third.pr_Task);
    check(heap_allocs==1&&pt&&guest_of(ctx,&third.pr_Task)==pt&&ctx->num_shadows==shadows+2,"the next call finds the same guest Process, registered once with its port");
    r=set_as(ctx,&third.pr_Task,0x700);
    check(r==0&&pt&&arg_of(ctx,pt)==0x700&&arg_of(ctx,0x800)==0x600,"it sets its own field and not the root's");
    reset(ctx);
    r=get_as(ctx,&plain);pt=guest_of(ctx,&plain);
    check(r==0&&io_error==73&&pt&&pt!=0x800,"a plain Task gets its own guest structure");
    r=set_as(ctx,&plain,0x700);
    check(r==0&&pt&&arg_of(ctx,pt)==0x700&&get_as(ctx,&plain)==0x700&&arg_of(ctx,0x800)==0x600,"a plain Task sets and reads its own field");
    /* no guest Process for the caller */
    reset(ctx);missing_task_layout=TRUE;
    check(get_as(ctx,&root.pr_Task)==0&&io_error==ERROR_BAD_NUMBER,"no Task layout: the getter is rejected");
    io_error=73;check(set_as(ctx,&root.pr_Task,0x700)==0&&io_error==ERROR_BAD_NUMBER&&arg_of(ctx,0x800)==0x600,"no Task layout: the setter is rejected and changes nothing");
    reset(ctx);heap_fail=TRUE;shadows=ctx->num_shadows;
    check(get_as(ctx,&third.pr_Task)==0&&io_error==ERROR_BAD_NUMBER,"no guest memory at first sight: the getter is rejected");
    io_error=73;check(set_as(ctx,&third.pr_Task,0x700)==0&&io_error==ERROR_BAD_NUMBER,"no guest memory at first sight: the setter is rejected");
    check(ctx->num_shadows==shadows&&arg_of(ctx,0x800)==0x600&&!heap_frees,"a failed first sight registers nothing and changes no field");
    reset(ctx);ctx->num_shadows=M68KEMU_MAX_SHADOWS;
    check(get_as(ctx,&third.pr_Task)==0&&io_error==ERROR_BAD_NUMBER,"full registry at first sight: the getter is rejected");
    check(heap_allocs==1&&heap_frees==1&&heap_freed_addr==0xA00&&heap_freed_size==M68K_SIZEOF_PROCESS,"full registry at first sight: the guest block is rolled back");
    io_error=73;check(set_as(ctx,&third.pr_Task,0x700)==0&&io_error==ERROR_BAD_NUMBER,"full registry at first sight: the setter is rejected");
    check(ctx->num_shadows==M68KEMU_MAX_SHADOWS&&arg_of(ctx,0x800)==0x600&&logs==1,"full registry: nothing is registered, no field changes, the refusal is logged once");
    /* one slot left: a Process needs two entries, its own and its port's; a plain Task needs one */
    reset(ctx);(void)get_as(ctx,&root.pr_Task);ctx->num_shadows=M68KEMU_MAX_SHADOWS-1;
    check(get_as(ctx,&third.pr_Task)==0&&io_error==ERROR_BAD_NUMBER,"one registry slot left: a Process first seen gets no guest Process");
    check(heap_allocs==1&&heap_frees==1&&heap_freed_addr==0xA00&&heap_freed_size==M68K_SIZEOF_PROCESS&&ctx->num_shadows==M68KEMU_MAX_SHADOWS-1&&!guest_of(ctx,&third.pr_Task)&&logs==1,"one slot left: its Task entry is taken back and its guest block released, no Process without its port stays");
    check(arg_of(ctx,0x800)==0x600&&shadow_lookup(ctx,0x800)==&root.pr_Task&&shadow_lookup(ctx,0x800+M68K_PR_MSGPORT)==&root.pr_MsgPort,"one slot left: the parent's entries and argument are untouched");
    io_error=73;r=get_as(ctx,&plain);pt=guest_of(ctx,&plain);
    check(r==0&&io_error==73&&pt&&ctx->num_shadows==M68KEMU_MAX_SHADOWS,"one slot left: a plain Task still takes it");
    ctx->num_shadows=M68KEMU_MAX_SHADOWS-2;io_error=73;r=get_as(ctx,&third.pr_Task);pt=guest_of(ctx,&third.pr_Task);
    check(r==0&&io_error==73&&pt&&shadow_lookup(ctx,pt)==&third.pr_Task&&shadow_lookup(ctx,pt+M68K_PR_MSGPORT)==&third.pr_MsgPort&&ctx->num_shadows==M68KEMU_MAX_SHADOWS,"two slots: the Process and its port are registered");
    /* no MsgPort layout: a new Process cannot register its port */
    reset(ctx);(void)get_as(ctx,&root.pr_Task);shadows=ctx->num_shadows;missing_port_layout=TRUE;
    {
    const char *label="without a MsgPort layout a new Process is taken back whole (a tree that reads through the missing layout is not called, and fails here)";
#if PROCESS_PORT_LAYOUT_CHECKED
    r=get_as(ctx,&third.pr_Task);
    check(r==0&&io_error==ERROR_BAD_NUMBER&&heap_frees==1&&heap_freed_size==M68K_SIZEOF_PROCESS&&ctx->num_shadows==shadows&&!guest_of(ctx,&third.pr_Task),label);
#else
    /* This tree reads through the missing layout: report failure without calling it. */
    check(FALSE,label);
#endif
    }
    missing_port_layout=FALSE;
    /* a process ends; a new native process has its address */
    reset(ctx);memcpy(ctx->mem+B,"child-b",8);memcpy(ctx->mem+D,"other-d",8);
    (void)get_as(ctx,&root.pr_Task);pc=create_process(ctx,&child);po=create_process(ctx,&other);
    (void)set_as(ctx,&child.pr_Task,B);(void)set_as(ctx,&other.pr_Task,D);
    current=&root.pr_Task;memset(&child,0,sizeof(child));child.pr_Task.tc_Node.ln_Type=NT_PROCESS;child.pr_Arguments=native_child;
    shadows=ctx->num_shadows;p2=create_process(ctx,&child);
    check(p2&&p2!=pc,"a new process at an ended one's native address gets a new guest Process");
    check(p2&&arg_of(ctx,p2)==0&&get_as(ctx,&child.pr_Task)==0,"it starts with a NULL argument, not the ended one's");
    check(pc&&arg_of(ctx,pc)==B,"the ended one's guest structure keeps its bytes");
    check(p2&&shadow_lookup(ctx,p2)==&child.pr_Task&&shadow_lookup(ctx,p2+M68K_PR_MSGPORT)==&child.pr_MsgPort,"the new process and its port are registered");
    check(pc&&p2!=pc&&!shadow_lookup(ctx,pc)&&!shadow_lookup(ctx,pc+M68K_PR_MSGPORT),"the ended one's Process and port are no longer found");
    check(ctx->num_shadows==shadows,"the registry does not grow at a reused address");
    check(shadow_lookup(ctx,0x800)==&root.pr_Task&&shadow_lookup(ctx,0x800+M68K_PR_MSGPORT)==&root.pr_MsgPort&&po&&shadow_lookup(ctx,po)==&other.pr_Task&&shadow_lookup(ctx,po+M68K_PR_MSGPORT)==&other.pr_MsgPort,"forgetting the ended one leaves every other live entry");
    check(get_as(ctx,&other.pr_Task)==D&&get_as(ctx,&root.pr_Task)==0x600,"and the arguments of the other processes");
    check(!native_gets&&!native_sets&&native_args==native_initial,"no call of any task reaches the native argument accessors");
    check(root.pr_Arguments==native_initial&&child.pr_Arguments==native_child,"native Process argument pointers are untouched");
    printf("GROUP tasks: %u failed\n",failures-before);
}
int main(void) {
    struct M68KEmuContext ctx={0};ctx.mem_size=4096;ctx.mem=malloc(ctx.mem_size);assert(ctx.mem);
    test_semantics(&ctx);test_bounds(&ctx);test_ownership(&ctx);test_tasks(&ctx);free(ctx.mem);
    printf("argument APIs: %u checks, %u failed\n",checks,failures);return failures?1:0;
}
