/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    libssh_threads_aros.c - single-threaded crypto threading hooks.

    Replaces libssh's src/threads/mbedtls.c: the AROS mbedTLS linklibs are
    built without MBEDTLS_THREADING_C (mbedtls_aros_config.h) and libssh is
    registered with the threads_noop callbacks, so the only thing the
    backend file would contribute is its no-threading fallthrough - which
    does not compile (an upstream '#warn' typo) and would reject non-noop
    callbacks at runtime anyway.
*/

#include "config.h"

#include <string.h>

#include <libssh/libssh.h>
#include <libssh/threads.h>
#include <libssh/callbacks.h>

int crypto_thread_init(struct ssh_threads_callbacks_struct *user_callbacks)
{
    if (user_callbacks == NULL) {
        return SSH_OK;
    }
    if (strcmp(user_callbacks->type, "threads_noop") == 0) {
        return SSH_OK;
    }
    /* mbedTLS is built single-threaded on AROS */
    return SSH_ERROR;
}

void crypto_thread_finalize(void)
{
}
