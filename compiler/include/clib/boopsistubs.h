#ifndef BOOPSISTUBS_H
#define BOOPSISTUBS_H

/*
**	$Id$
**
**	Copyright (C) 1997,1999,2000 Bernardo Innocenti <bernie@cosmos.it>
**	All rights reserved.
**
**	Using these inline versions of the amiga.lib boopsi support functions
**	results in faster and smaller code against their linked library
**	counterparts. When debug is active, these functions will also
**	validate the parameters you pass in.
*/

/* 
    The USE_BOOPSI_STUBS symbol prevents redefinition of the following stubs 
    with their amiga.lib equivalents. In AmigaOS, this trick works only if you
    are using patched version of <clib/alib_protos.h>
*/

#ifndef USE_BOOPSI_STUBS
#define USE_BOOPSI_STUBS
#endif /* USE_BOOPSI_STUBS */


#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif /* INTUITION_CLASSES_H */

#if defined(__AROS__)

	#ifndef AROS_DEBUG_H
	#include <aros/debug.h>
	#endif /* AROS_DEBUG_H */

	#ifndef AROS_ASMCALL_H
	#include <aros/asmcall.h>
	#endif /* AROS_ASMCALL_H */

	#define _CALL_DISPATCHER(entry, cl, o, msg) \
		AROS_UFC3(IPTR, entry, \
			AROS_UFCA(Class *, cl, A0), \
			AROS_UFCA(Object *, o, A2), \
			AROS_UFCA(APTR, msg, A1))

	#ifndef INLINE
	#define INLINE static inline
	#endif

#else /* !__AROS__ */

	#ifndef COMPILERSPECIFIC_H
	#include "CompilerSpecific.h"
	#endif /* COMPILERSPECIFIC_H */

	#ifndef DEBUGMACROS_H
	#include "DebugMacros.h"
	#endif /* DEBUGMACROS_H */

	/* the _HookPtr type is a shortcut for a pointer to a hook function */
	typedef ASMCALL IPTR (*HookPtr)
		(REG(a0, Class *), REG(a2, Object *), REG(a1, APTR));

	#define _CALL_DISPATCHER(entry, cl, o, msg) \
		((HookPtr)(entry))(cl, o, msg)

#endif /* __AROS__ */



/* SAS/C is clever enough to inline these even var-args functions, while gcc and egcs
 * refuse to do it (yikes!).  The GCC versions of these functions are macro blocks
 * similar to those  used in the inline/#?.h headers.
 */
#if defined(__SASC) || defined (__STORM__)

	INLINE ULONG CoerceMethodA(Class *cl, Object *o, Msg msg)
	{
		ASSERT_VALID_PTR(cl);
		ASSERT_VALID_PTR_OR_NULL(o);

		return _CALL_DISPATCHER(cl->cl_Dispatcher.h_Entry, cl, o, msg);
	}

	INLINE ULONG DoSuperMethodA(Class *cl, Object *o, Msg msg)
	{
		ASSERT_VALID_PTR(cl);
		ASSERT_VALID_PTR_OR_NULL(o);

		cl = cl->cl_Super;
		return _CALL_DISPATCHER(cl->cl_Dispatcher.h_Entry, cl, o, msg);
	}

	INLINE ULONG DoMethodA(Object *o, Msg msg)
	{
		Class *cl;
		ASSERT_VALID_PTR(o);
		cl = OCLASS (o);
		ASSERT_VALID_PTR(cl);

		return _CALL_DISPATCHER(cl->cl_Dispatcher.h_Entry, cl, o, msg);
	}

	INLINE ULONG CoerceMethod(Class *cl, Object *o, ULONG MethodID, ...)
	{
		ASSERT_VALID_PTR(cl);
		ASSERT_VALID_PTR_OR_NULL(o);

		return ((HookPtr)cl->cl_Dispatcher.h_Entry) ((APTR)cl, (APTR)o, (APTR)&MethodID);
	}

	INLINE ULONG DoSuperMethod(Class *cl, Object *o, ULONG MethodID, ...)
	{
		ASSERT_VALID_PTR(cl);
		ASSERT_VALID_PTR_OR_NULL(o);

		cl = cl->cl_Super;
		return ((HookPtr)cl->cl_Dispatcher.h_Entry) ((APTR)cl, (APTR)o, (APTR)&MethodID);
	}

	INLINE ULONG DoMethod(Object *o, ULONG MethodID, ...)
	{
		Class *cl;

		ASSERT_VALID_PTR(o);
		cl = OCLASS (o);
		ASSERT_VALID_PTR(cl);

		return ((HookPtr)cl->cl_Dispatcher.h_Entry) ((APTR)cl, (APTR)o, (APTR)&MethodID);
	}

	/* varargs stub for the OM_NOTIFY method */
	INLINE void NotifyAttrs(Object *o, struct GadgetInfo *gi, ULONG flags, Tag attr1, ...)
	{
		ASSERT_VALID_PTR(o);
		ASSERT_VALID_PTR_OR_NULL(gi);

		DoMethod(o, OM_NOTIFY, &attr1, gi, flags);
	}

	/* varargs stub for the OM_UPDATE method */
	INLINE void UpdateAttrs(Object *o, struct GadgetInfo *gi, ULONG flags, Tag attr1, ...)
	{
		ASSERT_VALID_PTR(o);
		ASSERT_VALID_PTR_OR_NULL(gi);

		DoMethod(o, OM_UPDATE, &attr1, gi, flags);
	}

	/* varargs stub for the OM_SET method. Similar to SetAttrs(), but allows
	 * to pass the GadgetInfo structure
	 */
	INLINE void SetAttrsGI(Object *o, struct GadgetInfo *gi, ULONG flags, Tag attr1, ...)
	{
		ASSERT_VALID_PTR(o);
		ASSERT_VALID_PTR_OR_NULL(gi);

		DoMethod(o, OM_SET, &attr1, gi, flags);
	}

#elif defined(__GNUC__)
    #ifndef CoerceMethodA
        #define CoerceMethodA(cl, o, msg) \
        ({ \
                ASSERT_VALID_PTR(cl); \
                ASSERT_VALID_PTR_OR_NULL(o); \
                _CALL_DISPATCHER(cl->cl_Dispatcher.h_Entry, cl, o, msg); \
        })
    #endif
    
    #ifndef DoSuperMethodA
        #define DoSuperMethodA(cl, o, msg) \
        ({ \
                Class *_cl; \
                ASSERT_VALID_PTR(cl); \
                ASSERT_VALID_PTR_OR_NULL(o); \
                _cl = cl->cl_Super; \
                ASSERT_VALID_PTR(_cl); \
                _CALL_DISPATCHER(_cl->cl_Dispatcher.h_Entry, _cl, o, msg); \
        })
    #endif
    
    #ifndef DoMethodA
        #define DoMethodA(o, msg) \
        ({ \
                Class *_cl; \
                ASSERT_VALID_PTR(o); \
                _cl = OCLASS(o); \
                ASSERT_VALID_PTR(_cl); \
                _CALL_DISPATCHER(_cl->cl_Dispatcher.h_Entry, _cl, o, msg); \
        })
    #endif
    
    #ifndef CoerceMethod
        #define CoerceMethod(cl, o, msg...) \
        ({ \
                IPTR _msg[] = { msg }; \
                ASSERT_VALID_PTR(cl); \
                ASSERT_VALID_PTR_OR_NULL(o); \
                _CALL_DISPATCHER(cl->cl_Dispatcher.h_Entry, cl, o, _msg); \
        })
    #endif
    
    #ifndef DoSuperMethod
        #define DoSuperMethod(cl, o, msg...) \
        ({ \
                Class *_cl; \
                IPTR _msg[] = { msg }; \
                ASSERT_VALID_PTR(cl); \
                ASSERT_VALID_PTR_OR_NULL(o); \
                _cl = cl->cl_Super; \
                ASSERT_VALID_PTR(_cl); \
                _CALL_DISPATCHER(_cl->cl_Dispatcher.h_Entry, _cl, o, _msg); \
        })
    #endif
    
    #ifndef DoMethod
        #define DoMethod(o, msg...) \
        ({ \
                Class *_cl; \
                IPTR _msg[] = { msg }; \
                ASSERT_VALID_PTR(o); \
                _cl = OCLASS(o); \
                ASSERT_VALID_PTR_OR_NULL(_cl); \
                _CALL_DISPATCHER(_cl->cl_Dispatcher.h_Entry, _cl, o, _msg); \
        })
    #endif
    
    #ifndef NotifyAttrs
        /* Var-args stub for the OM_NOTIFY method */
        #define NotifyAttrs(o, gi, flags, attrs...) \
        ({ \
                Class *_cl; \
                IPTR _attrs[] = { attrs }; \
                IPTR _msg[] = { OM_NOTIFY, (IPTR)_attrs, (IPTR)gi, flags }; \
                ASSERT_VALID_PTR(o); \
                _cl = OCLASS(o); \
                ASSERT_VALID_PTR(_cl); \
                ASSERT_VALID_PTR_OR_NULL(gi); \
                _CALL_DISPATCHER(_cl->cl_Dispatcher.h_Entry, _cl, o, _msg); \
        })
    #endif
    
    #ifndef UpdateAttrs
        /* Var-args stub for the OM_UPDATE method */
        #define UpdateAttrs(o, gi, flags, attrs...) \
        ({ \
                Class *_cl; \
                IPTR _attrs[] = { attrs }; \
                IPTR _msg[] = { OM_UPDATE, (IPTR)_attrs, (IPTR)gi, flags }; \
                ASSERT_VALID_PTR(o); \
                _cl = OCLASS(o); \
                ASSERT_VALID_PTR(_cl); \
                ASSERT_VALID_PTR_OR_NULL(gi); \
                _CALL_DISPATCHER(_cl->cl_Dispatcher.h_Entry, _cl, o, _msg); \
        })
    #endif
#endif

#endif /* !BOOPSISTUBS_H */
