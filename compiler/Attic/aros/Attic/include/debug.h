/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    Debugging macros.

    This include file can be included several times !
*/
#ifndef CLIB_AROS_PROTOS_H
#   include <clib/aros_protos.h>
#endif

#ifndef DEBUG
#   define DEBUG 0
#endif

/* Remove all macros. They get new values each time this file is
    included */
#undef D
#undef D2
#undef ReturnVoid
#undef ReturnPtr
#undef ReturnStr
#undef ReturnInt
#undef ReturnXInt
#undef ReturnFloat
#undef ReturnSpecial
#undef ReturnBool

#if DEBUG
#   define D(x)     x

#   if DEBUG > 1
#	define D2(x)    x
#   else
#	define D2(x)    /* eps */
#endif

    /* return-macros. NOTE: I make a copy of the value in __aros_val, because
       the return-value might have side effects (like return x++;). */
#   define ReturnVoid(name)         { kprintf ("Exit " name "()\n"); return; }
#   define ReturnPtr(name,type,val) { type __aros_val = (type)val; \
				    kprintf ("Exit " name "=%08lx\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnStr(name,type,val) { type __aros_val = (type)val; \
				    kprintf ("Exit " name "=\"%s\"\n", \
				    __aros_val); return __aros_val; }
#   define ReturnInt(name,type,val) { type __aros_val = (type)val; \
				    kprintf ("Exit " name "=%ld\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnXInt(name,type,val) { type __aros_val = (type)val; \
				    kprintf ("Exit " name "=%lx\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnFloat(name,type,val) { type __aros_val = (type)val; \
				    kprintf ("Exit " name "=%g\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnSpecial(name,type,val,fmt) { type __aros_val = (type)val; \
				    kprintf ("Exit " name "=" fmt "\n", \
				    (ULONG)__aros_val); return __aros_val; }
#   define ReturnBool(name,val)     { BOOL __aros_val = (val != 0); \
				    kprintf ("Exit " name "=%s\n", \
				    __aros_val ? "TRUE" : "FALSE"); \
				    return __aros_val; }
#else /* !DEBUG */
#   define D(x)     /* eps */
#   define D2(x)     /* eps */

#   define ReturnVoid(name)                 return
#   define ReturnPtr(name,type,val)         return val
#   define ReturnStr(name,type,val)         return val
#   define ReturnInt(name,type,val)         return val
#   define ReturnXInt(name,type,val)        return val
#   define ReturnFloat(name,type,val)       return val
#   define ReturnSpecial(name,type,val,fmt) return val
#   define ReturnBool(name,val)             return val
#endif /* DEBUG */

#ifndef AROS_DEBUG_H
#define AROS_DEBUG_H

#define bug	 kprintf

#endif /* AROS_DEBUG_H */
