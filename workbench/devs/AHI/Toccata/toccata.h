/*
     toccata.library - AHI-based Toccata emulation library
     Copyright (C) 1997-2005 Martin Blom <martin@blom.org> and Teemu Suikki.
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/


#ifndef TOCCATAEMUL_H
#define TOCCATAEMUL_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <libraries/toccata.h>

/* Force 32 bit results */

#define BOOL LONG


/* Arguments in registers */

#define ASM     __asm
#define REG(x)  register __ ## x


/* Definitions */

#define PLAYERFREQ 50

/* Structures */

struct toccataprefs {
  UBYTE ID[8];

  LONG  MixAux1Left;
  LONG  MixAux1Right;
  LONG  MixAux2Left;
  LONG  MixAux2Right;
  LONG  InputVolumeLeft;
  LONG  InputVolumeRight;
  LONG  OutputVolumeLeft;
  LONG  OutputVolumeRight;
  LONG  LoopbackVolume;
  ULONG Mode;
  ULONG Frequency;
  ULONG Input;
  ULONG MicGain;
  LONG  CaptureIoPri;
  LONG  CaptureBufferPri;
  ULONG CaptureBlockSize;
  ULONG MaxCaptureBlocks;
  LONG  PlaybackIoPri;
  LONG  PlaybackBufferPri;
  ULONG PlaybackBlockSize;
  ULONG PlaybackStartBlocks;
  ULONG PlaybackBlocks;

  ULONG MonoMode;
  ULONG StereoMode;
  ULONG LineInput;
  ULONG Aux1Input;
  ULONG MicInput;
  ULONG MicGainInput;
  ULONG MixInput;
};


struct slavemessage {
	struct Message Msg;
  ULONG          ID;   
  APTR           Data;
};


/* ID codes */

#define MSG_MODE      1
#define MSG_HWPROP    2
#define MSG_RAWPLAY   3
#define MSG_PLAY      4
#define MSG_RECORD    5
#define MSG_STOP      6
#define MSG_PAUSE     7
#define MSG_LEVELON   8
#define MSG_LEVELOFF  9


/* Externals */

extern char __far _LibID[];
extern char __far _LibName[];

extern struct ToccataBase *ToccataBase;
extern struct Process *SlaveProcess;
extern BOOL SlaveInitialized;
extern struct AHIAudioCtrl *audioctrl;
extern struct toccataprefs tprefs;
extern const Fixed negboundaries[];
extern const Fixed posboundaries[];

extern ULONG error;

void kprintf(char *, ...);

ASM void puta4(void);

ASM void SlaveTaskEntry(void);
ASM void IOTaskEntry(void);
ASM void HookLoad(void);
ASM ULONG GetRawReply(REG(a0) struct ToccataBase *);
ASM RawReply(void);

void fillhardinfo(void);

#endif /* TOCCATAEMUL_H */
