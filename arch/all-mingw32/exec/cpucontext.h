/*
    Copyright � 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _CPUCONTEXT_H
#define _CPUCONTEXT_H

#include <sys/types.h>
#include <sys/param.h>
#include <signal.h>
#include <errno.h>

#include "etask.h"

/* This was taken from Mingw32's w32api/winnt.h */
#ifdef __i386__
#define SIZE_OF_80387_REGISTERS	80
#define CONTEXT_i386	0x10000
#define CONTEXT_i486	0x10000
#define CONTEXT_CONTROL	(CONTEXT_i386|0x00000001L)
#define CONTEXT_INTEGER	(CONTEXT_i386|0x00000002L)
#define CONTEXT_SEGMENTS	(CONTEXT_i386|0x00000004L)
#define CONTEXT_FLOATING_POINT	(CONTEXT_i386|0x00000008L)
#define CONTEXT_DEBUG_REGISTERS	(CONTEXT_i386|0x00000010L)
#define CONTEXT_EXTENDED_REGISTERS (CONTEXT_i386|0x00000020L)
#define CONTEXT_FULL	(CONTEXT_CONTROL|CONTEXT_INTEGER|CONTEXT_SEGMENTS)
#define MAXIMUM_SUPPORTED_EXTENSION  512
typedef struct _FLOATING_SAVE_AREA {
	IPTR	ControlWord;
	IPTR	StatusWord;
	IPTR	TagWord;
	IPTR	ErrorOffset;
	IPTR	ErrorSelector;
	IPTR	DataOffset;
	IPTR	DataSelector;
	UBYTE	RegisterArea[80];
	IPTR	Cr0NpxState;
} FLOATING_SAVE_AREA;
struct AROSCPUContext {
	IPTR	ContextFlags;
	IPTR	Dr0;
	IPTR	Dr1;
	IPTR	Dr2;
	IPTR	Dr3;
	IPTR	Dr6;
	IPTR	Dr7;
	FLOATING_SAVE_AREA FloatSave;
	IPTR	SegGs;
	IPTR	SegFs;
	IPTR	SegEs;
	IPTR	SegDs;
	IPTR	Edi;
	IPTR	Esi;
	IPTR	Ebx;
	IPTR	Edx;
	IPTR	Ecx;
	IPTR	Eax;
	IPTR	Ebp;
	IPTR	Eip;
	IPTR	SegCs;
	IPTR	EFlags;
	IPTR	Esp;
	IPTR	SegSs;
	BYTE	ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
};

#define PRINT_CPUCONTEXT(ctx) \
	kprintf ("    ContextFlags: 0x%08lX\n" \
		 "    ESP=%08lx  EBP=%08lx  EIP=%08lx\n" \
		 "    EAX=%08lx  EBX=%08lx  ECX=%08lx  EDX=%08lx\n" \
		 "    EDI=%08lx  ESI=%08lx  EFLAGS=%08lx\n" \
	    , ctx->ContextFlags \
	    , ctx->Esp \
	    , ctx->Ebp \
	    , ctx->Eip \
	    , ctx->Eax \
	    , ctx->Ebx \
	    , ctx->Ecx \
	    , ctx->Edx \
	    , ctx->Edi \
	    , ctx->Esi \
	    , ctx->EFlags \
      );

#define PREPARE_INITIAL_CONTEXT(ctx, sp, pc) ctx->Ebp = 0;			 \
					     ctx->Eip = (IPTR)pc;		 \
					     ctx->Esp = (IPTR)sp;		 \
					     ctx->ContextFlags = CONTEXT_CONTROL;
#else
#error Unsupported CPU type
#endif

/* Put a value of type SP_TYPE on the stack or get it off the stack. */
#define _PUSH(sp,val)       (*--sp = (SP_TYPE)(val))
#define _POP(sp)            (*sp++)

#define SP_TYPE		IPTR

#define GetSP(task)		(*(SP_TYPE **)(&task->tc_SPReg))

#endif /* _CPUCONTEXT_H */
