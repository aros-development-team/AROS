/**
 * Many similar implementations exist. See for example libwsbm
 * or the linux kernel include/atomic.h
 *
 * No copyright claimed on this file.
 *
 */

#ifndef U_ATOMIC_H
#define U_ATOMIC_H

#include "pipe/p_compiler.h"
#include "pipe/p_defines.h"

/* Favor OS-provided implementations.
 *
 * Where no OS-provided implementation is available, fall back to
 * locally coded assembly, compiler intrinsic or ultimately a
 * mutex-based implementation.
 */
#if (defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY) || \
     defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT))
#define PIPE_ATOMIC_OS_UNLOCKED
#elif defined(PIPE_OS_SOLARIS)
#define PIPE_ATOMIC_OS_SOLARIS
#elif defined(PIPE_CC_MSVC)
#define PIPE_ATOMIC_MSVC_INTRINSIC
#elif (defined(PIPE_CC_MSVC) && defined(PIPE_ARCH_X86))
#define PIPE_ATOMIC_ASM_MSVC_X86                
#elif (defined(PIPE_CC_GCC) && defined(PIPE_ARCH_X86))
#define PIPE_ATOMIC_ASM_GCC_X86
#elif (defined(PIPE_CC_GCC) && defined(PIPE_ARCH_ARM))
#define PIPE_ATOMIC_ASM_GCC_ARM
#elif (defined(PIPE_CC_GCC) && defined(PIPE_ARCH_X86_64))
#define PIPE_ATOMIC_ASM_GCC_X86_64
#elif defined(PIPE_OS_AROS) && defined(PIPE_ARCH_M68K)
#define PIPE_ATOMIC_OS_AROS_CPU_M68K
#elif defined(PIPE_CC_GCC) && (PIPE_CC_GCC_VERSION >= 401)
#define PIPE_ATOMIC_GCC_INTRINSIC
#else
#error "Unsupported platform"
#endif


#if defined(PIPE_ATOMIC_ASM_GCC_X86_64)
#define PIPE_ATOMIC "GCC x86_64 assembly"

#ifdef __cplusplus
extern "C" {
#endif

#define p_atomic_set(_v, _i) (*(_v) = (_i))
#define p_atomic_read(_v) (*(_v))

static INLINE boolean
p_atomic_dec_zero(int32_t *v)
{
   unsigned char c;

   __asm__ __volatile__("lock; decl %0; sete %1":"+m"(*v), "=qm"(c)
			::"memory");

   return c != 0;
}

static INLINE void
p_atomic_inc(int32_t *v)
{
   __asm__ __volatile__("lock; incl %0":"+m"(*v));
}

static INLINE void
p_atomic_dec(int32_t *v)
{
   __asm__ __volatile__("lock; decl %0":"+m"(*v));
}

static INLINE int32_t
p_atomic_cmpxchg(int32_t *v, int32_t old, int32_t _new)
{
   return __sync_val_compare_and_swap(v, old, _new);
}

#ifdef __cplusplus
}
#endif

#endif /* PIPE_ATOMIC_ASM_GCC_X86_64 */


#if defined(PIPE_ATOMIC_ASM_GCC_X86)

#define PIPE_ATOMIC "GCC x86 assembly"

#ifdef __cplusplus
extern "C" {
#endif

#define p_atomic_set(_v, _i) (*(_v) = (_i))
#define p_atomic_read(_v) (*(_v))

static INLINE boolean
p_atomic_dec_zero(int32_t *v)
{
   unsigned char c;

   __asm__ __volatile__("lock; decl %0; sete %1":"+m"(*v), "=qm"(c)
			::"memory");

   return c != 0;
}

static INLINE void
p_atomic_inc(int32_t *v)
{
   __asm__ __volatile__("lock; incl %0":"+m"(*v));
}

static INLINE void
p_atomic_dec(int32_t *v)
{
   __asm__ __volatile__("lock; decl %0":"+m"(*v));
}

static INLINE int32_t
p_atomic_cmpxchg(int32_t *v, int32_t old, int32_t _new)
{
   return __sync_val_compare_and_swap(v, old, _new);
}

#ifdef __cplusplus
}
#endif

#endif

#if defined(PIPE_ATOMIC_ASM_GCC_ARM)

#define PIPE_ATOMIC "GCC ARM assembly"

#ifdef __cplusplus
extern "C" {
#endif

#define p_atomic_set(_v, _i) (*(_v) = (_i))
#define p_atomic_read(_v) (*(_v))

static INLINE boolean
p_atomic_dec_zero(int32_t *v)
{
   unsigned long temp;
   int result;
   unsigned long cc;
   __asm__ __volatile__("\n1: ldrex %0, [%3]; subs %0, %0, #1; moveq %2, #1; movne %2, #0; strex %1, %0, [%3]; teq %1, #0; bne 1b"
						   :"=&r"(result), "=&r"(temp), "=&r"(cc)
						   :"r"(v)
						   :"cc");
   return cc;
}

static INLINE void
p_atomic_inc(int32_t *v)
{
   unsigned long temp;
   int result;
   __asm__ __volatile__("\n1: ldrex %0, [%2]; add %0, %0, #1; strex %1, %0, [%2]; teq %1, #0; bne 1b"
		   	   	   	   	   :"=&r"(result), "=&r"(temp)
		   	   	   	   	   :"r"(v)
		   	   	   	   	   :"cc");
}

static INLINE void
p_atomic_dec(int32_t *v)
{
   unsigned long temp;
   int result;
   __asm__ __volatile__("\n1: ldrex %0, [%2]; sub %0, %0, #1; strex %1, %0, [%2]; teq %1, #0; bne 1b"
			   	   	   	   :"=&r"(result), "=&r"(temp)
			   	   	   	   :"r"(v)
			   	   	   	   :"cc");
}

static INLINE int32_t
p_atomic_cmpxchg(int32_t *v, int32_t old, int32_t _new)
{
	  int32_t oldval;
	   unsigned long temp;
	   __asm__ __volatile__("\n1: ldrex %0,[%2]; teq %0, %3; strexeq %1, %4, [%2]; teq %1, #0; bne 1b"
	                                                   :"=&r"(oldval), "=&r"(temp)
	                                                   :"r"(v), "Ir"(old), "r"(_new)
	                                                   :"cc");
	   return oldval;
}

#ifdef __cplusplus
}
#endif

#endif


/* Implementation using GCC-provided synchronization intrinsics
 */
#if defined(PIPE_ATOMIC_GCC_INTRINSIC)

#define PIPE_ATOMIC "GCC Sync Intrinsics"

#ifdef __cplusplus
extern "C" {
#endif

#define p_atomic_set(_v, _i) (*(_v) = (_i))
#define p_atomic_read(_v) (*(_v))

static INLINE boolean
p_atomic_dec_zero(int32_t *v)
{
   return (__sync_sub_and_fetch(v, 1) == 0);
}

static INLINE void
p_atomic_inc(int32_t *v)
{
   (void) __sync_add_and_fetch(v, 1);
}

static INLINE void
p_atomic_dec(int32_t *v)
{
   (void) __sync_sub_and_fetch(v, 1);
}

static INLINE int32_t
p_atomic_cmpxchg(int32_t *v, int32_t old, int32_t _new)
{
   return __sync_val_compare_and_swap(v, old, _new);
}

#ifdef __cplusplus
}
#endif

#endif



/* Unlocked version for single threaded environments, such as some
 * windows kernel modules.
 */
#if defined(PIPE_ATOMIC_OS_UNLOCKED) 

#define PIPE_ATOMIC "Unlocked"

#define p_atomic_set(_v, _i) (*(_v) = (_i))
#define p_atomic_read(_v) (*(_v))
#define p_atomic_dec_zero(_v) ((boolean) --(*(_v)))
#define p_atomic_inc(_v) ((void) (*(_v))++)
#define p_atomic_dec(_v) ((void) (*(_v))--)
#define p_atomic_cmpxchg(_v, old, _new) (*(_v) == old ? *(_v) = (_new) : *(_v))

#endif


/* Locally coded assembly for MSVC on x86:
 */
#if defined(PIPE_ATOMIC_ASM_MSVC_X86)

#define PIPE_ATOMIC "MSVC x86 assembly"

#ifdef __cplusplus
extern "C" {
#endif

#define p_atomic_set(_v, _i) (*(_v) = (_i))
#define p_atomic_read(_v) (*(_v))

static INLINE boolean
p_atomic_dec_zero(int32_t *v)
{
   unsigned char c;

   __asm {
      mov       eax, [v]
      lock dec  dword ptr [eax]
      sete      byte ptr [c]
   }

   return c != 0;
}

static INLINE void
p_atomic_inc(int32_t *v)
{
   __asm {
      mov       eax, [v]
      lock inc  dword ptr [eax]
   }
}

static INLINE void
p_atomic_dec(int32_t *v)
{
   __asm {
      mov       eax, [v]
      lock dec  dword ptr [eax]
   }
}

static INLINE int32_t
p_atomic_cmpxchg(int32_t *v, int32_t old, int32_t _new)
{
   int32_t orig;

   __asm {
      mov ecx, [v]
      mov eax, [old]
      mov edx, [_new]
      lock cmpxchg [ecx], edx
      mov [orig], eax
   }

   return orig;
}

#ifdef __cplusplus
}
#endif

#endif


#if defined(PIPE_ATOMIC_MSVC_INTRINSIC)

#define PIPE_ATOMIC "MSVC Intrinsics"

#include <intrin.h>

#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedCompareExchange)

#ifdef __cplusplus
extern "C" {
#endif

#define p_atomic_set(_v, _i) (*(_v) = (_i))
#define p_atomic_read(_v) (*(_v))

static INLINE boolean
p_atomic_dec_zero(int32_t *v)
{
   return _InterlockedDecrement((long *)v) == 0;
}

static INLINE void
p_atomic_inc(int32_t *v)
{
   _InterlockedIncrement((long *)v);
}

static INLINE void
p_atomic_dec(int32_t *v)
{
   _InterlockedDecrement((long *)v);
}

static INLINE int32_t
p_atomic_cmpxchg(int32_t *v, int32_t old, int32_t _new)
{
   return _InterlockedCompareExchange((long *)v, _new, old);
}

#ifdef __cplusplus
}
#endif

#endif

#if defined(PIPE_ATOMIC_OS_SOLARIS)

#define PIPE_ATOMIC "Solaris OS atomic functions"

#include <atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

#define p_atomic_set(_v, _i) (*(_v) = (_i))
#define p_atomic_read(_v) (*(_v))

static INLINE boolean
p_atomic_dec_zero(int32_t *v)
{
   uint32_t n = atomic_dec_32_nv((uint32_t *) v);

   return n != 0;
}

#define p_atomic_inc(_v) atomic_inc_32((uint32_t *) _v)
#define p_atomic_dec(_v) atomic_dec_32((uint32_t *) _v)

#define p_atomic_cmpxchg(_v, _old, _new) \
	atomic_cas_32( (uint32_t *) _v, (uint32_t) _old, (uint32_t) _new)

#ifdef __cplusplus
}
#endif

#endif


#if defined(PIPE_ATOMIC_OS_AROS_CPU_M68K)

#define PIPE_ATOMIC "AROS OS atomic functions"

#include <aros/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

#define p_atomic_set(_v, _i) (*(_v) = (_i))
#define p_atomic_read(_v) (*(_v))

static INLINE boolean
p_atomic_dec_zero(int32_t *v)
{
   boolean n;
 
   /* FIXME: AROS needs an atomic decrement and return... */
   Disable();
   AROS_ATOMIC_DEC(*(LONG *)v);
   n = (*v == 0) ? TRUE : FALSE;
   Enable();

   return n;
}

#define p_atomic_inc(_v) AROS_ATOMIC_INC(*(LONG *)_v)
#define p_atomic_dec(_v) AROS_ATOMIC_DEC(*(LONG *)_v)

static INLINE int32_t
p_atomic_cmpxchg(int32_t *v, int32_t o, int32_t n)
{
	int32_t ret;

	/* FIXME: AROS needs an atomic cmpxchg, using CAS.
	 * However we can't do this if:
	 *  a) We are on a 68000 or
	 *  b) The 'v' points to Chip RAM (no r/m/w possible)
	 *
	 *  Settle for Disable()/Enable() for now.
	 */
	Disable();
	if (*v == o)
		*v = (n);
	ret = *v;
	Enable();

	return ret;
}

#ifdef __cplusplus
}
#endif

#endif


#ifndef PIPE_ATOMIC
#error "No pipe_atomic implementation selected"
#endif



#endif /* U_ATOMIC_H */
