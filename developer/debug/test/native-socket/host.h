/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
 * Licensed under the AROS Public License (APL), Version 1.1. */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdatomic.h>
#include <fcntl.h>
#include <stdarg.h>
typedef int16_t WORD;typedef uint16_t UWORD;typedef int32_t LONG; typedef uint32_t ULONG; typedef uint8_t UBYTE;
typedef uintptr_t IPTR;typedef intptr_t SIPTR;typedef int64_t QUAD;
typedef void *APTR;typedef const void *CONST_APTR;typedef int BOOL;
typedef char *caddr_t;typedef uint32_t socklen_t;
#define TRUE 1
#define FALSE 0
#define MEMF_ANY 0
#define MEMF_CLEAR 1
#define MEMF_PUBLIC 0
#define ENABLE_FDLIBRARY 1
#define AROS_LIBFUNC_INIT
#define AROS_LIBFUNC_EXIT
#define AROS_LHA(type,name,reg) type name
#define AROS_LH1(ret,name,a,bt,base,n,lib) ret name(a,bt base)
#define AROS_LH2(ret,name,a,b,bt,base,n,lib) ret name(a,b,bt base)
#define AROS_LH3(ret,name,a,b,c,bt,base,n,lib) ret name(a,b,c,bt base)
#define AROS_LH4(ret,name,a,b,c,d,bt,base,n,lib) ret name(a,b,c,d,bt base)
#define AROS_LH6(ret,name,a,b,c,d,e,f,bt,base,n,lib) ret name(a,b,c,d,e,f,bt base)
struct Library {ULONG lib_OpenCnt;ULONG lib_Version;};
struct SignalSemaphore {pthread_mutex_t mutex;};
static atomic_int allocations, fail_at;
static void ObtainSemaphore(struct SignalSemaphore *s){assert(!pthread_mutex_lock(&s->mutex));}
static void ReleaseSemaphore(struct SignalSemaphore *s){assert(!pthread_mutex_unlock(&s->mutex));}
static void *AllocVec(size_t size,int flags){
 (void)flags;int failure=atomic_load(&fail_at);
 if(failure>0&&atomic_fetch_sub(&fail_at,1)==1)return NULL;
 void *p=calloc(1,size);if(p)atomic_fetch_add(&allocations,1);return p;
}
static void FreeVec(void *p){if(p){atomic_fetch_sub(&allocations,1);free(p);}}
#define CopyMem(a,b,n) memcpy(b,a,n)
