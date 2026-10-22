/*
    Copyright (C) 2026, John KNIPPER <code@jkn.me>.
    Licensed under the AROS Public License (APL) Version 1.1.
*/
#ifndef AROS_RUST_SOCKET_NATIVE_H
#define AROS_RUST_SOCKET_NATIVE_H
#include <bsdsocket/native.h>
/* Returns native errno; zero denotes success. No pointer is retained. */
int aros_socket_native_call(struct aros_socket_request *request);
#endif
