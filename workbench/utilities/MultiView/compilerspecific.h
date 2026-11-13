/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.
*/

#ifndef COMPILERSPECIFIC_H
#define COMPILERSPECIFIC_H

#undef SAVEDS
#undef ASM
#undef STDARGS

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifdef __AROS__

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif

#define SAVEDS
#define ASM
#define STDARGS

#define getreg(x) 0
#define putreg(a,b) 

#else

typedef unsigned long IPTR;

#define SAVEDS  __saveds
#define ASM     __asm
#define STDARGS __stdargs

#if !defined(_DOS_H) && defined(__SASC)
#include <dos.h>
#define BNULL NULL
#define PMODE_V42 (0)
#define PMODE_V43 (1)
#define SNA_Notify (TAG_USER+0x02)
#define SNOTIFY_WAIT_REPLY (1<<15)
#define SNOTIFY_BEFORE_CLOSEWB (1<<3)
#define SNOTIFY_AFTER_OPENWB (1<<2)
#define SNA_MsgPort (TAG_USER+0x06)
#define SNA_Priority (TAG_USER+0x07)
struct ScreenNotifyMessage
{
	struct Message snm_Message;
	ULONG          snm_Class;
};
#define EndScreenNotify(a) TRUE
#define StartScreenNotifyTags(a,b,c,d,e,f,g) 0
#define __sprintf sprintf
#endif

#endif

#endif /* COMPILERSPECIFIC_H */
