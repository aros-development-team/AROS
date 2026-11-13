/*
 * Configuration 
 *
 * Copyright © 2025 The AROS Dev Team.
 * Copyright © 1993 Pekka Pessi
  */

#ifdef __SASC
#define ASM         __asm 
#define SAVEDS      __saveds
#define FAR	                                //For crypt.c, was defined as empty in Smakefile
#define COMMON      __far
#define REG(x)      register __ ## x
#define USE_PRAGMAS 1
#else
#ifdef __GNUC__
#define ASM
#define COMMON
#define FAR
#define SAVEDS
#else
#error "Unsupported compiler"
#endif
#endif

#define INTERNAL static
