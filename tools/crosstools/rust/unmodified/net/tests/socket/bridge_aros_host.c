/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL) Version 1.1.
*/
#include "socket_native.h"
#include <exec/libraries.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
static struct Library library;
static struct aros_socket_request *expected;
static int opens, closes, vectors, missing, response;
struct Library *OpenLibrary(CONST_STRPTR name, ULONG version) {
    assert(!strcmp(name, "bsdsocket.library") && version == 4); opens++;
    return missing ? NULL : &library;
}
void CloseLibrary(struct Library *base) { assert(base == &library); closes++; errno = EBADF; }
int test_vector(struct Library *base, struct aros_socket_request *request, ULONG size, int vector) {
    assert(base == &library && request == expected && size == sizeof(*request));
    assert(vector == 139 && opens == 1 && !closes); vectors++;
    request->result = 4711; return response;
}
static void reset(void) { opens = closes = vectors = missing = response = 0; }
int main(void) {
    struct aros_socket_request request = {0}; expected = &request;
    reset(); missing = 1;
    assert(aros_socket_native_call(&request) == ENETDOWN && opens == 1 && !closes && !vectors);
    reset(); library.lib_NegSize = (AROS_SOCKET_VECTOR + 1) * LIB_VECTSIZE - 1;
    assert(aros_socket_native_call(&request) == ENOSYS && opens == 1 && closes == 1 && !vectors);
    for (int i = 0; i < 3; i++) {
        reset(); library.lib_NegSize = (AROS_SOCKET_VECTOR + 1) * LIB_VECTSIZE + i;
        response = i == 0 ? 0 : (i == 1 ? EACCES : ENOSYS);
        assert(aros_socket_native_call(&request) == response);
        assert(opens == 1 && closes == 1 && vectors == 1 && request.result == 4711);
    }
    puts("PASS 5 native adapter cases; open/version/vector139/size/close/error pairing");
    return 0;
}
