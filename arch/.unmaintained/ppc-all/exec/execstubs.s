/*
    Copyright ? 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
                                                                                
    Desc: Stubs to call C functions while preserving all registers
    Lang: english
*/
   #include "include/machine.i"
                                                                                
/*
    Macros:
   PUSH - Save all registers on the stack
   POP - Restore all registers from the stack
                                                                                
   STUB_ARG0(name) - Call a function preserving all registers
          which gets no arguments
   STUB_ARG1(name) - Call a function preserving all registers
          which gets a single argument
   STUB_ARG2(name) - Call a function preserving all registers
          which gets two arguments
*/

#define NUM_REGS   (32+2)
#define OFFSET     (NUM_REGS*4)
#define FIRST_ARG  ((NUM_REGS+0)*4)
#define SECOND_ARG ((NUM_REGS+1)*4)

#define PUSH \
	stmw 0,-OFFSET(1);  \
	li   1,-OFFSET;     \
	mflr 0;             \
	stw  0,OFFSET-8(1); \
	mfcr 0;             \
	stw  0,OFFSET-4(1)

#define POP  \
	lwz  0,OFFSET-4(1); \
	mtcr 0;             \
	lwz  0,OFFSET-8(1); \
	mtlr 0;             \
	lmw  2,8(1);        \
	lwz  0,0(1);        \
	lwz  1,4(1)

#define STUB_ARG0(name) \
	PUSH;             \
	lis  9,name@ha;   \
	la   0,name@l(9); \
	mtlr 0;           \
	blrl;             \
	POP;              \
	blrl

#define STUB_ARG1(name) \
	PUSH;                  \
	lwz  0,FIRST_ARG(1); \
	stw  0,-4(1);        \
	li   1,-4;         \
	lis  9,name@ha;    \
	la   0,name@l(9);  \
	mtlr 0;            \
	blrl;              \
	li   1,4;          \
	POP;               \
	blrl

#define STUB_ARG2(name) \
	PUSH;                   \
	lwz  0,SECOND_ARG(1); \
	stw  0,-4(1);         \
	lwz  0,FIRST_ARG(1);  \
	stw  0,-8(1);         \
	li   1,-8;          \
	lis  9,name@ha;     \
	la   0,name@l(9);   \
	mtlr 0;             \
	blrl;               \
	li   0,8;           \
	POP;                \
	blrl

/* To save typing work */
#define STUB0(cname,name)             \
   .globl   cname        ; \
   _FUNCTION(cname)      ; \
cname:                ; \
   STUB_ARG0(name)
                                                                                
#define STUB1(cname,name)             \
   .globl   cname        ; \
   _FUNCTION(cname)      ; \
cname:                ; \
   STUB_ARG1(name)
                                                                                
                                                                                
#define STUB2(cname,name) \
   .globl   cname       ; \
   _FUNCTION(cname)     ; \
cname:                  ; \
   STUB_ARG2(name)
                                                                                
   .text
   _ALIGNMENT

   /* Call functions and preserve registers */
#ifdef  UseExecstubs
   STUB1(AROS_SLIB_ENTRY(Forbid,Exec),AROS_CSYMNAME(_Exec_Forbid))
   STUB1(AROS_SLIB_ENTRY(Permit,Exec),AROS_CSYMNAME(_Exec_Permit))
   STUB1(AROS_SLIB_ENTRY(Disable,Exec),AROS_CSYMNAME(_Exec_Disable))
   STUB1(AROS_SLIB_ENTRY(Enable,Exec),AROS_CSYMNAME(_Exec_Enable))
                                                                                
   STUB2(AROS_SLIB_ENTRY(ObtainSemaphore,Exec),AROS_CSYMNAME(_Exec_ObtainSemaphore))
   STUB2(AROS_SLIB_ENTRY(ReleaseSemaphore,Exec),AROS_CSYMNAME(_Exec_ReleaseSemaphore))
   STUB2(AROS_SLIB_ENTRY(ObtainSemaphoreShared,Exec),AROS_CSYMNAME(_Exec_ObtainSemaphoreShared))
#endif

