/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
/* Host-only fault injection around the actual production duplication helper.
   cc -std=c99 -Wall -Wextra -Werror run_handle_host.c -o run_handle_host */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
typedef intptr_t BPTR;
typedef int32_t LONG;
struct DosLibrary { int unused; };
#define BNULL 0
#define OFFSET_CURRENT 0
#define OFFSET_BEGINNING -1
#define CHANGE_FH 1
#define SHARED_LOCK -2
#define ERROR_INVALID_LOCK 211
#define ERROR_OBJECT_IN_USE 202
#define ERROR_ACTION_NOT_KNOWN 209
#define ERROR_SEEK_ERROR 219
static LONG error, position, seek_error;
static int interactive, exclusive, share_ok, open_ok, restore_ok;
static unsigned seeks, duplicates, shares, unlocks, closes;
static LONG get_error(struct DosLibrary *base) { (void)base;return error; }
static void set_error(LONG e,struct DosLibrary *base) { (void)base;error=e; }
static int is_interactive(BPTR f,struct DosLibrary *base)
{ (void)base;assert(f==1);return interactive; }
static LONG seek_file(BPTR f,LONG offset,LONG mode,struct DosLibrary *base)
{
    (void)base;++seeks;
    if(f==1) { assert(offset==0 && mode==OFFSET_CURRENT);error=seek_error;return position; }
    assert(f==3 && offset==position && mode==OFFSET_BEGINNING);
    error=ERROR_SEEK_ERROR;return restore_ok?0:-1;
}
static BPTR duplicate(BPTR f,struct DosLibrary *base)
{
    (void)base;assert(f==1);++duplicates;
    if(exclusive && !shares) { error=ERROR_OBJECT_IN_USE;return 0; }
    return 2;
}
static int change_mode(LONG type,BPTR f,LONG mode,struct DosLibrary *base)
{ (void)base;assert(type==CHANGE_FH && f==1 && mode==SHARED_LOCK);++shares;error=223;return share_ok; }
static BPTR open_lock(BPTR l,struct DosLibrary *base)
{ (void)base;assert(l==2);error=103;return open_ok?3:0; }
static void unlock(BPTR l,struct DosLibrary *base)
{ (void)base;assert(l==2);++unlocks;error=0; }
static void close_file(BPTR f,struct DosLibrary *base)
{ (void)base;assert(f==3);++closes;error=0; }
#define IoErr() get_error(DOSBase)
#define SetIoErr(e) set_error((e),DOSBase)
#define IsInteractive(f) is_interactive((f),DOSBase)
#define Seek(f,p,m) seek_file((f),(p),(m),DOSBase)
#define DupLockFromFH(f) duplicate((f),DOSBase)
#define ChangeMode(t,f,m) change_mode((t),(f),(m),DOSBase)
#define OpenFromLock(l) open_lock((l),DOSBase)
#define UnLock(l) unlock((l),DOSBase)
#define Close(f) close_file((f),DOSBase)
#include "../../../../workbench/c/shellcommands/run_handle.h"
static void reset(void)
{
    error=seek_error=0;position=7;interactive=exclusive=0;
    share_ok=open_ok=restore_ok=1;
    seeks=duplicates=shares=unlocks=closes=0;
}
int main(void)
{
    reset();position=-1;seek_error=ERROR_SEEK_ERROR;
    assert(DuplicateRunHandle(1,NULL)==0 && error==ERROR_SEEK_ERROR && !duplicates);
    reset();position=INT32_MIN;
    assert(DuplicateRunHandle(1,NULL)==0 && error==ERROR_SEEK_ERROR && !duplicates);
    reset();position=-1;seek_error=ERROR_ACTION_NOT_KNOWN;
    assert(DuplicateRunHandle(1,NULL)==3 && seeks==1);
    reset();exclusive=1;
    assert(DuplicateRunHandle(1,NULL)==3 && shares==1 && duplicates==2 && seeks==2);
    reset();exclusive=1;share_ok=0;
    assert(DuplicateRunHandle(1,NULL)==0 && error==223 && duplicates==1);
    reset();open_ok=0;
    assert(DuplicateRunHandle(1,NULL)==0 && error==103 && unlocks==1 && !closes);
    reset();restore_ok=0;
    assert(DuplicateRunHandle(1,NULL)==0 && error==ERROR_SEEK_ERROR && closes==1 && !unlocks);
    reset();interactive=1;
    assert(DuplicateRunHandle(1,NULL)==3 && !seeks);
    reset();assert(DuplicateRunHandle(0,NULL)==0 && error==ERROR_INVALID_LOCK && !duplicates);
    puts("RUN HANDLE PASS: seek failures, range rejection, nonseekable, sharing, ownership");
    return 0;
}
