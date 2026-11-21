/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Compile the complete, unmodified DosEntry and streamSane bodies from
    rom/dos/createnewproc.c. The fixture substitutes native dependencies, not
    their dispatch or cleanup logic. This verifies control flow and ownership
    calls, not native ABI or scheduling.
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "production_constants.h"

typedef uint32_t ULONG;
typedef int32_t LONG;
typedef intptr_t SIPTR;
typedef uintptr_t IPTR, BPTR;
typedef void *APTR;
typedef char *STRPTR;
typedef const char *CONST_STRPTR;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define BNULL ((BPTR)0)
#define BADDR(value) ((void *)(uintptr_t)(value))
#define D(value) do { } while (0)
#define TAG_DONE 0
#define TAG_USER (1UL << 31)
#define TAGGEDOPEN_DOS 12
#define AT_DeadEnd 1
#define AG_OpenLib 2
#define AO_DOSLib 4
#define AN_FreeVec 8
#define SIGF_CHILD (1U << 20)

static const char k_arguments[] = "original\n";
#define ARGUMENTS_LEN ((ULONG)(sizeof(k_arguments) - 1u))

struct Library { int identity; };
struct DosLibrary { struct Library library; APTR dl_GV; };
struct Node { CONST_STRPTR ln_Name; };
struct Task { struct Node tc_Node; };
struct ETask { struct Task *et_Parent; };
struct Process {
    struct Task pr_Task;
    STRPTR pr_Arguments;
    BPTR pr_CIS, pr_COS, pr_CES, pr_SegList;
    SIPTR pr_Result2;
    ULONG pr_StackSize;
    APTR pr_ExitCode;
    IPTR pr_ExitData;
    ULONG pr_Flags;
    BPTR pr_CurrentDir, pr_HomeDir, pr_CLI;
    APTR pr_GlobVec;
};
struct FileHandle { APTR fh_Type; SIPTR fh_Func3; };
struct CommandLineInterface { BPTR cli_StandardError; };
struct TagItem { IPTR ti_Tag, ti_Data; };

static unsigned checks, failures;
static const char *case_name;
static void check(BOOL ok, const char *description)
{
    ++checks;
    if (!ok) { ++failures; printf("FAIL [%s] %s\n", case_name, description); }
}
static struct {
    struct Process process;
    struct Task parent;
    struct ETask etask;
    struct DosLibrary dos;
    struct Library engine;
    BPTR *segments;
    APTR module;
    struct FileHandle *streams[4];
    struct FileHandle *replacement;
    struct CommandLineInterface *cli;
    STRPTR arguments;
    IPTR initial_entry;
    ULONG query_result;
    BOOL marker, engine_available, has_module, clobber_cleanup, mutate_stream;
    LONG run_result, native_result;
    SIPTR run_error, native_error, callback_error;
    LONG callback_result;
    unsigned opens_dos, opens_engine, closes_dos, closes_engine, queries;
    unsigned native_calls, native_entries, wrong_entry_calls, hunk_calls, callbacks;
    unsigned frees_args, frees_segments, frees_cli, unloads, free_vars, root_removals;
    unsigned closes[4], unlocks_current, unlocks_home, completions, notifications;
    char events[64]; unsigned event_count;
    BOOL args_live, segments_live, cli_live, module_live, stream_live[4];
} f;

static void event(char kind) { if (f.event_count < sizeof(f.events)) f.events[f.event_count++] = kind; }
static SIPTR IoErr(void) { return f.process.pr_Result2; }
static SIPTR SetIoErr(SIPTR error) { SIPTR old = IoErr(); f.process.pr_Result2 = error; return old; }
static void cleanup_error(void) { if (f.clobber_cleanup) SetIoErr(-999); }
static struct Task *FindTask(CONST_STRPTR name)
{ check(name == NULL, "entry uses its current native Process"); return &f.process.pr_Task; }
static struct ETask *GetETask(struct Process *process)
{ check(process == &f.process, "notification uses the same Process"); return &f.etask; }
static void bug(const char *format, ...) { (void)format; }
static void Alert(ULONG flags) { (void)flags; check(FALSE, "no unexpected dead-end alert"); }
static APTR TaggedOpenLibrary(ULONG tag)
{ check(tag == TAGGEDOPEN_DOS, "opens own DOSBase"); ++f.opens_dos; event('D'); return &f.dos; }
static struct Library *OpenLibrary(CONST_STRPTR name, ULONG version)
{
    check(!strcmp(name, "m68kemu.library") && version == 0, "opens only m68kemu.library");
    ++f.opens_engine; event('O');
    if (!f.engine_available) { SetIoErr(4567); return NULL; }
    return &f.engine;
}
static void CloseLibrary(APTR library)
{
    if (library == &f.engine) { ++f.closes_engine; event('E'); }
    else if (library == &f.dos) { ++f.closes_dos; event('Z'); cleanup_error(); }
    else check(FALSE, "never closes NULL or an unowned library");
}
static ULONG GetSegListInfo(BPTR module, const struct TagItem *tags)
{
    ++f.queries; event('Q');
    check(module == (BPTR)(uintptr_t)f.module && f.module_live, "queries a live native module");
    check(tags[0].ti_Tag == GSLI_68KHUNK && tags[1].ti_Tag == TAG_DONE,
          "production lookup requests the 68k hunk marker");
    *(IPTR *)tags[0].ti_Data = f.marker ? (IPTR)f.module : 0;
    return f.query_result;
}
static LONG native_entry(STRPTR args, ULONG length, struct Process *process)
{
    ++f.native_entries;
    check(args == f.arguments && length == ARGUMENTS_LEN && process == &f.process,
          "native entry receives original arguments and Process");
    SetIoErr(f.native_error);
    return f.native_result;
}
static LONG CallEntry(STRPTR args, ULONG length, APTR entry, struct Process *process)
{
    ++f.native_calls; event('N');
    /* Contain the baseline failure: record the wrong call without jumping
       into fake 68k bytes, so the regression fails without a host crash. */
    if ((IPTR)entry != (IPTR)native_entry) {
        ++f.wrong_entry_calls;
        SetIoErr(3434);
        return 0x777;
    }
    if (f.mutate_stream) process->pr_CIS = (BPTR)(uintptr_t)f.replacement;
    return native_entry(args, length, process);
}
static LONG RunHunk(BPTR module, ULONG stack, CONST_STRPTR args, ULONG length)
{
    ++f.hunk_calls; event('H');
    check(module == (BPTR)(uintptr_t)f.module && f.module_live, "RunHunk receives original live module");
    check(stack == 5000 && args == f.arguments && length == ARGUMENTS_LEN,
          "RunHunk receives requested stack and unchanged arguments");
    SetIoErr(f.run_error);
    return f.run_result;
}
static void fixture_exit_callback(IPTR result, IPTR data)
{
    ++f.callbacks; event('X');
    f.callback_result = (LONG)result; f.callback_error = IoErr();
    check(data == 0x12345678, "exit callback receives its original data");
    check(f.args_live && f.module_live && f.segments_live && f.cli_live,
          "exit callback runs before resource cleanup");
}
static ULONG TypeOfMem(APTR memory)
{
    if (memory == &f.dos || memory == &f.engine) return 1;
    for (unsigned i = 0; i < 4; ++i)
        if (memory == f.streams[i] && f.stream_live[i]) return 1;
    return 0;
}
static void freeLocalVars(struct Process *process, struct DosLibrary *dos)
{ check(process == &f.process && dos == &f.dos, "cleanup belongs to the dying Process"); ++f.free_vars; event('V'); cleanup_error(); }
static BOOL Close(BPTR stream)
{
    for (unsigned i = 0; i < 4; ++i) if (stream == (BPTR)(uintptr_t)f.streams[i]) {
        check(f.stream_live[i], "owned stream is closed at most once");
        ++f.closes[i]; event('S'); f.stream_live[i] = FALSE; free(f.streams[i]); cleanup_error(); return TRUE;
    }
    check(FALSE, "cleanup closes only startup streams, not a replaced selector"); return FALSE;
}
static void FreeVec(APTR memory)
{
    if (memory == f.arguments) {
        check(f.args_live, "arguments freed once"); ++f.frees_args; f.args_live = FALSE;
    } else if (memory == f.segments) {
        check(f.segments_live, "SegArray freed once"); ++f.frees_segments; f.segments_live = FALSE;
    } else check(FALSE, "FreeVec owns its allocation");
    event('F'); free(memory); cleanup_error();
}
static void UnLoadSeg(BPTR module)
{
    check(module == (BPTR)(uintptr_t)f.module && f.module_live, "unloads the original module exactly once");
    ++f.unloads; event('U'); f.module_live = FALSE; free(f.module); cleanup_error();
}
static void UnLock(BPTR lock)
{
    if (lock == 0x1010) ++f.unlocks_current;
    else if (lock == 0x2020) ++f.unlocks_home;
    else check(FALSE, "only process-owned directories are unlocked");
    event('L'); cleanup_error();
}
static void FreeDosObject(ULONG kind, APTR memory)
{
    check(kind == DOS_CLI && memory == f.cli && f.cli_live, "frees the original CLI exactly once");
    ++f.frees_cli; f.cli_live = FALSE; event('C'); free(memory); cleanup_error();
}
static void removefromrootnode(struct Process *process, struct DosLibrary *dos)
{ check(process == &f.process && dos == &f.dos, "removes dying CLI process from root"); ++f.root_removals; event('R'); }
static void internal_ChildFree(APTR process, struct DosLibrary *dos)
{ check(process == &f.process && dos == &f.dos, "completion uses dying Process"); ++f.completions; event('Y'); }
static void Signal(struct Task *task, ULONG signals)
{ check(task == &f.parent && signals == SIGF_CHILD, "notifies its own parent"); ++f.notifications; event('P'); }

#include "production_dosentry.inc"

static APTR allocate(size_t size)
{ APTR result = calloc(1, size); if (!result) abort(); return result; }
static void prepare(const char *description, BOOL owned)
{
    case_name = description;
    memset(&f, 0, sizeof(f));
    f.process.pr_Task.tc_Node.ln_Name = "dosentry-fixture";
    f.etask.et_Parent = &f.parent;
    f.dos.dl_GV = &f.dos;
    f.process.pr_GlobVec = f.dos.dl_GV;
    f.arguments = allocate(sizeof(k_arguments));
    memcpy(f.arguments, k_arguments, sizeof(k_arguments));
    f.args_live = TRUE;
    f.segments = allocate(7 * sizeof(BPTR)); f.segments_live = TRUE;
    f.module = allocate(32); f.module_live = TRUE;
    f.segments[0] = 6; f.segments[3] = (BPTR)(uintptr_t)f.module;
    f.cli = allocate(sizeof(*f.cli)); f.cli_live = TRUE;
    for (unsigned i = 0; i < 4; ++i) { f.streams[i] = allocate(sizeof(*f.streams[i])); f.stream_live[i] = TRUE; }
    f.replacement = allocate(sizeof(*f.replacement));
    f.cli->cli_StandardError = (BPTR)(uintptr_t)f.streams[3];
    f.process.pr_Arguments = f.arguments;
    f.process.pr_SegList = (BPTR)(uintptr_t)f.segments;
    f.process.pr_CIS = (BPTR)(uintptr_t)f.streams[0];
    f.process.pr_COS = (BPTR)(uintptr_t)f.streams[1];
    f.process.pr_CES = (BPTR)(uintptr_t)f.streams[2];
    f.process.pr_CLI = (BPTR)(uintptr_t)f.cli;
    f.process.pr_CurrentDir = 0x1010; f.process.pr_HomeDir = 0x2020;
    f.process.pr_StackSize = 5000;
    f.process.pr_ExitCode = (APTR)fixture_exit_callback; f.process.pr_ExitData = 0x12345678;
    f.process.pr_Result2 = (SIPTR)native_entry;
    if (owned) f.process.pr_Flags = PRF_CLOSEINPUT | PRF_CLOSEOUTPUT | PRF_CLOSEERROR |
        PRF_FREEARGS | PRF_FREESEGLIST | PRF_FREECURRDIR | PRF_FREECLI |
        PRF_CLOSECLIERROR | PRF_SYNCHRONOUS | PRF_NOTIFYONDEATH;
    f.query_result = 1; f.marker = TRUE; f.engine_available = TRUE; f.has_module = TRUE;
    f.native_result = 9; f.native_error = 2468; f.run_result = 7; f.run_error = 1234;
}
static void cleanup_checks(BOOL owned)
{
    check(f.free_vars == 1 && f.frees_segments == 1 && f.closes_dos == 1,
          "all dispatch paths reach common process cleanup exactly once");
    check(f.frees_args == (unsigned)owned && f.unloads == (unsigned)(owned && f.has_module) &&
          f.frees_cli == (unsigned)owned && f.root_removals == (unsigned)owned,
          "ownership flags control arguments, module and CLI disposal");
    check(f.unlocks_current == (unsigned)owned && f.unlocks_home == 1,
          "current-directory flag and home-directory ownership remain intact");
    BOOL streams_ok = TRUE;
    for (unsigned i = 0; i < 4; ++i) streams_ok &= f.closes[i] == (unsigned)owned;
    check(streams_ok, "only flagged startup streams are closed exactly once");
    check(f.completions == (unsigned)owned && f.notifications == (unsigned)owned,
          "synchronous completion and death notification survive dispatch refusal");
    if (owned) {
        char *completion = memchr(f.events, 'Y', f.event_count), *notification = memchr(f.events, 'P', f.event_count);
        check(completion && notification && completion < notification,
              "synchronous completion precedes death notification");
        check(f.process.pr_CLI == BNULL, "freed CLI is cleared from Process");
    }
    check(f.events[f.event_count - 1] == 'Z', "own DOS library closes after all process cleanup");
    if (f.args_live) free(f.arguments);
    if (f.segments_live) free(f.segments);
    if (f.module_live) free(f.module);
    if (f.cli_live) free(f.cli);
    for (unsigned i = 0; i < 4; ++i) if (f.stream_live[i]) free(f.streams[i]);
    free(f.replacement);
}
static void missing_engine(void)
{
    for (unsigned variant = 0; variant < 4; ++variant) {
        BOOL owned = variant != 1;
        prepare("recognized 68k, missing emulator", owned);
        f.engine_available = FALSE;
        f.process.pr_Result2 = variant == 2 ? 0 : (SIPTR)(uintptr_t)((char *)f.module + sizeof(BPTR));
        f.clobber_cleanup = variant == 3;
        if (variant == 1) f.process.pr_ExitCode = NULL;
        DosEntry();
        check(f.opens_engine == 1 && !f.closes_engine && !f.hunk_calls,
              "failed emulator open neither executes nor closes an absent library");
        check(!f.native_calls && !f.native_entries && !f.wrong_entry_calls,
              "recognized 68k code never reaches native CallEntry");
        if (variant != 1) {
            check(f.callbacks == 1 && f.callback_result == RETURN_FAIL,
                  "exit callback receives RETURN_FAIL for unavailable emulator");
            check(f.callback_error == ERROR_INVALID_RESIDENT_LIBRARY,
                  "exit callback observes deterministic required-library error before cleanup");
        }
        if (!f.clobber_cleanup)
            check(IoErr() == ERROR_INVALID_RESIDENT_LIBRARY, "missing engine sets child process IoErr");
        cleanup_checks(owned);
    }
}
static void emulated_entry(void)
{
    const LONG results[] = {-1, 0, 7, 20};
    for (unsigned i = 0; i < sizeof(results) / sizeof(results[0]); ++i) {
        LONG result = results[i];
        prepare("recognized 68k, working emulator", TRUE);
        f.run_result = result;
        DosEntry();
        check(f.hunk_calls == 1 && !f.native_calls && !f.native_entries,
              "recognized 68k executes RunHunk exactly once without native fallback");
        check(f.opens_engine == 1 && f.closes_engine == 1,
              "emulator library reference is balanced");
        check(f.callbacks == 1 && f.callback_result == result && f.callback_error == f.run_error,
              "RunHunk result and secondary error reach unchanged exit callback");
        cleanup_checks(TRUE);
    }
}
static void native_dispatch(void)
{
    for (unsigned variant = 0; variant < 4; ++variant) {
        prepare("native entry control", TRUE);
        f.marker = FALSE;
        f.query_result = variant == 1 ? 0 : 1;
        if (variant == 2) { f.segments[3] = BNULL; f.has_module = FALSE; }
        f.mutate_stream = variant == 3;
        DosEntry();
        check(f.native_calls == 1 && f.native_entries == 1 && !f.wrong_entry_calls && !f.hunk_calls,
              "ordinary native entry still executes exactly once");
        check(!f.opens_engine && !f.closes_engine && f.queries == (unsigned)(variant != 2),
              "native and empty-segment paths never open the emulator");
        check(f.callbacks == 1 && f.callback_result == f.native_result && f.callback_error == f.native_error,
              "native result and IoErr behavior remain unchanged");
        cleanup_checks(TRUE);
    }
}
int main(void)
{
    missing_engine();
    emulated_entry();
    native_dispatch();
    printf("DOS process entry: %u checks, %u failed\n", checks, failures);
    return failures ? 1 : 0;
}
