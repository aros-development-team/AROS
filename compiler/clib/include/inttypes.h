#ifndef _INTTYPES_H_
#define _INTTYPES_H_

/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Standard fixed sized integral types.
*/

#include <aros/system.h>	    /* for restrict */
#include <aros/systypes.h>
#include <stdint.h>

/*
    TODO TODO TODO 

#if defined __cplusplus && defined __STDC_FORMAT_MACROS

    Macros for fprintf()

    Signed:
	PRId<N> PRIdLEAST<N> PRIdFAST<N> PRIdMAX PRIdPTR
	PRIi<N> PRIiLEAST<N> PRIiFAST<N> PRIiMAX PRIiPTR

    Unsigned:
	PRIo<N> PRIoLEAST<N> PRIoFAST<N> PRIiMAX PRIiPTR
	PRIu<N> PRIuLEAST<N> PRIuFAST<N> PRIiMAX PRIiPTR
	PRIx<N> PRIxLEAST<N> PRIxFAST<N> PRIxMAX PRIxPTR
	PRIX<N> PRIXLEAST<N> PRIXFAST<N> PRIXMAX PRIXPTR

    Macros for fscanf()
	SCNd<N> SCNdLEAST<N> SCNdFAST<N> SCNdMAX SCNdPTR
	SCNi<N> SCNiLEAST<N> SCNiFAST<N> SCNiMAX SCNiPTR

	SCNo<N> SCNoLEAST<N> SCNoFAST<N> SCNoMAX SCNoPTR
	SCNu<N> SCNuLEAST<N> SCNuFAST<N> SCNuMAX SCNuPTR
	SCNx<N> SCNxLEAST<N> SCNxFAST<N> SCNxMAX SCNxPTR

#endif
*/

#ifdef	_AROS_WCHAR_T_
typedef _AROS_WCHAR_T_	    wchar_t;
#undef	_AROS_WCHAR_T_
#endif

__BEGIN_DECLS

intmax_t strtoimax(const char * restrict nptr,
		    char ** restrict endptr, int base);
uintmax_t strtoumax(const char * restrict nptr,
		    char ** restrict endptr, int base);

intmax_t wcstoimax(const wchar_t * restrict nptr,
		    wchar_t ** restrict endptr, int base);
uintmax_t wcstoumax(const wchar_t * restrict nptr,
		    wchar_t ** restrict endptr, int base);

__END_DECLS

#endif /* _INTTYPES_H_ */
