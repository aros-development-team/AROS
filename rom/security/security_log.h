/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: security.library - log.resource provider
*/
#ifndef _SECURITY_LOG_H
#define _SECURITY_LOG_H

#include <exec/types.h>
#include <resources/log.h>

struct SecurityBase;

/* Sub-components */
#define SECLOG_SUB_SERVER       "Security.server"
#define SECLOG_SUB_AUTH         "Security.auth"
#define SECLOG_SUB_LIBRARY      ""

extern void InitLogging(struct SecurityBase *secBase);

/* Post one entry (type: LOGF_Flag_Type_#?); fmt/argv as for VLogF(), fmt
 * already converted by FixFormat(). A no-op without log.resource. */
extern void VSecLog(struct SecurityBase *secBase, ULONG type, CONST_STRPTR sub, CONST_STRPTR fmt, SIPTR *argv);

#endif /* _SECURITY_LOG_H */
