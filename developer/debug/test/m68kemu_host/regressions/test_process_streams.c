/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Process streams: the actual launch, BPTR handle and Process projection
    code of rom/m68kemu, extracted by run_process_streams.py, with recording
    collaborators. This fixture does not model a native SDK layout, the CPU
    or child execution.
*/
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint32_t ULONG;
typedef uint16_t UWORD;
typedef uint8_t UBYTE;
typedef int32_t LONG;
typedef uint64_t IPTR;
typedef uint64_t BPTR;
typedef uint64_t Tag;
typedef void *APTR;
typedef const unsigned char *CONST_STRPTR;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define BNULL ((BPTR)0)
#define MKBADDR(p) ((BPTR)(uintptr_t)(p))
#include "constants.inc"
struct Node { UBYTE ln_Type; };
struct Task { struct Node tc_Node; };
struct MsgPort { unsigned unused; };
struct Process {
    struct Task pr_Task;
    BPTR pr_CLI,pr_CIS,pr_COS,pr_CES;
    struct MsgPort pr_MsgPort;
};
struct TagItem { Tag ti_Tag; IPTR ti_Data; };
struct M68KEmuLibBase { unsigned unused; };
struct M68KStructLayout { UWORD shadow_type; };
struct M68KEmuContext {
    UBYTE mem[16384]; ULONG bump,m68k_proc,seg_start,m68k_argptr;
    struct Task *emu_task;
    unsigned num_shadows;
    struct { ULONG m68k_addr; void *native_ptr; UWORD type; } shadow_map[16];
    BPTR *handles; ULONG num_handles;
    char program_name[108];
    void *wb_startup; BOOL wb_queued;
    ULONG entry_point,m68k_argsize,m68k_sysbase;
    LONG exit_code;
};
static struct Process parent,other;
static struct Task only_task={{1}};
static struct Task *current;
static struct M68KStructLayout task_layout={1},port_layout={2};
static struct M68KEmuLibBase base;
static int sysbase;
#define SysBase (&sysbase)
static unsigned checks,failures,creates,executes,unlocks,messages_enqueued,wb_done,restores;
static unsigned newproc_fail,execbase_fail,caller_stream_closes;
static struct TagItem captured[32];
static unsigned captured_count;
static void check(int ok,const char *name) {
    ++checks;
    if(!ok){if(failures<40)printf("FAIL: %s\n",name);++failures;}
}
static void fixture_bug(const char *format,...) {(void)format;}
#define bug(...) fixture_bug(__VA_ARGS__)
static void *AllocVec(size_t n,ULONG flags) {(void)flags;void *p=calloc(1,n);assert(p);return p;}
static void *m68k_to_host(struct M68KEmuContext *ctx,ULONG a) {return ctx->mem+a;}
static ULONG read32(struct M68KEmuContext *ctx,ULONG a) {
    assert(a<=sizeof(ctx->mem)-4);
    return ((ULONG)ctx->mem[a]<<24)|((ULONG)ctx->mem[a+1]<<16)|((ULONG)ctx->mem[a+2]<<8)|ctx->mem[a+3];
}
static void m68k_write32(struct M68KEmuContext *ctx,ULONG a,ULONG v) {
    assert(a<=sizeof(ctx->mem)-4);
    ctx->mem[a]=(UBYTE)(v>>24);ctx->mem[a+1]=(UBYTE)(v>>16);
    ctx->mem[a+2]=(UBYTE)(v>>8);ctx->mem[a+3]=(UBYTE)v;
}
static ULONG M68KEmu_HeapAlloc(struct M68KEmuContext *ctx,ULONG n,ULONG flags) {
    ULONG a=ctx->bump;(void)flags;n=(n+3)&~3u;assert(a+n<=sizeof(ctx->mem));
    ctx->bump+=n;memset(ctx->mem+a,0,n);return a;
}
static struct Task *FindTask(const void *name) {(void)name;return current;}
static BPTR Input(void) {return ((struct Process *)current)->pr_CIS;}
static BPTR Output(void) {return ((struct Process *)current)->pr_COS;}
static BPTR ErrorOutput(void) {return ((struct Process *)current)->pr_CES;}
static const struct M68KStructLayout *shadow_find_layout(const char *name) {
    return !strcmp(name,"Task")?&task_layout:&port_layout;
}
static ULONG shadow_register(struct M68KEmuContext *ctx,ULONG a,void *native,UWORD type) {
    unsigned i=ctx->num_shadows++;assert(i<16);
    ctx->shadow_map[i].m68k_addr=a;ctx->shadow_map[i].native_ptr=native;ctx->shadow_map[i].type=type;return a;
}
static void shadow_sync(struct M68KEmuContext *ctx,const struct M68KStructLayout *layout,
                        ULONG a,void *native) {
    (void)layout;(void)native;assert(a+92<=sizeof(ctx->mem));
    /* The actual Task layout ends before every Process stream. */
    memset(ctx->mem+a,0,92);
}
static void *shadow_lookup(struct M68KEmuContext *ctx,ULONG a) {
    unsigned i;for(i=0;i<ctx->num_shadows;i++)if(ctx->shadow_map[i].m68k_addr==a)return ctx->shadow_map[i].native_ptr;
    return NULL;
}
static void m68k_task_stack(struct M68KEmuContext *ctx,ULONG a,struct Task *task) {(void)ctx;(void)a;(void)task;}
#include "handles.inc"
#include "initial.inc"
#include "process.inc"
static int m68kemu_thunks_exec[1],m68kemu_thunks_exec_gen[1],m68kemu_thunks_dos[1],m68kemu_thunks_dos_gen[1];
static const ULONG m68kemu_thunks_exec_count=1,m68kemu_thunks_exec_gen_count=1,m68kemu_thunks_dos_count=1,m68kemu_thunks_dos_gen_count=1;
static ULONG M68KEmu_SetupLibBase(struct M68KEmuContext *ctx,int id,const char *name,ULONG max,
                                 const void *a,ULONG b,const void *c,ULONG d) {
    (void)ctx;(void)name;(void)max;(void)a;(void)b;(void)c;(void)d;return id?0x300:execbase_fail?0:0x80;
}
static void CopyMem(APTR from,APTR to,ULONG n) {memcpy(to,from,n);}
static void shadow_init_execbase(struct M68KEmuContext *ctx,ULONG a,void *native) {(void)native;initial_process_projection(ctx,a);}
static BPTR SetProgramDir(BPTR dir) {(void)dir;return 7;}
static void PutMsg(struct MsgPort *port,void *message) {(void)port;(void)message;++messages_enqueued;}
static LONG M68KEmu_Execute(struct M68KEmuContext *ctx) {(void)ctx;++executes;return 17;}
static void m68k_wbstartup_done(struct M68KEmuContext *ctx) {(void)ctx;++wb_done;}
static void UnLock(BPTR dir) {(void)dir;++unlocks;}
static void m68k_restore_windowptr(struct M68KEmuContext *ctx) {(void)ctx;++restores;}
static void M68KEmu_AutoOpenLibs(struct M68KEmuContext *ctx) {(void)ctx;}
static BPTR CurrentDir(BPTR dir) {(void)dir;return UINT64_C(0x120000010);}
static BPTR DupLock(BPTR dir) {return dir+4;}
static void emu_proc_entry(void) {}
static IPTR tag_value(Tag tag,IPTR absent) {
    unsigned i;for(i=0;i<captured_count;i++)if(captured[i].ti_Tag==tag)return captured[i].ti_Data;return absent;
}
static struct Process *record_create(const struct TagItem *tags) {
    struct M68KEmuContext *ctx;unsigned i;++creates;
    for(i=0;i<31&&tags[i].ti_Tag!=TAG_DONE;i++)captured[i]=tags[i];
    assert(i<31);captured_count=i;
    ctx=(struct M68KEmuContext *)(uintptr_t)tag_value(NP_UserData,0);assert(ctx);
    if(newproc_fail)return NULL;
    /* Simulate the documented close policy only, not native child execution. */
    if(tag_value(NP_CloseError,TRUE)&&tag_value(NP_Error,0)==parent.pr_CES&&parent.pr_CES)++caller_stream_closes;
    ctx->exit_code=17;return &other;
}
/* Native DOS inlines use this parameter through their library-base macro. */
#define CreateNewProc(tags) ((void)M68KEmuBase,record_create(tags))
#include "launch.inc"

static void reset(struct M68KEmuContext *ctx) {
    memset(ctx,0,sizeof(*ctx));ctx->bump=0x1000;
    memset(&parent,0,sizeof(parent));memset(&other,0,sizeof(other));
    parent.pr_Task.tc_Node.ln_Type=other.pr_Task.tc_Node.ln_Type=NT_PROCESS;
    parent.pr_CLI=1;current=&parent.pr_Task;ctx->emu_task=current;
    parent.pr_CIS=UINT64_C(0x100000010);parent.pr_COS=UINT64_C(0x200000020);parent.pr_CES=UINT64_C(0x300000030);
    other.pr_CIS=UINT64_C(0x400000040);other.pr_COS=UINT64_C(0x500000050);other.pr_CES=UINT64_C(0x600000060);
    creates=executes=unlocks=messages_enqueued=wb_done=restores=newproc_fail=execbase_fail=caller_stream_closes=captured_count=0;
}
static void release(struct M68KEmuContext *ctx) {free(ctx->handles);ctx->handles=NULL;}
static void expect_stream(struct M68KEmuContext *ctx,ULONG a,BPTR native,const char *label) {
    ULONG v=read32(ctx,a);int valid=native?(v>=M68KEMU_HANDLE_BASE&&v<M68KEMU_HANDLE_BASE+M68KEMU_MAX_HANDLES*4):v==0;
    check(valid,label);check(valid&&m68k_bptr_in(ctx,v)==native,"full-width BPTR roundtrip");
}
static void expect_projection(struct M68KEmuContext *ctx,ULONG a,struct Process *proc) {
    expect_stream(ctx,a+156,proc->pr_CIS,"Process input is a translated handle");
    expect_stream(ctx,a+160,proc->pr_COS,"Process output is a translated handle");
    expect_stream(ctx,a+224,proc->pr_CES,"Process stderr is a translated handle");
}
static void test_launches(void) {
    struct M68KEmuContext ctx;unsigned before=failures,i;
    for(i=0;i<3;i++){
        reset(&ctx);if(i==1)parent.pr_CES=0;if(i==2)parent.pr_CES=parent.pr_COS;
        BPTR in=parent.pr_CIS,out=parent.pr_COS,err=parent.pr_CES;
        LONG rc=setup_and_execute(&ctx,0x800,(CONST_STRPTR)"--help ",7,1048576,"io-fixture",0,&base);
        check(rc==17&&creates==1&&!executes,"synchronous child route and result");
        check(tag_value(NP_Synchronous,FALSE)==TRUE&&tag_value(NP_FreeSeglist,TRUE)==FALSE,"child lifecycle tags unchanged");
        check(tag_value(NP_Input,UINT64_MAX)==in,"child inherits full-width input");
        check(tag_value(NP_Output,UINT64_MAX)==out,"child inherits full-width output");
        check(tag_value(NP_Error,UINT64_MAX)==err,"child inherits stderr including null/alias");
        check(tag_value(NP_CloseInput,TRUE)==FALSE&&tag_value(NP_CloseOutput,TRUE)==FALSE,"input/output remain borrowed");
        check(tag_value(NP_CloseError,TRUE)==FALSE,"stderr remains borrowed");
        check(!caller_stream_closes,"child teardown does not close caller stderr");
        check(parent.pr_CIS==in&&parent.pr_COS==out&&parent.pr_CES==err,"caller stream identities unchanged");
        release(&ctx);
    }
    reset(&ctx);newproc_fail=1;
    check(setup_and_execute(&ctx,0x800,NULL,0,4096,"io-fixture",9,&base)==-1,"child creation failure returns failure");
    check(unlocks==2&&!caller_stream_closes,"failure releases directory locks and keeps caller streams");release(&ctx);
    reset(&ctx);ctx.wb_startup=&parent;
    check(setup_and_execute(&ctx,0x800,NULL,0,0,"io-fixture",9,&base)==17,"zero stack keeps direct route");
    check(!creates&&executes==1&&messages_enqueued==1&&wb_done==1&&restores==1&&unlocks==1,"direct route ownership unchanged");release(&ctx);
    reset(&ctx);execbase_fail=1;
    check(setup_and_execute(&ctx,0x800,NULL,0,4096,NULL,0,&base)==-1&&!creates&&!executes,"early setup failure does not launch");release(&ctx);
    printf("GROUP launch: %u failed\n",failures-before);
}
static void test_projections(void) {
    struct M68KEmuContext ctx;unsigned before=failures;ULONG a,b;
    reset(&ctx);strcpy(ctx.program_name,"io-fixture");initial_process_projection(&ctx,0x80);a=ctx.m68k_proc;
    check(a!=0&&read32(&ctx,0x80+276)==a,"real initial Process publication");expect_projection(&ctx,a,&parent);
    parent.pr_CES=UINT64_C(0x700000070);
    check(m68k_process_of(&ctx,current)==a,"owner refresh retains guest identity");expect_projection(&ctx,a,&parent);
    b=m68k_process_of(&ctx,&other.pr_Task);check(b&&b!=a,"new other Process gets its own shadow");expect_projection(&ctx,b,&other);
    other.pr_CIS=0;other.pr_CES=other.pr_COS;
    check(m68k_process_of(&ctx,&other.pr_Task)==b,"cached other Process retains identity");expect_projection(&ctx,b,&other);
    check(read32(&ctx,b+160)==read32(&ctx,b+224),"aliased stdout/stderr share handle");
    a=m68k_process_of(&ctx,&only_task);check(a!=0,"non-Process Task still supported");
    check(!read32(&ctx,a+156)&&!read32(&ctx,a+160)&&!read32(&ctx,a+224),"non-Process Task does not expose invalid Process fields");
    check(!caller_stream_closes,"shadow publication never closes caller handles");release(&ctx);
    printf("GROUP projection: %u failed\n",failures-before);
}
int main(void) {
    test_launches();test_projections();
    printf("process streams: %u checks, %u failed\n",checks,failures);
    return failures?1:0;
}
