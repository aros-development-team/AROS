#ifndef BOOPSISTUBS_H
#define BOOPSISTUBS_H
/*
**	$VER: BoopsiStubs.h 1.2 (1.9.97)
**
**	Copyright (C) 1997 Bernardo Innocenti. All rights reserved.
**
**	Use 4 chars wide TABs to read this file
**
**	Using these inline versions of the amiga.lib boopsi support functions
**	results in faster and smaller code against their linked library
**	counterparts. When debug is active, this function also validate the
**	parameters you pass in.
**
*/

#ifndef COMPILERSPECIFIC_H
#include "CompilerSpecific.h"
#endif /* COMPILERSPECIFIC_H */

#ifndef DEBUG_H
#include "Debug.h"
#endif /* DEBUG_H */


/* the _HookPtr type is a shortcut for a pointer to a hook function */

typedef ASMCALL ULONG (*_HookPtr)
	(REG(a0, Class *), REG(a2, Object *), REG(a1, APTR));

INLINE ULONG CoerceMethodA (struct IClass *cl, Object *o, Msg message)
{
	ASSERT_VALIDNO0(cl)
	ASSERT_VALID(o)

	return ((_HookPtr)cl->cl_Dispatcher.h_Entry) ((APTR)cl, (APTR)o, (APTR)message);
}

INLINE ULONG DoSuperMethodA (struct IClass *cl, Object *o, Msg message)
{
	ASSERT_VALIDNO0(cl)
	ASSERT_VALID(o)

	cl = cl->cl_Super;
	return ((_HookPtr)cl->cl_Dispatcher.h_Entry) ((APTR)cl, (APTR)o, (APTR)message);
}

INLINE ULONG DoMethodA (Object *o, Msg message)
{
	Class *cl;
	ASSERT_VALIDNO0(o)
	cl = OCLASS (o);
	ASSERT_VALIDNO0(cl)

	return ((_HookPtr)cl->cl_Dispatcher.h_Entry) ((APTR)cl, (APTR)o, (APTR)message);
}


	#define CoerceMethod(cl, o, msg...)												\
	({																				\
		ULONG _msg[] = { msg };														\
		ASSERT_VALIDNO0(cl)															\
		ASSERT_VALID(o)																\
		((_HookPtr)cl->cl_Dispatcher.h_Entry) ((APTR)cl, (APTR)o, (APTR)_msg);		\
	})

	#define DoSuperMethod(cl, o, msg...)											\
	({																				\
		Class *_cl;																	\
		ULONG _msg[] = { msg };														\
		ASSERT_VALID(o)																\
		ASSERT_VALIDNO0(cl)															\
		_cl = cl = cl->cl_Super;													\
		ASSERT_VALIDNO0(_cl)														\
		((_HookPtr)_cl->cl_Dispatcher.h_Entry) ((APTR)_cl, (APTR)o, (APTR)_msg);	\
	})

	#define DoMethod(o, msg...)														\
	({																				\
		Class *_cl;																	\
		ULONG _msg[] = { msg };														\
		ASSERT_VALIDNO0(o)															\
		_cl = OCLASS(o);															\
		ASSERT_VALIDNO0(_cl)														\
		((_HookPtr)_cl->cl_Dispatcher.h_Entry) ((APTR)_cl, (APTR)o, (APTR)_msg);	\
	})

	/* Var-args stub for the OM_NOTIFY method */
	#define NotifyAttrs(o, gi, flags, attrs...)										\
	({																				\
		Class *_cl;																	\
		ULONG _attrs[] = { attrs };													\
		ULONG _msg[] = { OM_NOTIFY, (ULONG)_attrs, (ULONG)gi, flags };				\
		ASSERT_VALIDNO0(o)															\
		_cl = OCLASS(o);															\
		ASSERT_VALIDNO0(_cl)														\
		ASSERT_VALID(gi)															\
		((_HookPtr)_cl->cl_Dispatcher.h_Entry) ((APTR)_cl, (APTR)o, (APTR)_msg);	\
	})

	/* Var-args stub for the OM_UPDATE method */
	#define UpdateAttrs(o, gi, flags, attrs...)										\
	({																				\
		Class *_cl;																	\
		ULONG _attrs[] = { attrs };													\
		ULONG _msg[] = { OM_UPDATE, (ULONG)_attrs, (ULONG)gi, flags };				\
		ASSERT_VALIDNO0(o)															\
		_cl = OCLASS(o);															\
		ASSERT_VALIDNO0(_cl)														\
		ASSERT_VALID(gi)															\
		((_HookPtr)_cl->cl_Dispatcher.h_Entry) ((APTR)_cl, (APTR)o, (APTR)_msg);	\
	})

/* Nobody else needs this anymore... */
#undef _HookPtr

#endif /* !BOOPSISTUBS_H */
