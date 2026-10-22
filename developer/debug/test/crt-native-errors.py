#!/usr/bin/env python3
# Copyright (C) 2026 John Knipper.
# Distributed under the AROS Public License, version 1.1.
"""Host regression of actual CRT bodies with a fake DOS transport.

Run: python3 developer/debug/test/crt-native-errors.py [--cc clang]
This is not native AROS execution, handler atomicity or an ABI test. Includes
are removed from production files; function bodies are otherwise unchanged.
"""
import argparse
import hashlib
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
FILES = {
    'stdc.inc': 'compiler/crt/stdc/rename.c',
    'error.inc': 'compiler/crt/posixc/__dos_error.h',
    'remove.inc': 'compiler/crt/posixc/remove.c',
    'rename.inc': 'compiler/crt/posixc/rename.c',
}
FIXTURE = r'''
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef long LONG;
typedef const char *CONST_STRPTR;
#define ERROR_OBJECT_EXISTS 203
#define ERROR_OBJECT_NOT_FOUND 205
#define ERROR_OBJECT_IN_USE 202
#define ERROR_ACTION_NOT_KNOWN 209
#define ERROR_NOT_IMPLEMENTED 236
static LONG native_error, next_error, delegated_error;
static int rename_ok, source_exists, target_exists, conversion_calls;
static int conversion_failure, duplicate_failure, delegated_calls, remove_ok;
static int mock_rename, rename_calls;
static const char *source_bytes, *target_bytes;
static unsigned checks;
#define CHECK(x) do { checks++; assert(x); } while (0)
static LONG IoErr(void) { return native_error; }
static LONG SetIoErr(LONG e) { LONG old=native_error; native_error=e; return old; }
static int __stdc_ioerr2errno(LONG e) {
    switch(e) {
    case 0: return 0;
    case ERROR_OBJECT_NOT_FOUND: return ENOENT;
    case ERROR_OBJECT_IN_USE: return EBUSY;
    default: return 1000+(int)e;
    }
}
static int Rename(CONST_STRPTR a, CONST_STRPTR b) {
    assert(a && b); rename_calls++;
    if (rename_ok) { source_exists=0; target_exists=1; target_bytes=source_bytes; return 1; }
    native_error=next_error; return 0;
}
#define rename tested_stdc_rename
#include "stdc.inc"
#undef rename
#include "error.inc"
static const char *convert(const char *p) {
    if (++conversion_calls==conversion_failure) { errno=EFAULT; return NULL; }
    return p;
}
static char *duplicate(const char *p) {
    char *result;
    if (duplicate_failure) return NULL;
    result=malloc(strlen(p)+1); assert(result); return strcpy(result,p);
}
static void cleanup(void *p) { free(p); errno=EACCES; native_error=999; }
static int delegated_remove(const char *p) {
    assert(p); delegated_calls++; native_error=delegated_error; errno=0;
    return remove_ok ? 0 : -1;
}
static int delegated_rename(const char *a,const char *b) {
    delegated_calls++;
    if (mock_rename) { native_error=delegated_error; errno=0; return -1; }
    return tested_stdc_rename(a,b);
}
#define __path_u2a convert
#define __stdcio_remove delegated_remove
#define remove tested_remove
#include "remove.inc"
#undef remove
#undef __stdcio_remove
#define __stdcio_rename delegated_rename
#define strdup duplicate
#define free cleanup
#define rename tested_rename
#include "rename.inc"
#undef rename
#undef free
#undef strdup
static void reset(void) {
    native_error=0; next_error=ERROR_OBJECT_EXISTS;
    delegated_error=ERROR_OBJECT_IN_USE;
    rename_ok=conversion_calls=conversion_failure=duplicate_failure=0;
    delegated_calls=remove_ok=mock_rename=rename_calls=0;
    source_exists=target_exists=1;
    source_bytes="replacement exact bytes"; target_bytes="old destination exact bytes";
    errno=0;
}
static void preserved(void) {
    CHECK(source_exists); CHECK(target_exists);
    CHECK(!strcmp(source_bytes,"replacement exact bytes"));
    CHECK(!strcmp(target_bytes,"old destination exact bytes"));
}
int main(void) {
    const LONG errors[]={ERROR_OBJECT_IN_USE,ERROR_ACTION_NOT_KNOWN,ERROR_NOT_IMPLEMENTED,0};
    unsigned i;
    reset(); rename_ok=1;
    CHECK(tested_stdc_rename("RAM:source","RAM:new")==0);
    CHECK(!source_exists); CHECK(!strcmp(target_bytes,source_bytes));
    reset(); CHECK(tested_stdc_rename("RAM:source","RAM:target")==-1);
    CHECK(native_error==ERROR_ACTION_NOT_KNOWN); CHECK(errno!=0); preserved();
    reset(); next_error=0;
    CHECK(tested_stdc_rename("RAM:source","RAM:target")==-1);
    CHECK(errno==EIO); CHECK(native_error==0); preserved();
    reset(); CHECK(tested_rename("RAM:source","RAM:target")==-1);
    CHECK(errno==ENOSYS); CHECK(native_error==ERROR_ACTION_NOT_KNOWN); preserved();
    for(i=0;i<sizeof(errors)/sizeof(errors[0]);i++) {
        int expected=errors[i]==ERROR_OBJECT_IN_USE ? EBUSY : errors[i]==0 ? EIO : ENOSYS;
        reset(); delegated_error=errors[i];
        CHECK(tested_remove("RAM:source")==-1); CHECK(errno==expected);
        CHECK(native_error==errors[i]); CHECK(delegated_calls==1);
        reset(); mock_rename=1; delegated_error=errors[i];
        CHECK(tested_rename("RAM:source","RAM:target")==-1);
        CHECK(errno==expected); CHECK(native_error==errors[i]); preserved();
    }
    reset(); conversion_failure=1;
    CHECK(tested_remove("RAM:source")==-1); CHECK(errno==EFAULT); CHECK(!delegated_calls);
    reset(); conversion_failure=2;
    CHECK(tested_rename("RAM:source","RAM:target")==-1); CHECK(errno==EFAULT); CHECK(!delegated_calls);
    reset(); duplicate_failure=1;
    CHECK(tested_rename("RAM:source","RAM:target")==-1); CHECK(errno==ENOMEM); CHECK(!delegated_calls);
    reset(); rename_ok=1;
    CHECK(tested_rename("RAM:source","RAM:new")==0); CHECK(!source_exists);
    reset(); remove_ok=1; CHECK(tested_remove("RAM:source")==0);
    printf("PASS checks=%u scope=host-fake-DOS-production-bodies\n",checks);
    return 0;
}
'''

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='cc')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='aros-crt-errors-') as temporary:
        out = Path(temporary)
        for destination, source in FILES.items():
            raw = (ROOT / source).read_text()
            (out / destination).write_text(re.sub(r'^\s*#include[^\n]*\n', '', raw, flags=re.M))
            print(source + ' sha256=' + hashlib.sha256(raw.encode()).hexdigest())
        (out / 'test.c').write_text(FIXTURE)
        subprocess.run(shlex.split(args.cc) + ['-std=c11', '-O1', '-Wall', '-Wextra', '-Werror',
                        str(out / 'test.c'), '-o', str(out / 'test')], check=True)
        subprocess.run([str(out / 'test')], check=True)

if __name__ == '__main__':
    main()
