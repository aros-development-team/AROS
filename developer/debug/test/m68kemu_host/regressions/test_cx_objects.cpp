/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Host fixture for the production commodities CreateCxObj thunk: each object
    type reaches the native CreateCxObj with native arguments, a custom object
    is made without its 68k function, and the types that need a 68k callback
    are refused.
*/
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using ULONG = uint32_t;
using LONG = int32_t;
using IPTR = uintptr_t;
using APTR = void *;
using BOOL = int;
struct CxObj;
struct MsgPort;
struct Task;

#include "constants.inc"

struct TestCPU
{
    ULONG d[8] = {}, a[8] = {};
};

struct Call
{
    ULONG type;
    IPTR a1, a2;
};

struct M68KEmuContext
{
    char program_name[64] = "RAM:Demo/Program";
    bool fail_create = false;
    std::vector<Call> creates;
    std::vector<std::string> logs;
    char text[16] = "rawkey f2";
};

static Task *const THIS_TASK = reinterpret_cast<Task *>(0x7a5c);
static MsgPort *const PORT_TWIN = reinterpret_cast<MsgPort *>(0x9047);
static M68KEmuContext *current;

static ULONG M68KEmu_GetD(void *cpu, int n) { return static_cast<TestCPU *>(cpu)->d[n]; }
static ULONG M68KEmu_GetA(void *cpu, int n) { return static_cast<TestCPU *>(cpu)->a[n]; }
static void *m68k_to_host(M68KEmuContext *ctx, ULONG a) { return a == 0x200 ? ctx->text : nullptr; }
static MsgPort *m68k_port_twin(M68KEmuContext *, ULONG a, BOOL *own) { *own = 0; return a == 0x300 ? PORT_TWIN : nullptr; }
static Task *FindTask(const void *) { return THIS_TASK; }

static CxObj *CreateCxObj(ULONG type, IPTR a1, IPTR a2)
{
    current->creates.push_back({type, a1, a2});
    return current->fail_create ? nullptr : reinterpret_cast<CxObj *>(0x5000 + type);
}

static IPTR m68k_native_out(M68KEmuContext *, CxObj *co)
{
    return co ? 0x00800000u + (reinterpret_cast<IPTR>(co) & 0xff) : 0;
}

static void m68k_log(M68KEmuContext *ctx, const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ctx->logs.push_back(buf);
}

#include "functions.inc"

static unsigned checks;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
    std::exit(1); } } while (0)

static IPTR create(M68KEmuContext &ctx, ULONG type, ULONG a0, ULONG a1)
{
    TestCPU cpu;
    cpu.d[0] = type;
    cpu.a[0] = a0;
    cpu.a[1] = a1;
    current = &ctx;
    return cx_Create(&ctx, &cpu);
}

static void custom_without_its_function()
{
    M68KEmuContext ctx;
    IPTR r = create(ctx, CX_CUSTOM, 0x00acc054, 0x1234);
    CHECK(r == 0x00800000u + 0x5000u % 0x100u + CX_CUSTOM);
    CHECK(ctx.creates.size() == 1);
    CHECK(ctx.creates[0].type == CX_CUSTOM && ctx.creates[0].a1 == 0 && ctx.creates[0].a2 == 0x1234);
    CHECK(ctx.logs.size() == 1 && ctx.logs[0] == "RAM:Demo/Program: a 68k commodities custom function is not run");

    M68KEmuContext unnamed;
    unnamed.program_name[0] = 0;
    CHECK(create(unnamed, CX_CUSTOM, 0x00acc054, 0) != 0);
    CHECK(unnamed.logs.size() == 1 && unnamed.logs[0] == "68k program: a 68k commodities custom function is not run");

    M68KEmuContext failing;
    failing.fail_create = true;
    CHECK(create(failing, CX_CUSTOM, 0x00acc054, 7) == 0);
    CHECK(failing.creates.size() == 1 && failing.creates[0].a1 == 0 && failing.creates[0].a2 == 7);
}

static void other_types_unchanged()
{
    M68KEmuContext ctx;
    CHECK(create(ctx, CX_FILTER, 0x200, 0x22) != 0);
    CHECK(ctx.creates.back().type == CX_FILTER && ctx.creates.back().a1 == reinterpret_cast<IPTR>(ctx.text) && ctx.creates.back().a2 == 0x22);
    CHECK(create(ctx, CX_FILTER, 0, 0x22) != 0 && ctx.creates.back().a1 == 0);
    CHECK(create(ctx, CX_TYPEFILTER, 0x44, 0x55) != 0 && ctx.creates.back().a1 == 0x44 && ctx.creates.back().a2 == 0x55);
    CHECK(create(ctx, CX_SEND, 0x300, 0x66) != 0 && ctx.creates.back().a1 == reinterpret_cast<IPTR>(PORT_TWIN) && ctx.creates.back().a2 == 0x66);
    CHECK(create(ctx, CX_SIGNAL, 0x77, 0x88) != 0 && ctx.creates.back().a1 == reinterpret_cast<IPTR>(THIS_TASK) && ctx.creates.back().a2 == 0x88);
    CHECK(create(ctx, CX_TRANSLATE, 0x99, 0xaa) != 0 && ctx.creates.back().a1 == 0 && ctx.creates.back().a2 == 0);
    CHECK(create(ctx, CX_DEBUG, 0xbb, 0xcc) != 0 && ctx.creates.back().a1 == 0xbb && ctx.creates.back().a2 == 0);
    CHECK(ctx.creates.size() == 7 && ctx.logs.empty());
}

static void callback_types_refused()
{
    for (ULONG type : {ULONG(CX_INVALID), ULONG(CX_BROKER), ULONG(CX_ZERO), ULONG(42)})
    {
        M68KEmuContext ctx;
        CHECK(create(ctx, type, 0x1000, 0x2000) == 0);
        CHECK(ctx.creates.empty());
        char want[160];
        std::snprintf(want, sizeof(want), "RAM:Demo/Program: commodities object type %lu needs a 68k callback, not supported yet", (unsigned long)type);
        CHECK(ctx.logs.size() == 1 && ctx.logs[0] == want);
    }
}

int main()
{
    custom_without_its_function();
    other_types_unchanged();
    callback_types_refused();
    std::printf("m68kemu commodities objects: %u checks, 0 failed\n", checks);
}
