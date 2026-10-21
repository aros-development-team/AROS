/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL) Version 1.1.
*/
#include <bsdsocket/native.h>
#include <exec/libraries.h>
int test_vector(struct Library *, struct aros_socket_request *, ULONG, int);
#define AROS_LCA(type, value, reg) value
#define AROS_LC2(type, name, a, b, basetype, base, vector, prefix) test_vector(base, a, b, vector)
