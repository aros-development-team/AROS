/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Standalone C99 host regression for AllocCxStructure() extracted from
    workbench/libs/commodities/eventfuncs.c.
*/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t UBYTE;
typedef uint16_t UWORD;
typedef uint32_t ULONG;
typedef int32_t LONG;
typedef void *APTR;
#define VOID void

#define MEMF_PUBLIC (1UL << 0)
#define MEMF_CLEAR  (1UL << 16)

#define CBD_NAMELEN  24
#define CBD_TITLELEN 40
#define CBD_DESCRLEN 40

#define CX_INVALID    0
#define CX_FILTER     1
#define CX_TYPEFILTER 2
#define CX_SEND       3
#define CX_SIGNAL     4
#define CX_TRANSLATE  5
#define CX_BROKER     6
#define CX_DEBUG      7
#define CX_CUSTOM     8
#define CX_ZERO       9

#define CXM_IEVENT  (1U << 5)
#define CXM_COMMAND (1U << 6)

#define cxm_MaxLevel 32

enum { CX_OBJECT, CX_MESSAGE, CX_INPUTEVENT };
enum { CXM_SINGLE, CXM_DOUBLE };

struct Node {
    struct Node *ln_Succ;
    struct Node *ln_Pred;
    UBYTE ln_Type;
    signed char ln_Pri;
    char *ln_Name;
};

struct MinNode {
    struct MinNode *mln_Succ;
    struct MinNode *mln_Pred;
};

struct MinList {
    struct MinNode *mlh_Head;
    struct MinNode *mlh_Tail;
    struct MinNode *mlh_TailPred;
};

#define NEWLIST(l) \
    do { \
        struct MinList *nl_ = (l); \
        nl_->mlh_Head = (struct MinNode *)&nl_->mlh_Tail; \
        nl_->mlh_Tail = NULL; \
        nl_->mlh_TailPred = (struct MinNode *)&nl_->mlh_Head; \
    } while (0)

struct Library {
    struct Node lib_Node;
};

struct MsgPort {
    struct Node mp_Node;
};

struct Message {
    struct Node mn_Node;
    struct MsgPort *mn_ReplyPort;
    UWORD mn_Length;
};

struct Task {
    int placeholder;
};

struct InputEvent {
    struct InputEvent *ie_NextEvent;
    UBYTE ie_Class;
    UBYTE ie_SubClass;
    APTR ie_EventAddress;
};

typedef struct InputXpression {
    UBYTE ix_Version;
    UBYTE ix_Class;
    UWORD ix_Code;
    UWORD ix_CodeMask;
    UWORD ix_Qualifier;
    UWORD ix_QualMask;
    UWORD ix_QualSame;
} IX;

struct BrokerExt {
    char bext_Name[CBD_NAMELEN];
    char bext_Title[CBD_TITLELEN];
    char bext_Descr[CBD_DESCRLEN];
    struct Task *bext_Task;
    struct MsgPort *bext_MsgPort;
};

struct SendExt {
    struct MsgPort *sext_MsgPort;
    ULONG sext_ID;
};

struct SignalExt {
    struct Task *sixt_Task;
    UBYTE sixt_SigBit;
};

struct CustomExt {
    VOID (*cext_Action)(VOID);
    ULONG cext_ID;
};

typedef struct cx_Object {
    struct Node co_Node;
    UBYTE co_Flags;
    UBYTE co_Error;
    struct MinList co_ObjList;
    union {
        ULONG co_DebugID;
        ULONG co_TypeFilter;
        struct InputEvent *co_IE;
        IX *co_FilterIX;
        struct BrokerExt *co_BExt;
        struct SendExt *co_SendExt;
        struct SignalExt *co_SignalExt;
        struct CustomExt *co_CustomExt;
    } co_Ext;
} CxObj;

typedef struct cx_Message {
    struct Message cxm_Message;
    CxObj *cxm_Routing;
    LONG cxm_ID;
    UBYTE cxm_Type;
    UBYTE cxm_Level;
    CxObj *cxm_retObj[cxm_MaxLevel];
    struct InputEvent *cxm_Data;
} CxMsg;

struct GeneratedInputEvent {
    struct InputEvent ie;
    struct MinNode node;
};

struct CommoditiesBase {
    struct Library cx_Lib;
    struct MsgPort cx_MsgPort;
};

#define GPB(x) ((struct CommoditiesBase *)(x))

struct TrackedBlock {
    APTR ptr;
    ULONG size;
    int kind;
    int live;
};

static struct {
    struct CommoditiesBase base;
    struct TrackedBlock blocks[8];
    unsigned block_count;
    unsigned live_count;
    unsigned allocvec_calls;
    unsigned freevec_calls;
    unsigned allocmem_calls;
    ULONG last_vec_size;
    ULONG last_vec_flags;
    ULONG last_mem_size;
    ULONG last_mem_flags;
    int fail_allocvec;
    int fail_allocmem;
    UBYTE guard[256];
} state;

static unsigned checks;
static unsigned failures;
static const char *case_name;

static void check(int ok, const char *description)
{
    ++checks;
    if (!ok) {
        ++failures;
        printf("FAIL [%s] %s\n", case_name, description);
    }
}

static APTR record_alloc(ULONG size, ULONG flags, int kind)
{
    APTR raw;
    if (state.block_count >= sizeof(state.blocks) / sizeof(state.blocks[0])) {
        check(0, "fixture block table has room");
        return NULL;
    }
    raw = malloc(size ? (size_t)size : 1u);
    if (raw == NULL) {
        abort();
    }
    if ((flags & MEMF_CLEAR) != 0) {
        memset(raw, 0, (size_t)size);
    } else {
        memset(raw, 0xA5, (size_t)size);
    }
    state.blocks[state.block_count].ptr = raw;
    state.blocks[state.block_count].size = size;
    state.blocks[state.block_count].kind = kind;
    state.blocks[state.block_count].live = 1;
    ++state.block_count;
    ++state.live_count;
    return raw;
}

static void release_block(APTR ptr, int expected_kind)
{
    unsigned i;
    for (i = 0; i < state.block_count; ++i) {
        if (state.blocks[i].ptr == ptr && state.blocks[i].live) {
            check(state.blocks[i].kind == expected_kind, "deallocator matches allocator kind");
            state.blocks[i].live = 0;
            --state.live_count;
            free(ptr);
            return;
        }
    }
    check(0, "only live fixture allocations are freed");
}

static APTR AllocVec(ULONG size, ULONG flags)
{
    ++state.allocvec_calls;
    state.last_vec_size = size;
    state.last_vec_flags = flags;
    if (state.fail_allocvec) {
        return NULL;
    }
    return record_alloc(size, flags, 1);
}

static void FreeVec(APTR memory)
{
    ++state.freevec_calls;
    if (memory == NULL) {
        return;
    }
    release_block(memory, 1);
}

static APTR AllocMem(ULONG size, ULONG flags)
{
    ++state.allocmem_calls;
    state.last_mem_size = size;
    state.last_mem_flags = flags;
    if (state.fail_allocmem) {
        return NULL;
    }
    return record_alloc(size, flags, 2);
}

APTR AllocCxStructure(LONG type, LONG objtype, struct Library *CxBase);

#include "production_alloccxstructure.inc"

static void reset_state(const char *label)
{
    unsigned i;
    for (i = 0; i < state.block_count; ++i) {
        if (state.blocks[i].live) {
            free(state.blocks[i].ptr);
        }
    }
    memset(&state, 0, sizeof(state));
    memset(state.guard, 0x5A, sizeof(state.guard));
    case_name = label;
}

static int guard_intact(void)
{
    unsigned i;
    for (i = 0; i < sizeof(state.guard); ++i) {
        if (state.guard[i] != 0x5A) {
            return 0;
        }
    }
    return 1;
}

static void test_cx_object_failure(void)
{
    LONG objtype;
    for (objtype = CX_INVALID; objtype <= CX_ZERO; ++objtype) {
        APTR got;
        reset_state("CX_OBJECT AllocVec failure");
        state.fail_allocvec = 1;
        got = AllocCxStructure(CX_OBJECT, objtype, &state.base.cx_Lib);
        check(got == NULL, "CX_OBJECT returns NULL when AllocVec fails");
        check(state.allocvec_calls == 1 && state.allocmem_calls == 0 && state.freevec_calls == 0,
              "CX_OBJECT failure makes exactly one AllocVec attempt and no FreeVec/AllocMem call");
        check(state.last_vec_size == (ULONG)sizeof(CxObj) + Extensions[objtype] &&
              state.last_vec_flags == (MEMF_CLEAR | MEMF_PUBLIC),
              "CX_OBJECT requests sizeof(CxObj) + Extensions[objtype] with MEMF_CLEAR | MEMF_PUBLIC");
        check(state.live_count == 0 && guard_intact(),
              "CX_OBJECT failure leaves nothing allocated and guard bytes untouched");
    }
}

static void test_cx_object_success(void)
{
    static const ULONG expected_ext[] = {
        0,
        (ULONG)sizeof(struct InputXpression),
        0,
        (ULONG)sizeof(struct SendExt),
        (ULONG)sizeof(struct SignalExt),
        0,
        (ULONG)sizeof(struct BrokerExt),
        0,
        (ULONG)sizeof(struct CustomExt),
        0
    };
    LONG objtype;
    for (objtype = CX_INVALID; objtype <= CX_ZERO; ++objtype) {
        CxObj *co;
        reset_state("CX_OBJECT AllocVec success");
        co = (CxObj *)AllocCxStructure(CX_OBJECT, objtype, &state.base.cx_Lib);
        check(co != NULL && state.live_count == 1,
              "CX_OBJECT returns one live allocation on success");
        check(state.last_vec_size == (ULONG)sizeof(CxObj) + expected_ext[objtype] &&
              state.last_vec_flags == (MEMF_CLEAR | MEMF_PUBLIC),
              "CX_OBJECT allocates exact object plus extension size");
        if (co != NULL) {
            check((APTR)co->co_Ext.co_FilterIX == (APTR)(co + 1),
                  "co_Ext.co_FilterIX points immediately past CxObj");
            check(co->co_ObjList.mlh_Head == (struct MinNode *)&co->co_ObjList.mlh_Tail &&
                  co->co_ObjList.mlh_Tail == NULL &&
                  co->co_ObjList.mlh_TailPred == (struct MinNode *)&co->co_ObjList.mlh_Head,
                  "co_ObjList is initialized as an empty MinList");
            if (objtype == CX_BROKER) {
                check(co->co_Node.ln_Name == &co->co_Ext.co_BExt->bext_Name[0],
                      "CX_BROKER points co_Node.ln_Name at bext_Name");
            } else {
                check(co->co_Node.ln_Name == NULL,
                      "non-broker CX_OBJECT leaves co_Node.ln_Name NULL");
            }
            FreeVec(co);
            check(state.live_count == 0, "FreeVec releases CX_OBJECT cleanly");
        }
    }
}

static void test_cx_message_and_inputevent(void)
{
    CxMsg *msg;
    APTR ie;

    reset_state("CXM_SINGLE failure");
    state.fail_allocvec = 1;
    msg = (CxMsg *)AllocCxStructure(CX_MESSAGE, CXM_SINGLE, &state.base.cx_Lib);
    check(msg == NULL && state.live_count == 0 && guard_intact(),
          "CXM_SINGLE returns NULL and leaks nothing when AllocVec fails");

    reset_state("CXM_SINGLE success");
    msg = (CxMsg *)AllocCxStructure(CX_MESSAGE, CXM_SINGLE, &state.base.cx_Lib);
    check(msg != NULL && state.live_count == 1, "CXM_SINGLE allocates one CxMsg");
    if (msg != NULL) {
        check(msg->cxm_Data == NULL && msg->cxm_Type == CXM_COMMAND &&
              msg->cxm_Message.mn_ReplyPort == &state.base.cx_MsgPort &&
              msg->cxm_Message.mn_Length == (UWORD)sizeof(CxMsg),
              "CXM_SINGLE initializes command message fields");
        FreeVec(msg);
    }

    reset_state("CXM_DOUBLE outer AllocVec failure");
    state.fail_allocvec = 1;
    msg = (CxMsg *)AllocCxStructure(CX_MESSAGE, CXM_DOUBLE, &state.base.cx_Lib);
    check(msg == NULL && state.allocmem_calls == 0 && state.live_count == 0,
          "CXM_DOUBLE skips inner AllocMem and returns NULL when outer AllocVec fails");

    reset_state("CXM_DOUBLE inner AllocMem failure");
    state.fail_allocmem = 1;
    msg = (CxMsg *)AllocCxStructure(CX_MESSAGE, CXM_DOUBLE, &state.base.cx_Lib);
    check(msg == NULL && state.allocvec_calls == 1 && state.allocmem_calls == 1 &&
          state.freevec_calls == 1 && state.live_count == 0,
          "CXM_DOUBLE frees outer CxMsg and returns NULL when inner CX_INPUTEVENT fails");

    reset_state("CXM_DOUBLE success");
    msg = (CxMsg *)AllocCxStructure(CX_MESSAGE, CXM_DOUBLE, &state.base.cx_Lib);
    check(msg != NULL && state.live_count == 2,
          "CXM_DOUBLE allocates CxMsg and GeneratedInputEvent on success");
    if (msg != NULL) {
        check(msg->cxm_Type == CXM_IEVENT && msg->cxm_Data != NULL &&
              msg->cxm_Message.mn_ReplyPort == &state.base.cx_MsgPort &&
              msg->cxm_Message.mn_Length == (UWORD)sizeof(CxMsg),
              "CXM_DOUBLE initializes input-event message fields");
        release_block(msg->cxm_Data, 2);
        FreeVec(msg);
        check(state.live_count == 0, "CXM_DOUBLE blocks release cleanly");
    }

    reset_state("CX_INPUTEVENT failure and success");
    state.fail_allocmem = 1;
    ie = AllocCxStructure(CX_INPUTEVENT, 0, &state.base.cx_Lib);
    check(ie == NULL && state.live_count == 0, "CX_INPUTEVENT returns NULL when AllocMem fails");
    state.fail_allocmem = 0;
    ie = AllocCxStructure(CX_INPUTEVENT, 0, &state.base.cx_Lib);
    check(ie != NULL && state.last_mem_size == (ULONG)sizeof(struct GeneratedInputEvent) &&
          state.last_mem_flags == (MEMF_CLEAR | MEMF_PUBLIC),
          "CX_INPUTEVENT allocates GeneratedInputEvent with MEMF_CLEAR | MEMF_PUBLIC");
    if (ie != NULL) {
        release_block(ie, 2);
    }
}

int main(void)
{
    test_cx_object_failure();
    test_cx_object_success();
    test_cx_message_and_inputevent();
    printf("Commodities AllocCxStructure: %u checks, %u failed\n", checks, failures);
    return failures ? 1 : 0;
}
