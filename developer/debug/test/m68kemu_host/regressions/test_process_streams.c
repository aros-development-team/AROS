/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Process streams: the actual launch, BPTR handle, shadow registry, Process
    projection and stream selector code of rom/m68kemu, extracted by
    run_process_streams.py, with recording collaborators. This fixture does
    not model a native SDK layout, the CPU or child execution.
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
    unsigned num_shadows; BOOL shadow_full_logged;
    struct { ULONG m68k_addr; void *native_ptr; UWORD type; UBYTE streams_valid; BPTR stream_native[3]; ULONG stream_handle[3]; } shadow_map[M68KEMU_MAX_SHADOWS];
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
static struct Process third;
static int dummy[2];
static unsigned logs,heap_frees,select_in_calls,select_out_calls;
static BOOL heap_fail,missing_task_layout;
static ULONG heap_freed_addr,heap_freed_size,cpu_d[8];
static BPTR last_selected;
#define THUNK_D(n) (((ULONG *)cpu)[n])
/* Every failed check is printed: the limit is above the number of checks. */
static void check(int ok,const char *name) {
    ++checks;
    if(!ok){if(failures<200)printf("FAIL: %s\n",name);++failures;}
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
    ULONG a=ctx->bump;(void)flags;if(heap_fail)return 0;n=(n+3)&~3u;assert(a+n<=sizeof(ctx->mem));
    ctx->bump+=n;memset(ctx->mem+a,0,n);return a;
}
static void M68KEmu_HeapFree(struct M68KEmuContext *ctx,ULONG a,ULONG n) {(void)ctx;++heap_frees;heap_freed_addr=a;heap_freed_size=n;}
static struct Task *FindTask(const void *name) {(void)name;return current;}
static BPTR Input(void) {return ((struct Process *)current)->pr_CIS;}
static BPTR Output(void) {return ((struct Process *)current)->pr_COS;}
static BPTR ErrorOutput(void) {return ((struct Process *)current)->pr_CES;}
static const struct M68KStructLayout *shadow_find_layout(const char *name) {
    if(!strcmp(name,"Task"))return missing_task_layout?NULL:&task_layout;
    return &port_layout;
}
static void m68k_log(struct M68KEmuContext *ctx,const char *format,...) {(void)ctx;(void)format;++logs;}
/* The registry is the actual one: its entries carry the stream state. */
#include "registry.inc"
static void shadow_sync(struct M68KEmuContext *ctx,const struct M68KStructLayout *layout,
                        ULONG a,void *native) {
    (void)layout;(void)native;assert(a+92<=sizeof(ctx->mem));
    /* The actual Task layout ends before every Process stream. */
    memset(ctx->mem+a,0,92);
}
static void m68k_task_stack(struct M68KEmuContext *ctx,ULONG a,struct Task *task) {(void)ctx;(void)a;(void)task;}
#include "handles.inc"
#include "initial.inc"
#include "process.inc"
#include "lifetime.inc"
static BPTR SelectInput(BPTR fh) {struct Process *me=(struct Process *)current;BPTR old=me->pr_CIS;++select_in_calls;last_selected=fh;me->pr_CIS=fh;return old;}
static BPTR SelectOutput(BPTR fh) {struct Process *me=(struct Process *)current;BPTR old=me->pr_COS;++select_out_calls;last_selected=fh;me->pr_COS=fh;return old;}
#include "selectors.inc"
/* The manual Input() and Output() thunks take the CPU and do not use it.
   The warning about that is off for these two unchanged production
   functions only; every other line keeps the flags of the runner. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "native-consumers.inc"
#pragma GCC diagnostic pop
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
    memset(&third,0,sizeof(third));third.pr_Task.tc_Node.ln_Type=NT_PROCESS;
    third.pr_CIS=UINT64_C(0x1100000110);third.pr_COS=UINT64_C(0x1200000120);third.pr_CES=UINT64_C(0x1300000130);
    logs=heap_frees=select_in_calls=select_out_calls=0;heap_fail=missing_task_layout=FALSE;
    heap_freed_addr=heap_freed_size=0;last_selected=0;memset(cpu_d,0,sizeof(cpu_d));
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
/* A field holds the handle of this native stream; a NULL stream is zero. */
static int holds(struct M68KEmuContext *ctx,ULONG a,BPTR native) {
    ULONG v=read32(ctx,a);
    return native?(v>=M68KEMU_HANDLE_BASE&&v<M68KEMU_HANDLE_BASE+M68KEMU_MAX_HANDLES*4&&m68k_bptr_in(ctx,v)==native):v==0;
}
static void refresh(struct M68KEmuContext *ctx,struct Task *task,unsigned n) {while(n--)(void)m68k_process_of(ctx,task);}
/* A process made as create_proc() makes one: by its parent, inside the
   parent's call, before the new process runs. */
static ULONG create_process(struct M68KEmuContext *ctx,struct Process *p) {
#if HAVE_PROCESS_FORGET
    m68k_process_forget(ctx,&p->pr_Task);
#endif
    return m68k_process_of(ctx,&p->pr_Task);
}
/* The launched root Process, as the projection group makes it. */
static ULONG start(struct M68KEmuContext *ctx) {
    reset(ctx);strcpy(ctx->program_name,"io-fixture");initial_process_projection(ctx,0x80);return ctx->m68k_proc;
}
/* No check stands inside a loop here. */
static void test_ownership(void) {
    const BPTR X1=UINT64_C(0x2100000210),X2=UINT64_C(0x2200000220),X3=UINT64_C(0x2300000230),X4=UINT64_C(0x2400000240);
    struct M68KEmuContext ctx;unsigned before=failures,i;ULONG a,b,c,g,h,v;IPTR r;BPTR was;
    /* a value the program stores in a field of its own Process stays */
    a=start(&ctx);g=m68k_bptr_out(&ctx,X1);
    m68k_write32(&ctx,a+M68K_PR_COS,g);refresh(&ctx,current,3);
    check(read32(&ctx,a+M68K_PR_COS)==g,"a handle stored in the output field stays through the owner's refreshes");
    b=m68k_process_of(&ctx,&other.pr_Task);current=&other.pr_Task;refresh(&ctx,&parent.pr_Task,2);current=&parent.pr_Task;refresh(&ctx,current,1);
    check(b&&read32(&ctx,a+M68K_PR_COS)==g,"it stays through another Process's lookup and a refresh made by another task");
    check(holds(&ctx,a+M68K_PR_CIS,parent.pr_CIS)&&holds(&ctx,a+M68K_PR_CES,parent.pr_CES),"the other two fields still show their native streams");
    m68k_write32(&ctx,a+M68K_PR_COS,0);refresh(&ctx,current,3);
    check(read32(&ctx,a+M68K_PR_COS)==0&&parent.pr_COS!=0,"a stored NULL stays");
    v=read32(&ctx,a+M68K_PR_CIS);m68k_write32(&ctx,a+M68K_PR_CES,v);refresh(&ctx,current,3);
    check(v&&read32(&ctx,a+M68K_PR_CES)==v&&parent.pr_CES!=parent.pr_CIS,"a stored copy of another field's handle stays");
    m68k_write32(&ctx,a+M68K_PR_COS,0x00123454);refresh(&ctx,current,3);
    check(read32(&ctx,a+M68K_PR_COS)==0x00123454,"a stored number that is no handle stays");
    /* a native change is shown, and wins over a stored value */
    parent.pr_CIS=X2;refresh(&ctx,current,1);
    check(holds(&ctx,a+M68K_PR_CIS,X2)&&read32(&ctx,a+M68K_PR_COS)==0x00123454&&read32(&ctx,a+M68K_PR_CES)==v,"a native change of one stream is shown and leaves the stored values of the others");
    parent.pr_COS=X3;refresh(&ctx,current,1);
    check(holds(&ctx,a+M68K_PR_COS,X3),"a native change of a stream whose field holds a stored value wins at the next refresh");
    parent.pr_COS=X4;refresh(&ctx,current,1);
    check(holds(&ctx,a+M68K_PR_COS,X4),"and the field follows the native stream from then on");
    release(&ctx);
    /* the selectors show their choice at once; their order against stored values is exact */
    a=start(&ctx);was=parent.pr_COS;cpu_d[1]=m68k_bptr_out(&ctx,X1);r=API_SELECT_OUT(&ctx,cpu_d);
    check(holds(&ctx,a+M68K_PR_COS,X1),"SelectOutput shows the chosen stream in the field at once");
    check(m68k_bptr_in(&ctx,(ULONG)r)==was&&select_out_calls==1&&last_selected==X1&&parent.pr_COS==X1,"SelectOutput returns the stream before and selects the full-width native stream once");
    was=parent.pr_CIS;cpu_d[1]=m68k_bptr_out(&ctx,X2);r=API_SELECT_IN(&ctx,cpu_d);
    check(holds(&ctx,a+M68K_PR_CIS,X2),"SelectInput shows the chosen stream in the field at once");
    check(m68k_bptr_in(&ctx,(ULONG)r)==was&&select_in_calls==1&&last_selected==X2&&parent.pr_CIS==X2,"SelectInput returns the stream before and selects the full-width native stream once");
    g=m68k_bptr_out(&ctx,X3);m68k_write32(&ctx,a+M68K_PR_COS,g);refresh(&ctx,current,3);
    check(read32(&ctx,a+M68K_PR_COS)==g,"a value stored after a selector call stays through refreshes");
    cpu_d[1]=m68k_bptr_out(&ctx,X4);(void)API_SELECT_OUT(&ctx,cpu_d);
    check(holds(&ctx,a+M68K_PR_COS,X4),"a selector call after a stored value wins at once");
    m68k_write32(&ctx,a+M68K_PR_COS,g);(void)API_SELECT_OUT(&ctx,cpu_d);
    check(holds(&ctx,a+M68K_PR_COS,X4),"a selector call wins also when it chooses the stream that is already the native one");
    m68k_write32(&ctx,a+M68K_PR_COS,g);refresh(&ctx,current,3);
    check(read32(&ctx,a+M68K_PR_COS)==g,"and a value stored after that stays again");
    release(&ctx);
    /* a parent and its children do not see each other's stored values */
    a=start(&ctx);b=create_process(&ctx,&other);g=m68k_bptr_out(&ctx,X1);h=m68k_bptr_out(&ctx,X2);
    m68k_write32(&ctx,a+M68K_PR_COS,g);if(b)m68k_write32(&ctx,b+M68K_PR_COS,h);
    refresh(&ctx,&parent.pr_Task,2);refresh(&ctx,&other.pr_Task,2);current=&other.pr_Task;refresh(&ctx,&parent.pr_Task,2);refresh(&ctx,&other.pr_Task,2);
    check(b&&b!=a&&read32(&ctx,a+M68K_PR_COS)==g&&read32(&ctx,b+M68K_PR_COS)==h,"parent and child each keep the value they stored, whichever task refreshes");
    c=create_process(&ctx,&third);
    check(c&&holds(&ctx,c+M68K_PR_CIS,third.pr_CIS)&&holds(&ctx,c+M68K_PR_COS,third.pr_COS)&&holds(&ctx,c+M68K_PR_CES,third.pr_CES),"a new child starts with its own native streams");
    check(b&&read32(&ctx,a+M68K_PR_COS)==g&&read32(&ctx,b+M68K_PR_COS)==h,"creating a child changes no field of the others");
    was=parent.pr_COS;cpu_d[1]=m68k_bptr_out(&ctx,X3);(void)API_SELECT_OUT(&ctx,cpu_d);
    check(b&&holds(&ctx,b+M68K_PR_COS,X3)&&read32(&ctx,a+M68K_PR_COS)==g&&parent.pr_COS==was,"a child's selector changes its own field and nothing of its parent");
    current=&parent.pr_Task;release(&ctx);
    /* a plain Task has no streams; Input() and Output() answer from the native Process */
    a=start(&ctx);c=m68k_process_of(&ctx,&only_task);refresh(&ctx,&only_task,3);
    check(c&&!read32(&ctx,c+M68K_PR_CIS)&&!read32(&ctx,c+M68K_PR_COS)&&!read32(&ctx,c+M68K_PR_CES),"a plain Task shows no stream field through refreshes");
    r=thunk_dos_Input(&ctx,cpu_d);v=read32(&ctx,a+M68K_PR_CIS);
    check(v&&(ULONG)r==v&&m68k_bptr_in(&ctx,v)==parent.pr_CIS,"Input() answers the native stream, which the field shows at the start");
    r=thunk_dos_Output(&ctx,cpu_d);v=read32(&ctx,a+M68K_PR_COS);
    check(v&&(ULONG)r==v&&m68k_bptr_in(&ctx,v)==parent.pr_COS,"Output() answers the native stream, which the field shows at the start");
    cpu_d[1]=m68k_bptr_out(&ctx,X1);(void)API_SELECT_OUT(&ctx,cpu_d);r=thunk_dos_Output(&ctx,cpu_d);
    check((ULONG)r==read32(&ctx,a+M68K_PR_COS)&&m68k_bptr_in(&ctx,(ULONG)r)==X1,"after a selector call Output() and the field agree");
    g=m68k_bptr_out(&ctx,X2);m68k_write32(&ctx,a+M68K_PR_COS,g);r=thunk_dos_Output(&ctx,cpu_d);
    check(m68k_bptr_in(&ctx,(ULONG)r)==X1&&(ULONG)r!=g&&read32(&ctx,a+M68K_PR_COS)==g,"a value stored in the field is kept and read back; it does not redirect Output()");
    release(&ctx);
    /* a new native process at an ended one's address, with the same streams */
    (void)start(&ctx);b=create_process(&ctx,&other);g=m68k_bptr_out(&ctx,X1);if(b)m68k_write32(&ctx,b+M68K_PR_COS,g);
    c=create_process(&ctx,&other);
    check(c&&holds(&ctx,c+M68K_PR_CIS,other.pr_CIS)&&holds(&ctx,c+M68K_PR_COS,other.pr_COS)&&holds(&ctx,c+M68K_PR_CES,other.pr_CES),"a process at an ended one's native address starts with its native streams");
    check(b&&c!=b&&read32(&ctx,b+M68K_PR_COS)==g,"it is not given the ended one's guest Process, whose stored value stays where it was");
    release(&ctx);
    /* a dropped handle is given out again: equal handles do not prove the same stream */
    a=start(&ctx);h=read32(&ctx,a+M68K_PR_COS);g=m68k_bptr_out(&ctx,X1);m68k_write32(&ctx,a+M68K_PR_COS,g);
    parent.pr_COS=X2;m68k_bptr_drop(&ctx,h);v=m68k_bptr_out(&ctx,X2);refresh(&ctx,current,1);
    check(h&&v==h&&g!=h,"fixture: the changed native stream takes over the dropped handle");
    check(read32(&ctx,a+M68K_PR_COS)==h&&m68k_bptr_in(&ctx,h)==X2,"a native change is shown also when the new stream has the old handle value");
    release(&ctx);
    /* a projection that got no handle is made when one is free again */
    (void)start(&ctx);
    for(i=0;m68k_bptr_out(&ctx,UINT64_C(0x4000000000)+16u*i);i++)assert(i<M68KEMU_MAX_HANDLES);
    b=create_process(&ctx,&other);
    check(b&&!read32(&ctx,b+M68K_PR_CIS)&&!read32(&ctx,b+M68K_PR_COS)&&!read32(&ctx,b+M68K_PR_CES),"fixture: with the handle table full a new Process gets no stream handle");
    m68k_bptr_drop(&ctx,(ULONG)(M68KEMU_HANDLE_BASE+4*(M68KEMU_MAX_HANDLES-1)));refresh(&ctx,&other.pr_Task,1);
    check(b&&read32(&ctx,b+M68K_PR_CIS)!=0&&holds(&ctx,b+M68K_PR_CIS,other.pr_CIS),"a projection that got no handle is made when a handle is free again");
    release(&ctx);
    /* a registry slot that a removal vacated carries nothing over */
    (void)start(&ctx);b=m68k_process_of(&ctx,&other.pr_Task);refresh(&ctx,&other.pr_Task,1);
    if(b){shadow_remove(&ctx,b+M68K_PR_MSGPORT);shadow_remove(&ctx,b);}
    c=m68k_process_of(&ctx,&other.pr_Task);
    check(b&&c&&c!=b&&holds(&ctx,c+M68K_PR_CIS,other.pr_CIS)&&holds(&ctx,c+M68K_PR_COS,other.pr_COS)&&holds(&ctx,c+M68K_PR_CES,other.pr_CES),"an entry registered into a vacated slot starts with no stream state: all three fields are written");
    release(&ctx);
    /* a removal moves whole entries: a Process's entry keeps its stream state */
    (void)start(&ctx);
    if(!shadow_register(&ctx,0x3000,&dummy[0],7)||!shadow_register(&ctx,0x3010,&dummy[1],7))abort();
    c=m68k_process_of(&ctx,&third.pr_Task);g=m68k_bptr_out(&ctx,X1);if(c)m68k_write32(&ctx,c+M68K_PR_COS,g);
    shadow_remove(&ctx,0x3000);shadow_remove(&ctx,0x3010);refresh(&ctx,&third.pr_Task,2);
    check(c&&shadow_lookup(&ctx,c)==(void *)&third.pr_Task&&shadow_lookup(&ctx,c+M68K_PR_MSGPORT)==(void *)&third.pr_MsgPort,"a removal that moves a Process's entries keeps them found");
    check(c&&read32(&ctx,c+M68K_PR_COS)==g&&holds(&ctx,c+M68K_PR_CIS,third.pr_CIS),"the moved entry keeps its stream state: a stored value stays");
    third.pr_COS=X2;refresh(&ctx,&third.pr_Task,1);
    check(c&&holds(&ctx,c+M68K_PR_COS,X2),"and a native change is still shown after the move");
    release(&ctx);
    /* no root Process is published that is not registered */
    reset(&ctx);heap_fail=TRUE;initial_process_projection(&ctx,0x80);
    check(!ctx.m68k_proc&&!ctx.num_shadows&&!read32(&ctx,0x80+M68K_EXECBASE_THISTASK),"no guest memory: no root Process, nothing registered, ThisTask left as it was");
    release(&ctx);reset(&ctx);ctx.num_shadows=M68KEMU_MAX_SHADOWS;initial_process_projection(&ctx,0x80);
    check(!ctx.m68k_proc&&!read32(&ctx,0x80+M68K_EXECBASE_THISTASK),"a refused registration publishes no root Process, as when there is no memory");
    check(heap_frees==1&&heap_freed_addr==0x1000&&heap_freed_size==M68K_SIZEOF_PROCESS&&ctx.num_shadows==M68KEMU_MAX_SHADOWS&&logs==1,"its guest block is rolled back and the refusal is logged once");
    release(&ctx);reset(&ctx);missing_task_layout=TRUE;initial_process_projection(&ctx,0x80);
    check(!ctx.m68k_proc&&!ctx.num_shadows&&!read32(&ctx,0x80+M68K_EXECBASE_THISTASK),"without a Task layout no root Process is published either");
    check(heap_frees==1&&heap_freed_addr==0x1000&&heap_freed_size==M68K_SIZEOF_PROCESS,"and its guest block is rolled back");
    release(&ctx);
    printf("GROUP ownership: %u failed\n",failures-before);
}
int main(void) {
    test_launches();test_projections();test_ownership();
    printf("process streams: %u checks, %u failed\n",checks,failures);
    return failures?1:0;
}
