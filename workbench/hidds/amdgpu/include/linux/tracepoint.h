/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_TRACEPOINT_H_
#define _LINUX_TRACEPOINT_H_

#include <linux/types.h>

#define TP_PROTO(args...)       args
#define TP_ARGS(args...)        args
#define TP_STRUCT__entry(args...)
#define TP_fast_assign(args...)
#define TP_printk(fmt, args...)
#define TP_CONDITION(args...)

#define DECLARE_TRACE(name, proto, args) \
    static inline void trace_##name(proto) { } \
    static inline bool trace_##name##_enabled(void) { return false; }
#define TRACE_EVENT(name, proto, args, tstruct, assign, print) \
    DECLARE_TRACE(name, PARAMS(proto), PARAMS(args))
#define TRACE_EVENT_CONDITION(name, proto, args, cond, tstruct, assign, print) \
    DECLARE_TRACE(name, PARAMS(proto), PARAMS(args))
#define DECLARE_EVENT_CLASS(name, proto, args, tstruct, assign, print)
#define DEFINE_EVENT(template, name, proto, args) \
    DECLARE_TRACE(name, PARAMS(proto), PARAMS(args))
#define DEFINE_EVENT_PRINT(template, name, proto, args, print) \
    DECLARE_TRACE(name, PARAMS(proto), PARAMS(args))
#define PARAMS(args...)         args

#define __string(item, src)
#define __assign_str(dst)
#define __get_str(field)        ""
#define __dynamic_array(type, item, len)
#define __get_dynamic_array(field) NULL
#define __print_array(array, count, el_size) ""
#define __print_symbolic(value, symbol_array...) ""
#define __print_flags(flag, delim, flag_array...) ""

#endif
