#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Distributed under the AROS Public License, version 1.1.
"""Run ram-handler's production rename/replace code on a modelled directory tree.

The RenameObject() body and its CmdRenameObject()/CmdReplaceObject() wrappers
are taken from commands.c and compiled against a small object model. Every
scenario checks that a successful replacement leaves the target name on the
source's data with the source name gone, and that every refusal leaves both
names exactly as they were.
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
sys.dont_write_bytecode = True
sys.path.insert(0, str(here.parents[1]))
from host_sanitizers import sanitizer_options

source = args.source or here.parents[4] / 'rom/filesys/ram/commands.c'
text = source.read_text(encoding='latin-1')
start = text.index('static BOOL RenameObject(')
end = text.index('/****i* ram.handler/CmdRenameDisk', start)
production = text[start:end]

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef int BOOL;
typedef int32_t LONG;
typedef unsigned char TEXT;
typedef char *STRPTR;
#define TRUE 1
#define FALSE 0
#define ST_FILE -3
#define ST_USERDIR 2
#define ST_SOFTLINK 3
#define ST_LINKFILE -4
#define ERROR_NO_FREE_STORE 103
#define ERROR_OBJECT_IN_USE 202
#define ERROR_OBJECT_EXISTS 203
#define ERROR_OBJECT_NOT_FOUND 205
#define ERROR_OBJECT_WRONG_TYPE 212
#define ERROR_DISK_WRITE_PROTECTED 214
#define ERROR_DELETE_PROTECTED 222
#define ERROR_READ_PROTECTED 224
#define FIBF_DELETE 1
#define RAM_ACCESS_WRITE 1
#define RAM_ACCESS_DELETE 2
struct Node { struct Node *ln_Succ; int ln_Pri; char *ln_Name; };
struct List { int unused; };
struct Object { struct Node node; struct Object *parent; void *lock; int protection; int data; int deleted; struct List elements; };
struct Lock { int unused; };
struct Handler { int locked; };

enum { ROOT, SUB, TARGET, SOURCE, OTHER, SUBFILE, OBJECTS };
static struct Object objects[OBJECTS];
static char names[OBJECTS][32];
static LONG error;
static int fail_setname, fail_security_at, security_calls, deletions;

static LONG IoErr(void) { return error; }
static void SetIoErr(LONG value) { error = value; }

/* Paths are "name" in ROOT or "sub/name" in SUB */
static struct Object *GetObjectNoFollow(struct Handler *h, struct Lock *l, const TEXT *path, struct Object **parent)
{
    const char *name = (const char *)path;
    struct Object *dir = &objects[ROOT];
    (void)h; (void)l;
    if (!strncmp(name, "sub/", 4)) { dir = &objects[SUB]; name += 4; }
    if (parent) *parent = dir;
    for (int i = TARGET; i < OBJECTS; i++)
        if (!objects[i].deleted && objects[i].parent == dir && !strcmp(objects[i].node.ln_Name, name))
            { error = 0; return &objects[i]; }
    error = ERROR_OBJECT_NOT_FOUND;
    return NULL;
}
static struct Object *GetRealObject(struct Object *o) { return o; }
static LONG ramSecCheckProperty(struct Handler *h, struct Object *o) { (void)h; (void)o; return ++security_calls == fail_security_at ? ERROR_READ_PROTECTED : 0; }
static LONG ramSecCheckObject(struct Handler *h, struct Object *o, int access) { (void)h; (void)o; (void)access; return ++security_calls == fail_security_at ? ERROR_READ_PROTECTED : 0; }
static char *FilePart(STRPTR path) { char *slash = strrchr(path, '/'); return slash ? slash + 1 : path; }
static BOOL SetName(struct Handler *h, struct Object *o, char *name)
{
    (void)h;
    if (fail_setname) { error = ERROR_NO_FREE_STORE; return FALSE; }
    int i = (int)(o - objects);
    snprintf(names[i], sizeof(names[i]), "%s", name);
    o->node.ln_Name = names[i];
    return TRUE;
}
static void NotifyAll(struct Handler *h, struct Object *o, BOOL all) { (void)h; (void)o; (void)all; }
static void UnmatchNotifyRequests(struct Handler *h, struct Object *o) { (void)h; (void)o; }
static void MatchNotifyRequests(struct Handler *h) { (void)h; }
static void RematchNotifyRequests(struct Handler *h) { (void)h; }
static void AdjustExaminations(struct Handler *h, struct Object *o) { (void)h; (void)o; }
static void DeleteObject(struct Handler *h, struct Object *o) { (void)h; assert(!o->deleted && !o->lock); o->deleted = 1; deletions++; }
static void Remove(struct Node *n) { (void)n; }
static void AddTail(struct List *l, struct Node *n) { (void)l; (void)n; }
'''

suffix = r'''
static void object(int i, int type, struct Object *parent, const char *name, int data)
{
    memset(&objects[i], 0, sizeof(objects[i]));
    objects[i].node.ln_Pri = type;
    objects[i].parent = parent;
    snprintf(names[i], sizeof(names[i]), "%s", name);
    objects[i].node.ln_Name = names[i];
    objects[i].data = data;
}
static void reset(void)
{
    object(ROOT, ST_USERDIR, NULL, "", 0);
    object(SUB, ST_USERDIR, &objects[ROOT], "sub", 0);
    object(TARGET, ST_FILE, &objects[ROOT], "target", 1);
    object(SOURCE, ST_FILE, &objects[ROOT], "new", 2);
    object(OTHER, ST_USERDIR, &objects[ROOT], "dir", 3);
    object(SUBFILE, ST_FILE, &objects[SUB], "staged", 4);
    error = fail_setname = fail_security_at = security_calls = deletions = 0;
}
static int data_at(const char *path)
{
    struct Object *o = GetObjectNoFollow(NULL, NULL, (const TEXT *)path, NULL);
    return o ? o->data : 0;
}
/* A refusal leaves both names on their original data */
static void refused(const char *from, const char *to, int from_data, int to_data, LONG expected)
{
    struct Handler h = { 0 };
    assert(!CmdReplaceObject(&h, NULL, (STRPTR)from, NULL, (STRPTR)to));
    assert(error == expected);
    assert(data_at(from) == from_data && data_at(to) == to_data && deletions == 0);
}

int main(void)
{
    struct Handler h = { 0 };
    int scenarios = 0;

    /* Rename still refuses an existing name */
    reset(); scenarios++;
    assert(!CmdRenameObject(&h, NULL, "new", NULL, "target") && error == ERROR_OBJECT_EXISTS);
    assert(data_at("new") == 2 && data_at("target") == 1);

    /* Replacement in one directory */
    reset(); scenarios++;
    assert(CmdReplaceObject(&h, NULL, "new", NULL, "target") && error == 0);
    assert(data_at("target") == 2 && data_at("new") == 0 && objects[TARGET].deleted && deletions == 1);

    /* Replacement from another directory */
    reset(); scenarios++;
    assert(CmdReplaceObject(&h, NULL, "sub/staged", NULL, "target"));
    assert(data_at("target") == 4 && data_at("sub/staged") == 0 && objects[SUBFILE].parent == &objects[ROOT]);

    /* No target: a rename */
    reset(); scenarios++;
    assert(CmdReplaceObject(&h, NULL, "new", NULL, "fresh"));
    assert(data_at("fresh") == 2 && data_at("new") == 0 && deletions == 0);

    /* The same object twice changes nothing */
    reset(); scenarios++;
    assert(CmdReplaceObject(&h, NULL, "target", NULL, "target"));
    assert(data_at("target") == 1 && deletions == 0);

    /* Refusals */
    reset(); scenarios++; objects[TARGET].lock = &objects[TARGET];
    refused("new", "target", 2, 1, ERROR_OBJECT_IN_USE);
    reset(); scenarios++; objects[TARGET].protection = FIBF_DELETE;
    refused("new", "target", 2, 1, ERROR_DELETE_PROTECTED);
    reset(); scenarios++;
    refused("new", "dir", 2, 3, ERROR_OBJECT_WRONG_TYPE);
    reset(); scenarios++;
    refused("dir", "target", 3, 1, ERROR_OBJECT_WRONG_TYPE);
    reset(); scenarios++;
    refused("dir", "fresh", 3, 0, ERROR_OBJECT_WRONG_TYPE);
    reset(); scenarios++; objects[TARGET].node.ln_Pri = ST_SOFTLINK;
    refused("new", "target", 2, 1, ERROR_OBJECT_WRONG_TYPE);
    reset(); scenarios++; objects[SOURCE].node.ln_Pri = ST_LINKFILE;
    refused("new", "target", 2, 1, ERROR_OBJECT_WRONG_TYPE);
    reset(); scenarios++; h.locked = 1;
    assert(!CmdReplaceObject(&h, NULL, "new", NULL, "target") && error == ERROR_DISK_WRITE_PROTECTED);
    assert(data_at("new") == 2 && data_at("target") == 1 && deletions == 0); h.locked = 0;
    reset(); scenarios++;
    refused("missing", "target", 0, 1, ERROR_OBJECT_NOT_FOUND);

    /* Each security check, including the delete check on the target */
    for (int at = 1; at <= 4; at++)
    {
        reset(); scenarios++; fail_security_at = at;
        refused("new", "target", 2, 1, ERROR_READ_PROTECTED);
    }

    /* The only step that can fail after the checks */
    reset(); scenarios++; fail_setname = 1;
    refused("new", "target", 2, 1, ERROR_NO_FREE_STORE);
    assert(!strcmp(objects[SOURCE].node.ln_Name, "new"));

    printf("PASS ram-handler replace: %d scenarios\n", scenarios);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix='aros-ram-replace-') as temporary:
    work = pathlib.Path(temporary)
    unit = work / 'replace.c'
    unit.write_text(prefix + production + suffix, encoding='latin-1')
    binary = work / 'replace'
    compiler = shlex.split(args.cc)
    sanitizers = sanitizer_options(compiler, args.require_sanitizers)
    subprocess.run(compiler + ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                               '-Wno-pointer-sign'] + sanitizers +
                   [str(unit), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
