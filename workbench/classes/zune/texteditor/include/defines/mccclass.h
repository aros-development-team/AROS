/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef _INLINE_MCCCLASS_H
#define _INLINE_MCCCLASS_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef MCCCLASS_BASE_NAME
#define MCCCLASS_BASE_NAME MCCClassBase
#endif /* !MCCCLASS_BASE_NAME */

#define MCC_Query(___which) \
	AROS_LC1(ULONG, MCC_Query, \
	AROS_LCA(LONG, (___which), D0), \
	struct Library *, MCCCLASS_BASE_NAME, 5, Mccclass)

#endif /* !_INLINE_MCCCLASS_H */
