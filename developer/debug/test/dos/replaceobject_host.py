#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Distributed under the AROS Public License, version 1.1.
"""Run the production ReplaceObject() body against modelled devices and packets.

The function body is taken from rom/dos/replaceobject.c and built twice: with
the allocating C2BSTR() of arch/m68k-all/dos/bstr_helper.c and with
AROS_FAST_BSTR names. Checks: the packet sent, refusal across devices,
failures of GetDeviceProc() and of name allocation, the handler's IoErr()
surviving the cleanup, and that every DevProc and name is freed.
"""
import argparse
import pathlib
import shlex
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='cc')
parser.add_argument('--source', type=pathlib.Path)
parser.add_argument('--require-sanitizers', action='store_true',
                    help='fail when the host compiler cannot build with ASan/UBSan')
args = parser.parse_args()
here = pathlib.Path(__file__).resolve().parent
root = here.parents[3]
sys.dont_write_bytecode = True
sys.path.insert(0, str(here.parent))
from host_sanitizers import sanitizer_options

source = (args.source or root / 'rom/dos/replaceobject.c').read_text(encoding='latin-1')
body = source.split('AROS_LIBFUNC_INIT', 1)[1].split('AROS_LIBFUNC_EXIT', 1)[0]
classic = (root / 'arch/m68k-all/dos/bstr_helper.c').read_text(encoding='latin-1')
classic = classic[classic.index('BSTR C2BSTR('):]
classic = classic[:classic.index('\n}\n') + 3]

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int BOOL;
typedef int32_t LONG;
typedef intptr_t SIPTR;
typedef uintptr_t BPTR;
typedef char *STRPTR;
typedef const char *CONST_STRPTR;
#define TRUE 1
#define FALSE 0
#define DOSFALSE 0
#define BNULL ((BPTR)0)
#define MEMF_ANY 0
#define ERROR_NO_FREE_STORE 103
#define ERROR_OBJECT_NOT_FOUND 205
#define ERROR_RENAME_ACROSS_DEVICES 215
#define ERROR_DEVICE_NOT_MOUNTED 218
#define ERROR_OBJECT_IN_USE 202
#define ACTION_REPLACE_OBJECT 1037
#define D(x)
struct DosLibrary { int unused; };
struct MsgPort { int id; };
struct DevProc { struct MsgPort *dvp_Port; BPTR dvp_Lock; };
static struct MsgPort port_a = { 1 }, port_b = { 2 };
static LONG error;
static int devprocs, names, allocations, device_calls, packets;
static int fail_device_at, fail_alloc_at, packet_error;
static struct { struct MsgPort *port; LONG action; BPTR lock1, lock2; char name1[64], name2[64]; } sent;
static LONG IoErr(void) { return error; }
static void SetIoErr(LONG value) { error = value; }
/* "B:" names are on a second handler; "A:dir/" assigns carry a lock */
static struct DevProc *GetDeviceProc(CONST_STRPTR name, struct DevProc *dp)
{
    assert(dp == NULL);
    if (++device_calls == fail_device_at) { error = ERROR_DEVICE_NOT_MOUNTED; return NULL; }
    struct DevProc *d = malloc(sizeof(*d));
    assert(d);
    devprocs++;
    d->dvp_Port = !strncmp(name, "B:", 2) ? &port_b : &port_a;
    d->dvp_Lock = !strncmp(name, "ASSIGN:", 7) ? 0x40 : 0;
    error = 0;
    return d;
}
/* FreeDeviceProc() and FreeVec() may change IoErr() */
static void FreeDeviceProc(struct DevProc *d) { assert(d && devprocs > 0); devprocs--; free(d); error = 999; }
#ifndef AROS_FAST_BSTR
typedef BPTR BSTR;
#define MKBADDR(p) ((BPTR)(uintptr_t)(p) >> 2)
#define BADDR(b) ((void *)((uintptr_t)(b) << 2))
static void *AllocVec(size_t size, int flags)
{
    (void)flags;
    if (++allocations == fail_alloc_at) return NULL;
    void *p = malloc(size);   /* aligned for BADDR() */
    assert(p);
    names++;
    return p;
}
static void FreeVec(void *p) { assert(p && names > 0); names--; free(p); error = 998; }
#define FREEC2BSTR(bstr) FreeVec(BADDR(bstr))
#define NAME(b) ((const char *)BADDR(b) + 1)
#define NAMELEN(b) (*(const unsigned char *)BADDR(b))
#else
typedef char *BSTR;
#define C2BSTR(x) ((char *)(x))
#define FREEC2BSTR(x) do { } while (0)
#define NAME(b) ((const char *)(b))
#define NAMELEN(b) strlen(b)
#endif
static SIPTR dopacket5f(SIPTR *res2, struct MsgPort *port, LONG action, SIPTR a1, SIPTR a2, SIPTR a3, SIPTR a4, SIPTR a5)
{
    assert(res2 == NULL && a5 == 0);
    packets++;
    sent.port = port; sent.action = action; sent.lock1 = (BPTR)a1; sent.lock2 = (BPTR)a3;
    assert(NAMELEN((BSTR)a2) == strlen(NAME((BSTR)a2)) && NAMELEN((BSTR)a4) == strlen(NAME((BSTR)a4)));
    snprintf(sent.name1, sizeof(sent.name1), "%s", NAME((BSTR)a2));
    snprintf(sent.name2, sizeof(sent.name2), "%s", NAME((BSTR)a4));
    error = packet_error;
    return packet_error == 0;
}
#define dopacket4(base, res2, port, action, a1, a2, a3, a4) \
    dopacket5f(res2, port, action, (SIPTR)(a1), (SIPTR)(a2), (SIPTR)(a3), (SIPTR)(a4), 0)
'''

suffix = r'''
static BOOL ReplaceObject(CONST_STRPTR source, CONST_STRPTR target)
{
    struct DosLibrary *DOSBase = NULL;
    (void)DOSBase;
BODY
}
static void reset(void)
{
    assert(devprocs == 0 && names == 0);
    error = allocations = device_calls = packets = 0;
    fail_device_at = fail_alloc_at = packet_error = 0;
    memset(&sent, 0, sizeof(sent));
}
static void balanced(void) { assert(devprocs == 0 && names == 0); }

int main(void)
{
    int scenarios = 0;

    /* The packet carries both locks and both names as given */
    reset(); scenarios++;
    assert(ReplaceObject("A:dir/new", "ASSIGN:target") && error == 0);
    assert(packets == 1 && sent.port == &port_a && sent.action == ACTION_REPLACE_OBJECT);
    assert(sent.lock1 == 0 && sent.lock2 == 0x40);
    assert(!strcmp(sent.name1, "A:dir/new") && !strcmp(sent.name2, "ASSIGN:target"));
    balanced();

    /* The handler's error survives the cleanup */
    reset(); scenarios++; packet_error = ERROR_OBJECT_IN_USE;
    assert(!ReplaceObject("A:new", "A:target") && error == ERROR_OBJECT_IN_USE && packets == 1);
    balanced();

    /* Names on different handlers */
    reset(); scenarios++;
    assert(!ReplaceObject("A:new", "B:target") && error == ERROR_RENAME_ACROSS_DEVICES && packets == 0);
    balanced();

    /* GetDeviceProc() failing for either name keeps its error */
    for (int at = 1; at <= 2; at++)
    {
        reset(); scenarios++; fail_device_at = at;
        assert(!ReplaceObject("A:new", "A:target") && error == ERROR_DEVICE_NOT_MOUNTED && packets == 0);
        balanced();
    }

#ifndef AROS_FAST_BSTR
    /* Either name failing to allocate */
    for (int at = 1; at <= 2; at++)
    {
        reset(); scenarios++; fail_alloc_at = at;
        assert(!ReplaceObject("A:new", "A:target") && error == ERROR_NO_FREE_STORE && packets == 0);
        balanced();
    }
#endif

    printf("PASS ReplaceObject (%s BSTR): %d scenarios\n",
#ifdef AROS_FAST_BSTR
        "fast",
#else
        "classic",
#endif
        scenarios);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix='aros-replaceobject-') as temporary:
    work = pathlib.Path(temporary)
    compiler = shlex.split(args.cc)
    sanitizers = sanitizer_options(compiler, args.require_sanitizers)
    for profile, flags, helper in (('classic', [], classic), ('fast', ['-DAROS_FAST_BSTR'], '')):
        unit = work / (profile + '.c')
        unit.write_text(prefix + '\n#ifndef AROS_FAST_BSTR\n' + helper + '\n#endif\n' +
                        suffix.replace('BODY', body), encoding='latin-1')
        binary = work / profile
        subprocess.run(compiler + ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                                   '-Wno-pointer-sign'] + flags + sanitizers +
                       [str(unit), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
