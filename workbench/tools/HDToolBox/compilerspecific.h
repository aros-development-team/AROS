#ifndef COMPILERSPECIFIC_H
#define COMPILERSPECIFIC_H

#ifndef __AROS__
#ifndef IPTR
#define IPTR ULONG
#endif
#ifndef STACKIPTR
#define STACKIPTR ULONG
#endif
#endif

#undef 	REGARGS
#undef 	STDARGS
#undef  ALIGNED
#undef	CHIP
#undef 	ASM
#undef	SAVEDS

#ifdef __GNUC__

/* GCC */

#undef USE_ASM_FUNCS
#define USE_ASM_FUNCS 	    	    	0

#undef USE_OPTASM_FUNCS
#define USE_OPTASM_FUNCS    	    	0

#define REGPARAM(reg,type,name)     	register type name asm(#reg)
#define ASM_REGPARAM(reg,type,name) 	type name
#define OPT_REGPARAM(reg,type,name) 	type name

#define REGARGS
#define STDARGS
#define ALIGNED

/* #warning Fix CHIP macro for GCC compiler in compilerspecific.h
   
   Does not really matter, as there is only one place with a
   mouse pointer data structure which uses/needs this CHIP. And
   this is anyway only used on OS versions < 39 */
   
#define CHIP

#define ASM
#define SAVEDS

#else

/* SAS C */

#define REGPARAM(reg,type,name)     	register __ ## reg type name

#if USE_ASM_FUNCS
#   define ASM_REGPARAM(reg,type,name) 	register __ ## reg type name
#else
#   define ASM_REGPARAM(reg,type,name) 	type name
#endif

#if USE_OPTASM_FUNCS
#   define OPT_REGPARAM(reg,type,name) 	register __ ## reg type name
#else
#   define OPT_REGPARAM(reg,type,name) 	type name
#endif

#define REGARGS     	    	    	__regargs
#define STDARGS     	    	    	__stdargs
#define ALIGNED     	    	    	__aligned
#define CHIP	    	    	    	__chip
#define ASM 	    	    	    	__asm
#define SAVEDS	    	    	    	__saveds

#endif

#endif /* COMPILERSPECIFIC_H */

