/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
   Licensed under the AROS Public License (APL), Version 1.1. */
#include <aros/debug.h>
#include <libraries/posixc.h>
void aros_rs_consumer_trace(const char *event) { bug("[RUSTAPP_TRACE] %s\n", event); }
unsigned int aros_rs_consumer_runtime(void) {
    const struct PosixCBase *base = __aros_getbase_PosixCBase();
    unsigned int version = base ? ((unsigned int)base->lib.lib_Version << 16) | base->lib.lib_Revision : 0;
    bug("[RUSTAPP_RUNTIME] posix-version=%u revision=%u\n", version >> 16, version & 65535);
    return version;
}
