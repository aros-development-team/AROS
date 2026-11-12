#!/usr/bin/env python3
# Copyright (C) 2026 John KNIPPER <code@jkn.me>.
# Distributed under the AROS Public License, version 1.1.
"""Run fat-handler's production OpReplaceFile() on a modelled directory.

OpReplaceFile() and CheckDirWritable() are taken from ops.c and
NormalizeDirEntryName() from names.c. The directory entry, lock and cluster
calls they make are modelled. After every directory write the model checks
that the target name still resolves to the old or the new data, so no step
of the replacement leaves it missing. Each refusal or injected failure must
leave both names on their original data, free no cluster and release every
lock it took.
"""
import argparse
import pathlib
import shlex
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cc', default='cc')
parser.add_argument('--source', type=pathlib.Path, help='ops.c to test')
parser.add_argument('--require-sanitizers', action='store_true',
                    help='fail when the host compiler cannot build with ASan/UBSan')
args = parser.parse_args()
here = pathlib.Path(__file__).resolve().parent
fat = here.parents[4] / 'rom/filesys/fat'
sys.dont_write_bytecode = True
sys.path.insert(0, str(here.parents[1]))
from host_sanitizers import sanitizer_options


def function(text, signature):
    start = text.index(signature)
    brace = text.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


ops = (args.source or fat / 'ops.c').read_text(encoding='latin-1')
names = (fat / 'names.c').read_text(encoding='latin-1')
production = '\n'.join([
    function(names, 'LONG NormalizeDirEntryName('),
    function(ops, 'static LONG CheckDirWritable('),
    function(ops, 'LONG OpReplaceFile('),
])

prefix = r'''
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
typedef int BOOL;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef uint16_t UWORD;
typedef uint8_t UBYTE;
typedef unsigned char *STRPTR;
#define TRUE 1
#define FALSE 0
#define EXCLUSIVE_LOCK -1
#define ATTR_READ_ONLY 0x01
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define FAT_MAX_LONG_FILENAME 255
#define ERROR_OBJECT_IN_USE 202
#define ERROR_OBJECT_NOT_FOUND 205
#define ERROR_INVALID_COMPONENT_NAME 210
#define ERROR_OBJECT_WRONG_TYPE 212
#define ERROR_DELETE_PROTECTED 222
#define ERROR_WRITE_PROTECTED 223
#define ERROR_UNKNOWN 1000
#define AROS_LE2WORD(x) (x)
#define FIRST_FILE_CLUSTER(de) \
    (AROS_LE2WORD((de)->e.entry.first_cluster_lo) | \
     (((ULONG) AROS_LE2WORD((de)->e.entry.first_cluster_hi)) << 16))
#define D(x)

struct FATDirEntry {
    UBYTE attr, create_time_tenth;
    UWORD create_time, create_date, last_access_date, first_cluster_hi;
    UWORD write_time, write_date, first_cluster_lo;
    ULONG file_size;
};
struct GlobalLock { struct GlobalLock *next; ULONG dir_cluster, dir_entry; UBYTE name[108]; };
struct LockList { struct GlobalLock *head; };
struct FSInfo { struct LockList locks; };
struct FSSuper { ULONG rootdir_cluster; struct FSInfo *info; };
struct Globals { struct FSSuper *sb; };
struct IOHandle { struct FSSuper *sb; ULONG first_cluster; };
struct DirHandle { struct IOHandle ioh; };
struct ExtFileLock { struct IOHandle ioh; struct GlobalLock *gl; };
struct DirEntry { struct FSSuper *sb; ULONG cluster, index; union { struct FATDirEntry entry; } e; };
#define ForeachNode(list, n) for ((n) = (list)->head; (n) != NULL; (n) = (n)->next)

/* The modelled volume: the root at cluster 0 and "sub" at cluster 10 */
#define SUB 10
struct Model { int used; ULONG dir, index; char name[32]; struct FATDirEntry entry; };
static struct Model disk[8];
static UBYTE dir_attr[2];
static struct FSInfo info;
static struct FSSuper super = { 0, &info };
static struct Globals globals = { &super };
static struct GlobalLock source_gl;
static int handles, locks, writes, notifies, renames, fail_update, fail_delete, fail_free;
static ULONG freed;
static char renamed_from[32], renamed_to[32];

static struct Model *slot(ULONG dir, ULONG index)
{
    for (unsigned i = 0; i < sizeof(disk) / sizeof(disk[0]); i++)
        if (disk[i].used && disk[i].dir == dir && disk[i].index == index)
            return &disk[i];
    return NULL;
}
static struct Model *named(ULONG dir, const char *name)
{
    for (unsigned i = 0; i < sizeof(disk) / sizeof(disk[0]); i++)
        if (disk[i].used && disk[i].dir == dir && !strcasecmp(disk[i].name, name))
            return &disk[i];
    return NULL;
}
static ULONG cluster_of(const struct Model *m) { return m->entry.first_cluster_lo | (ULONG)m->entry.first_cluster_hi << 16; }

/* Whatever has been written so far, the target name holds old or new data */
static ULONG target_dir;
static void target_present(void)
{
    struct Model *t = named(target_dir, "target");
    assert(t != NULL && (cluster_of(t) == 100 || cluster_of(t) == 200 || cluster_of(t) == 0x10200));
}

static void InitDirHandle(struct FSSuper *sb, ULONG cluster, struct DirHandle *dh, BOOL reuse, struct Globals *glob)
{
    (void)reuse; (void)glob;
    dh->ioh.sb = sb;
    dh->ioh.first_cluster = cluster == 0 ? sb->rootdir_cluster : cluster;
    handles++;
}
static LONG ReleaseDirHandle(struct DirHandle *dh, struct Globals *glob) { (void)dh; (void)glob; assert(handles > 0); handles--; return 0; }
static LONG MoveToSubdir(struct DirHandle *dh, UBYTE **pname, ULONG *plen, struct Globals *glob)
{
    char *name = (char *)*pname, *colon = memchr(name, ':', *plen);
    (void)glob;
    if (colon) { *plen -= colon + 1 - name; name = colon + 1; }
    if (*plen >= 4 && !strncmp(name, "sub/", 4)) { dh->ioh.first_cluster = SUB; name += 4; *plen -= 4; }
    else if (memchr(name, '/', *plen)) return ERROR_OBJECT_NOT_FOUND;
    *pname = (UBYTE *)name;
    return 0;
}
static void fill(struct DirEntry *de, struct Model *m)
{
    de->sb = &super; de->cluster = m->dir; de->index = m->index; de->e.entry = m->entry;
}
static LONG GetDirEntryByName(struct DirHandle *dh, STRPTR name, ULONG len, struct DirEntry *de, struct Globals *glob)
{
    char buf[64];
    (void)glob;
    snprintf(buf, sizeof(buf), "%.*s", (int)len, (char *)name);
    struct Model *m = named(dh->ioh.first_cluster, buf);
    if (m == NULL) return ERROR_OBJECT_NOT_FOUND;
    fill(de, m);
    return 0;
}
/* Only the dot entry of "sub" is read */
static LONG GetDirEntry(struct DirHandle *dh, ULONG index, struct DirEntry *de, struct Globals *glob)
{
    (void)glob;
    assert(index == 0 && dh->ioh.first_cluster == SUB);
    memset(de, 0, sizeof(*de));
    de->e.entry.attr = ATTR_DIRECTORY | dir_attr[1];
    return 0;
}
static LONG UpdateDirEntry(struct DirEntry *de, struct Globals *glob)
{
    (void)glob;
    if (fail_update && ++writes == fail_update) return ERROR_UNKNOWN;
    struct Model *m = slot(de->cluster, de->index);
    assert(m);
    m->entry = de->e.entry;
    target_present();
    return 0;
}
static LONG DeleteDirEntry(struct DirEntry *de, struct Globals *glob)
{
    (void)glob;
    if (fail_delete) return ERROR_UNKNOWN;
    struct Model *m = slot(de->cluster, de->index);
    assert(m);
    m->used = 0;
    target_present();
    return 0;
}
static LONG LockFile(ULONG dir, ULONG index, LONG access, struct ExtFileLock **lock, struct Globals *glob)
{
    struct GlobalLock *gl;
    assert(access == EXCLUSIVE_LOCK);
    ForeachNode(&glob->sb->info->locks, gl)
        if (gl->dir_cluster == dir && gl->dir_entry == index)
            return ERROR_OBJECT_IN_USE;
    *lock = calloc(1, sizeof(**lock));
    assert(*lock);
    locks++;
    return 0;
}
static void FreeLock(struct ExtFileLock *lock, struct Globals *glob) { (void)glob; assert(lock && locks > 0); locks--; free(lock); }
static LONG FreeClusterChain(struct FSSuper *sb, ULONG cluster)
{
    (void)sb;
    assert(locks == 0 && freed == 0);
    freed = cluster;
    return fail_free ? ERROR_UNKNOWN : 0;
}
static void SendNotifyByDirEntry(struct FSSuper *sb, struct DirEntry *de) { (void)sb; (void)de; notifies++; }
static LONG GetDirEntryShortName(struct DirEntry *de, STRPTR name, ULONG *len, struct Globals *glob)
{
    (void)glob;
    struct Model *m = slot(de->cluster, de->index);
    assert(m);
    *len = strlen(m->name);
    for (ULONG i = 0; i < *len; i++) name[i] = toupper((unsigned char)m->name[i]);
    return 0;
}
static LONG GetDirEntryLongName(struct DirEntry *de, STRPTR name, ULONG *len)
{
    struct Model *m = slot(de->cluster, de->index);
    assert(m);
    *len = strlen(m->name);
    memcpy(name, m->name, *len);
    return 0;
}
static LONG OpRenameFile(struct ExtFileLock *sl, UBYTE *sname, ULONG slen, struct ExtFileLock *dl, UBYTE *dname, ULONG dlen, struct Globals *glob)
{
    (void)sl; (void)dl; (void)glob;
    assert(handles == 0 && locks == 0);
    renames++;
    snprintf(renamed_from, sizeof(renamed_from), "%.*s", (int)slen, (char *)sname);
    snprintf(renamed_to, sizeof(renamed_to), "%.*s", (int)dlen, (char *)dname);
    return 0;
}
'''

suffix = r'''
static void entry(int i, ULONG dir, ULONG index, const char *name, UBYTE attr, ULONG cluster, ULONG size, UWORD date)
{
    memset(&disk[i], 0, sizeof(disk[i]));
    disk[i].used = 1; disk[i].dir = dir; disk[i].index = index;
    snprintf(disk[i].name, sizeof(disk[i].name), "%s", name);
    disk[i].entry.attr = attr;
    disk[i].entry.first_cluster_lo = cluster & 0xffff;
    disk[i].entry.first_cluster_hi = cluster >> 16;
    disk[i].entry.file_size = size;
    disk[i].entry.write_date = disk[i].entry.create_date = date;
}
static struct Model saved[8];
static void reset(void)
{
    memset(disk, 0, sizeof(disk));
    entry(0, 0, 3, "target", 0, 100, 1000, 1);
    entry(1, 0, 5, "new", 0, 200, 2000, 2);
    entry(2, 0, 7, "dir", ATTR_DIRECTORY, 300, 0, 3);
    entry(3, 0, 9, "sub", ATTR_DIRECTORY, SUB, 0, 4);
    entry(4, SUB, 2, "staged", 0, 0x10200, 3000, 5);
    entry(5, SUB, 4, "target", 0, 100, 1000, 1);
    dir_attr[0] = dir_attr[1] = 0;
    info.locks.head = NULL;
    handles = locks = writes = notifies = renames = fail_update = fail_delete = fail_free = 0;
    freed = 0;
    target_dir = 0;
    memcpy(saved, disk, sizeof(disk));
}
static LONG replace(const char *from, const char *to)
{
    LONG err = OpReplaceFile(NULL, (UBYTE *)from, strlen(from), NULL, (UBYTE *)to, strlen(to), &globals);
    assert(handles == 0 && locks == 0);
    return err;
}
/* A refusal changes nothing on disk and frees nothing */
static void unchanged(void)
{
    assert(!memcmp(disk, saved, sizeof(disk)) && freed == 0 && renames == 0);
}

int main(void)
{
    struct Model *m;
    int scenarios = 0;

    /* Replacement in one directory. A lock on the source follows it */
    reset(); scenarios++;
    source_gl.next = NULL; source_gl.dir_cluster = 0; source_gl.dir_entry = 5;
    info.locks.head = &source_gl;
    assert(replace("V:new", "V:target") == 0);
    m = named(0, "target");
    assert(m && m->index == 3 && cluster_of(m) == 200 && m->entry.file_size == 2000);
    assert(m->entry.create_date == 2 && (m->entry.attr & ATTR_ARCHIVE));
    assert(named(0, "new") == NULL && freed == 100 && notifies == 2);
    assert(source_gl.dir_cluster == 0 && source_gl.dir_entry == 3);
    assert(source_gl.name[0] == 6 && !memcmp(source_gl.name + 1, "target", 6));

    /* Replacement from another directory */
    reset(); scenarios++;
    assert(replace("sub/staged", "target") == 0);
    m = named(0, "target");
    assert(m && cluster_of(m) == 0x10200 && m->entry.file_size == 3000);
    assert(named(SUB, "staged") == NULL && freed == 100);

    /* Replacement inside "sub" */
    reset(); scenarios++; target_dir = SUB;
    assert(replace("sub/staged", "sub/target") == 0);
    assert(cluster_of(named(SUB, "target")) == 0x10200 && named(0, "target") && freed == 100);

    /* No target, or both names on one entry: a rename with the names given */
    reset(); scenarios++;
    assert(replace("V:new", "V:fresh") == 0 && renames == 1);
    assert(!strcmp(renamed_from, "V:new") && !strcmp(renamed_to, "V:fresh") && freed == 0);
    reset(); scenarios++;
    assert(replace("target", "TARGET") == 0 && renames == 1 && freed == 0);

    /* A name that a new entry would store differently finds the target */
    reset(); scenarios++;
    assert(replace("new", "target. ") == 0 && renames == 0 && freed == 100);
    assert(cluster_of(named(0, "target")) == 200);

    /* Refusals */
    reset(); scenarios++;
    assert(replace("missing", "target") == ERROR_OBJECT_NOT_FOUND); unchanged();
    reset(); scenarios++;
    assert(replace("new", "dir") == ERROR_OBJECT_WRONG_TYPE); unchanged();
    reset(); scenarios++;
    assert(replace("dir", "target") == ERROR_OBJECT_WRONG_TYPE); unchanged();
    reset(); scenarios++;
    assert(replace("dir", "fresh") == ERROR_OBJECT_WRONG_TYPE); unchanged();
    reset(); scenarios++;
    assert(replace("dir", "dir") == ERROR_OBJECT_WRONG_TYPE); unchanged();
    reset(); scenarios++; disk[0].entry.attr |= ATTR_READ_ONLY; memcpy(saved, disk, sizeof(disk));
    assert(replace("new", "target") == ERROR_DELETE_PROTECTED); unchanged();
    reset(); scenarios++;
    source_gl.next = NULL; source_gl.dir_cluster = 0; source_gl.dir_entry = 3;
    info.locks.head = &source_gl;
    assert(replace("new", "target") == ERROR_OBJECT_IN_USE); unchanged();
    reset(); scenarios++; dir_attr[1] = ATTR_READ_ONLY;
    assert(replace("sub/staged", "target") == ERROR_WRITE_PROTECTED); unchanged();
    reset(); scenarios++; dir_attr[1] = ATTR_READ_ONLY;
    assert(replace("new", "sub/target") == ERROR_WRITE_PROTECTED); unchanged();

    /* Failed writes leave the original entries */
    reset(); scenarios++; fail_update = 1;
    assert(replace("new", "target") == ERROR_UNKNOWN); unchanged();
    reset(); scenarios++; fail_delete = 1;
    assert(replace("new", "target") == ERROR_UNKNOWN); unchanged();

    /* Freeing the old clusters fails after the names are final */
    reset(); scenarios++; fail_free = 1;
    assert(replace("new", "target") == ERROR_UNKNOWN);
    assert(cluster_of(named(0, "target")) == 200 && named(0, "new") == NULL && freed == 100);

    printf("PASS fat-handler replace: %d scenarios\n", scenarios);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix='aros-fat-replace-') as temporary:
    work = pathlib.Path(temporary)
    unit = work / 'replace.c'
    unit.write_text(prefix + production + suffix, encoding='latin-1')
    binary = work / 'replace'
    compiler = shlex.split(args.cc)
    sanitizers = sanitizer_options(compiler, args.require_sanitizers)
    subprocess.run(compiler + ['-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                               '-Wno-pointer-sign', '-Wno-unused-parameter'] + sanitizers +
                   [str(unit), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
