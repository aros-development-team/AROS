/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Host test for the usergroup.library release in posixc's
    __optionallibs.c. The production file is compiled unchanged against
    host stand-ins for the exec calls it makes (see usergroup_release_host/),
    and the exit list is drained by stdc's own __callexitfuncs, also compiled
    unchanged. Build and run with usergroup_release_host.sh.

    Cases:
      release    the lazy open queues a release that closes usergroup when the
                 exit list is drained, after handlers registered earlier;
      nostdc     with no program C library base, usergroup is kept open and
                 nothing is queued (the previous behaviour);
      nomem      when the release cannot be queued, the open is undone and
                 usergroup is reported unavailable;
      noopen     when usergroup.library cannot be opened, nothing is queued.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <proto/exec.h>
#include "__posixc_intbase.h"
#include "__stdc_intbase.h"
#include "__exitfunc.h"

int __usergroup_available(struct PosixCIntBase *PosixCBase);
int __optionallibs_close(struct PosixCIntBase *PosixCBase);

struct ExecBase *SysBase;

static struct PosixCIntBase posixc;
static struct StdCIntBase stdc;
static struct StdCIntBase *stdc_ptr;
static struct Library usergroup;
static int opens, closes, allocs, fail_open, fail_alloc;
static int held_at_exit;       /* was usergroup still open in an earlier handler? */
static int failures;

void *__aros_getbase_PosixCBase(void) { return &posixc; }
void *__aros_getbase_StdCBase(void) { return stdc_ptr; }
int *__stdc_get_errorptr(void) { return NULL; }

struct Library *OpenLibrary(const char *name, unsigned long version)
{
    (void)version;
    if (strcmp(name, "usergroup.library") != 0 || fail_open)
        return NULL;
    opens++;
    return &usergroup;
}

void CloseLibrary(struct Library *lib)
{
    if (lib == &usergroup)
        closes++;
}

void *OpenResource(const char *name) { (void)name; return NULL; }
void Forbid(void) { }
void Permit(void) { }
struct Node *FindName(struct List *list, const char *name)
{
    (void)list; (void)name;
    return NULL;
}

void *test_malloc(size_t size)
{
    if (fail_alloc)
        return NULL;
    allocs++;
    return calloc(1, size);
}

/* An exit handler the program registered before its first getpwnam(): it
   runs after every handler registered later, but it must still see
   usergroup open, since it may read a struct passwd obtained earlier. */
static void program_handler(void)
{
    held_at_exit = posixc.PosixCUserGroupBase != NULL;
}

static void reset(int with_stdc)
{
    memset(&posixc, 0, sizeof(posixc));
    NEWLIST((struct List *)&stdc.atexit_list);
    NEWLIST((struct List *)&stdc.quick_exit_list);
    stdc_ptr = with_stdc ? &stdc : NULL;
    opens = closes = allocs = fail_open = fail_alloc = held_at_exit = 0;
}

static void check(const char *name, int ok, const char *detail)
{
    printf("USERGROUP RELEASE %-7s %s  %s\n", name, ok ? "PASS" : "FAIL", detail);
    if (!ok)
        failures++;
}

int main(void)
{
    static struct AtExitNode early = { .node.ln_Type = AEN_VOID };
    char msg[160];
    int r1, r2;

    /* Unbuffered, so that a crash still shows which case it happened in. */
    setvbuf(stdout, NULL, _IONBF, 0);

    /* release: registered early handler, then two lazy uses, then exit. */
    reset(1);
    early.func.fn = program_handler;
    ADDHEAD((struct List *)&stdc.atexit_list, &early.node);
    r1 = __usergroup_available(&posixc);
    r2 = __usergroup_available(&posixc);
    __callexitfuncs();
    snprintf(msg, sizeof(msg), "available=%d,%d opens=%d allocs=%d closes=%d open-in-earlier-handler=%d cache=%s",
             r1, r2, opens, allocs, closes, held_at_exit, posixc.PosixCUserGroupBase ? "set" : "clear");
    check("release", r1 && r2 && opens == 1 && allocs == 1 && closes == 1 && held_at_exit
          && posixc.PosixCUserGroupBase == NULL, msg);

    /* nostdc: no program C library base. */
    reset(0);
    r1 = __usergroup_available(&posixc);
    snprintf(msg, sizeof(msg), "available=%d opens=%d allocs=%d closes=%d cache=%s",
             r1, opens, allocs, closes, posixc.PosixCUserGroupBase ? "set" : "clear");
    check("nostdc", r1 && opens == 1 && allocs == 0 && closes == 0
          && posixc.PosixCUserGroupBase == &usergroup, msg);
    __optionallibs_close(&posixc);
    check("nostdc", closes == 1 && posixc.PosixCUserGroupBase == NULL,
          "released by __optionallibs_close as before");

    /* nomem: the release node cannot be allocated. */
    reset(1);
    fail_alloc = 1;
    r1 = __usergroup_available(&posixc);
    snprintf(msg, sizeof(msg), "available=%d opens=%d closes=%d cache=%s queued=%d",
             r1, opens, closes, posixc.PosixCUserGroupBase ? "set" : "clear",
             !IsListEmpty((struct List *)&stdc.atexit_list));
    check("nomem", !r1 && opens == 1 && closes == 1 && posixc.PosixCUserGroupBase == NULL
          && IsListEmpty((struct List *)&stdc.atexit_list), msg);
    fail_alloc = 0;
    r1 = __usergroup_available(&posixc);
    __callexitfuncs();
    snprintf(msg, sizeof(msg), "retry available=%d opens=%d closes=%d", r1, opens, closes);
    check("nomem", r1 && opens == 2 && closes == 2, msg);

    /* noopen: usergroup.library is not there. */
    reset(1);
    fail_open = 1;
    r1 = __usergroup_available(&posixc);
    snprintf(msg, sizeof(msg), "available=%d allocs=%d queued=%d", r1, allocs,
             !IsListEmpty((struct List *)&stdc.atexit_list));
    check("noopen", !r1 && allocs == 0 && IsListEmpty((struct List *)&stdc.atexit_list), msg);

    printf("USERGROUP RELEASE %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
