/*
    Copyright © 1997-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Atomic access functions to be used by macros in atomic.h.
    Lang: english
*/

#include <proto/exec.h>

void atomic_inc_l(LONG *p)
{
    LONG l;
	
    __asm__ __volatile__ (
    "lincl: lwarx  %1,%2,%0 \n\t"
    "       addi   %1,%1,1  \n\t"
    "       stwcx. %1,%2,%0 \n\t"
    "       bne-   lincl    \n\t"
    :
    : "r"(p),
      "r"(l),
      "I"(0)
    : "memory", "cc");
}

void atomic_dec_l(LONG *p)
{
    LONG l;
	
    __asm__ __volatile__ (
    "ldecl: lwarx  %1,%2,%0 \n\t"
    "       subi   %1,%1,1  \n\t"
    "       stwcx. %1,%2,%0 \n\t"
    "       bne-   ldecl    \n\t"
    :
    : "r"(p),
      "r"(l),
      "I"(0)
    : "memory", "cc");
}

void atomic_and_l(LONG *p, LONG mask)
{
    LONG l;

    __asm__ __volatile__ (
    "landl: lwarx  %2,%3,%1 \n\t"
    "       and    %2,%2,%0 \n\t"
    "       stwcx. %2,%3,%1 \n\t"
    "       bne-   landl    \n\t"
    :
    : "r"(mask),
      "r"(p),
      "r"(l),
      "I"(0)
    : "memory", "cc");
}

void atomic_or_l(LONG *p, LONG mask)
{
    LONG l;
	
    __asm__ __volatile__ (
    "lorl: lwarx  %2,%3,%1 \n\t"
    "      or     %2,%2,%0 \n\t"
    "      stwcx. %2,%3,%1 \n\t"
    "      bne-   lorl    \n\t"
    :
    : "r"(mask),
      "r"(p),
      "r"(l),
      "I"(0)
    : "memory", "cc");
}

void atomic_inc_b(BYTE *p)
{
    const IPTR rem      = ((IPTR) p) % 4;
    const BYTE *u       = (BYTE *) p - rem;
    const LONG shift    = 24 - rem * 8;
    const LONG bmask    = 0xff << shift;
    const LONG notbmask = ~bmask;
	  BYTE b;
	  LONG tmp;

    __asm__ __volatile__ (
    "lincb: lwarx  %1,%2,%0 \n\t"
    "       and    %5,%1,%3 \n\t"
    "       srw    %5,%5,%4 \n\t"
    "       addi   %5,%5,1 \n\t"
    "       slw    %5,%5,%4 \n\t"
    "       and    %1,%1,%6 \n\t"
    "       or     %1,%1,%5 \n\n"
    "       stwcx. %1,%2,%0 \n\t"
    "       bne-   lincb    \n\t"
    :
    : "r"(u),
      "r"(b),
      "I"(0),
      "r"(bmask),
      "r"(shift),
      "r"(tmp),
      "r"(notbmask)
    : "memory", "cc");
}

void atomic_dec_b(BYTE *p)
{
    const IPTR rem      = ((IPTR) p) % 4;
    const BYTE *u       = (BYTE *) p - rem;
    const LONG shift    = 24 - rem * 8;
    const LONG bmask    = 0xff << shift;
    const LONG notbmask = ~bmask;
	  BYTE b;
	  LONG tmp;

    __asm__ __volatile__ (
    "ldecb: lwarx  %1,%2,%0 \n\t"
    "       and    %5,%1,%3 \n\t"
    "       srw    %5,%5,%4 \n\t"
    "       subi   %5,%5,1 \n\t"
    "       slw    %5,%5,%4 \n\t"
    "       and    %1,%1,%6 \n\t"
    "       or     %1,%1,%5 \n\n"
    "       stwcx. %1,%2,%0 \n\t"
    "       bne-   ldecb    \n\t"
    :
    : "r"(u),
      "r"(b),
      "I"(0),
      "r"(bmask),
      "r"(shift),
      "r"(tmp),
      "r"(notbmask)
    : "memory", "cc");
}

void atomic_and_b(BYTE *p, BYTE mask)
{
    const IPTR rem      = ((IPTR) p) % 4;
    const BYTE *u       = (BYTE *) p - rem;
    const LONG shift    = 24 - rem * 8;
    const LONG bmask    = 0xff << shift;
    const LONG notbmask = ~bmask;
	  BYTE b;
	  LONG tmp;

    __asm__ __volatile__ (
    "landb: lwarx  %2,%3,%1 \n\t"
    "      and    %6,%2,%4 \n\t"
    "      srw    %6,%6,%5 \n\t"
    "      and    %6,%6,%0 \n\t"
    "      slw    %6,%6,%5 \n\t"
    "      and    %2,%2,%7 \n\t"
    "      or     %2,%2,%6 \n\n"
    "      stwcx. %2,%3,%1 \n\t"
    "      bne-   landb    \n\t"
    :
    : "r"(mask),
      "r"(u),
      "r"(b),
      "I"(0),
      "r"(bmask),
      "r"(shift),
      "r"(tmp),
      "r"(notbmask)
    : "memory", "cc");
}

void atomic_or_b(BYTE *p, BYTE mask)
{
    const IPTR rem      = ((IPTR) p) % 4;
    const BYTE *u       = (BYTE *) p - rem;
    const LONG shift    = 24 - rem * 8;
    const LONG bmask    = 0xff << shift;
    const LONG notbmask = ~bmask;
	  BYTE b;
	  LONG tmp;
    __asm__ __volatile__ (
    "lorb: lwarx  %2,%3,%1 \n\t"
    "      and    %6,%2,%4 \n\t"
    "      srw    %6,%6,%5 \n\t"
    "      or     %6,%6,%0 \n\t"
    "      slw    %6,%6,%5 \n\t"
    "      and    %2,%2,%7 \n\t"
    "      or     %2,%2,%6 \n\n"
    "      stwcx. %2,%3,%1 \n\t"
    "      bne-   lorb     \n\t"
    :
    : "r"(mask),
      "r"(u),
      "r"(b),
      "I"(0),
      "r"(bmask),
      "r"(shift),
      "r"(tmp),
      "r"(notbmask)
    : "memory", "cc");
}

void atomic_inc_w(WORD *p)
{
    const IPTR rem      = ((IPTR) p) % 2;
    const WORD *u       = (WORD *) p - rem;
    const LONG shift    = rem * 16;
    const LONG wmask    = 0xffff << shift;
    const LONG notwmask = ~wmask;
	  WORD w;
	  LONG tmp;

    __asm__ __volatile__ (
    "lincw: lwarx  %1,%2,%0 \n\t"
    "       and    %5,%1,%3 \n\t"
    "       srw    %5,%5,%6 \n\t"
    "       addi   %5,%5,1  \n\t"
    "       slw    %5,%5,%4 \n\t"
    "       and    %1,%1,%6 \n\t"
    "       or     %1,%1,%5 \n\n"
    "       stwcx. %1,%2,%0 \n\t"
    "       bne-   lincw    \n\t"
    :
    : "r"(u),
      "r"(w),
      "I"(0),
      "r"(wmask),
      "r"(shift),
      "r"(tmp),
      "r"(notwmask)
    : "memory", "cc");
}

void atomic_dec_w(WORD *p)
{
    const IPTR rem      = ((IPTR) p) % 2;
    const WORD *u       = (WORD *) p - rem;
    const LONG shift    = rem * 16;
    const LONG wmask    = 0xffff << shift;
    const LONG notwmask = ~wmask;
	  WORD w;
	  LONG tmp;

    __asm__ __volatile__ (
    "ldecw: lwarx  %1,%2,%0 \n\t"
    "       and    %5,%1,%3 \n\t"
    "       srw    %5,%5,%6 \n\t"
    "       subi   %5,%5,1  \n\t"
    "       slw    %5,%5,%4 \n\t"
    "       and    %1,%1,%6 \n\t"
    "       or     %1,%1,%5 \n\n"
    "       stwcx. %1,%2,%0 \n\t"
    "       bne-   ldecw    \n\t"
    :
    : "r"(u),
      "r"(w),
      "I"(0),
      "r"(wmask),
      "r"(shift),
      "r"(tmp),
      "r"(notwmask)
    : "memory", "cc");
}

void atomic_and_w(WORD *p, WORD mask)
{
    const IPTR rem      = (((IPTR) p) % 4) / 2;
    const WORD *u       = (WORD *) p - rem;
    const LONG shift    = 16 - rem * 16;
    const LONG wmask    = 0xffff << shift;
    const LONG notwmask = ~wmask;
	  WORD w;
	  LONG tmp;

    __asm__ __volatile__ (
    "landw: lwarx  %2,%3,%1 \n\t"
    "       and    %6,%2,%4 \n\t"
    "       srw    %6,%6,%5 \n\t"
    "       and    %6,%6,%0 \n\t"
    "       slw    %6,%6,%5 \n\t"
    "       and    %2,%2,%7 \n\t"
    "       or     %2,%2,%6 \n\n"
    "       stwcx. %2,%3,%1 \n\t"
    "       bne-   landw    \n\t"
    :
    : "r"(mask),
      "r"(u),
      "r"(w),
      "I"(0),
      "r"(wmask),
      "r"(shift),
      "r"(tmp),
      "r"(notwmask)
    : "memory", "cc");
}

void atomic_or_w(WORD *p, WORD mask)
{
    const IPTR rem      = ((IPTR) p) % 2;
    const WORD *u       = (WORD *) p - rem;
    const LONG shift    = rem * 16;
    const LONG wmask    = 0xffff << shift;
    const LONG notwmask = ~wmask;
	  WORD w;
	  LONG tmp;

    __asm__ __volatile__ (
    "lorw: lwarx  %2,%3,%1 \n\t"
    "      and    %6,%2,%4 \n\t"
    "      srw    %6,%6,%5 \n\t"
    "      or     %6,%6,%0 \n\t"
    "      slw    %6,%6,%5 \n\t"
    "      and    %2,%2,%7 \n\t"
    "      or     %2,%2,%6 \n\n"
    "      stwcx. %2,%3,%1 \n\t"
    "      bne-   lorw    \n\t"
    :
    : "r"(mask),
      "r"(u),
      "r"(w),
      "I"(0),
      "r"(wmask),
      "r"(shift),
      "r"(tmp),
      "r"(notwmask)
    : "memory", "cc");
}
