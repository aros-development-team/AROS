/*
    Copyright © 1997-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Atomic access functions to be used by macros in atomic.h.
    Lang: english
*/

#include <proto/exec.h>
#include <aros/atomic.h>

BOOL set_atomic(
    IPTR* addr,
    IPTR  old,
    IPTR  new)
{
    BOOL success = FALSE;
    
    __asm__ __volatile__ (
    "loop: lwarx  11,         0, %[addr] \n\t" /* load from memory and reserve storage location      */
    "      cmpw   %[old],    11          \n\t" /* has it been changed by another thread              */
    "      bne-   exit                   \n\t" /* give up, caller can try again on the changed value */
    "      stwcx. %[new],     0, %[addr] \n\t" /* check that storage was not changed by other thread */
    "      bne-   loop                   \n\t" /* in the meantime, then store, otherwise try again   */
    "      li     %[success], 1          \n\t" /* atomic store successful, set success flag                  */
    "exit:                               \n\t"
    : [success] "+r" (success)
    : [addr]    "r"  (addr),
      [old]     "r"  (old),
      [new]     "r"  (new)
    : "memory",
      "cc",
      "11");                                   /* we use r11, don't let the compiler choose it       */
    
    return success;
}

void atomic_inc_l(LONG* p)
{
    BOOL success = FALSE;

    while (!success)
    {
        CONST IPTR old = *p;
        CONST IPTR new = old + 1;
        
        success = set_atomic(p, old, new);
    }
}

void atomic_dec_l(LONG* p)
{
    BOOL success = FALSE;

    while (!success)
    {
        CONST IPTR old = *p;
        CONST IPTR new = old - 1;
        
        success = set_atomic(p, old, new);
    }
}

void atomic_and_l(ULONG* p, ULONG mask)
{
    BOOL success = FALSE;

    while (!success)
    {
        CONST IPTR old = *p;
        CONST IPTR new = old & mask;
        
        success = set_atomic(p, old, new);
    }
}

void atomic_or_l(ULONG* p, ULONG mask)
{
    BOOL success = FALSE;

    while (!success)
    {
        CONST IPTR old = *p;
        CONST IPTR new = old | mask;
        
        success = set_atomic(p, old, new);
    }
}

void atomic_inc_b(BYTE* p)
{
    CONST IPTR  rem     = ((IPTR) p) % 4;    /* get pointer to 4 byte aligned base */
          IPTR* addr    = (IPTR*) (p - rem); /* address of byte                    */
          BOOL  success = FALSE;

    while (!success)
    {
        CONST IPTR  old = *addr;
        CONST BYTE  b   = *p;
              union {
                  IPTR  new;
                  UBYTE a[4];
              } un;
              
        un.new    = old;
        un.a[rem] = b + 1;
        
        success = set_atomic(addr, old, un.new);
    }
}

void atomic_dec_b(BYTE* p)
{
    CONST IPTR  rem     = ((IPTR) p) % 4;
          IPTR* addr    = (IPTR*) (p - rem);
          BOOL  success = FALSE;

    while (!success)
    {
        CONST IPTR  old = *addr;
        CONST BYTE  b   = *p;
              union {
                  IPTR  new;
                  UBYTE a[4];
              } un;
              
        un.new    = old;
        un.a[rem] = b - 1;
        
        success = set_atomic(addr, old, un.new);
    }
}

void atomic_and_b(UBYTE* p, UBYTE mask)
{
    CONST IPTR  rem     = ((IPTR) p) % 4;
          IPTR* addr    = (IPTR*) (p - rem);
          BOOL  success = FALSE;

    while (!success)
    {
        CONST IPTR   old = *addr;
        CONST UBYTE  b   = *p;
              union {
                  IPTR  new;
                  UBYTE a[4];
              } un;
              
        un.new    = old;
        un.a[rem] = b & mask;
        
        success = set_atomic(addr, old, un.new);
    }
}

void atomic_or_b(UBYTE* p, UBYTE mask)
{
    CONST IPTR  rem     = ((IPTR) p) % 4;
          IPTR* addr    = (IPTR*) (p - rem);
          BOOL  success = FALSE;

    while (!success)
    {
        CONST IPTR   old = *addr;
        CONST UBYTE  b   = *p;
              union {
                  IPTR  new;
                  UBYTE a[4];
              } un;
              
        un.new    = old;
        un.a[rem] = b | mask;
        
        success = set_atomic(addr, old, un.new);
    }
}

void atomic_inc_w(WORD* p)
{
    CONST IPTR  rem     = (((IPTR) p) % 4 ) / 2;
          IPTR* addr    = (IPTR*) (p - rem);
          BOOL  success = FALSE;

    while (!success)
    {
        CONST IPTR  old = *addr;
        CONST WORD  w   = *p;
              union {
                  IPTR new;
                  WORD a[2];
              } un;
              
        un.new    = old;
        un.a[rem] = w + 1;
        
        success = set_atomic(addr, old, un.new);
    }
}

void atomic_dec_w(WORD* p)
{
    CONST IPTR  rem     = (((IPTR) p) % 4 ) / 2;
          IPTR* addr    = (IPTR*) (p - rem);
          BOOL  success = FALSE;

    while (!success)
    {
        CONST IPTR  old = *addr;
        CONST WORD  w   = *p;
              union {
                  IPTR new;
                  WORD a[2];
              } un;
              
        un.new    = old;
        un.a[rem] = w - 1;
        
        success = set_atomic(addr, old, un.new);
    }
}

void atomic_and_w(UWORD* p, UWORD mask)
{
    CONST IPTR  rem     = (((IPTR) p) % 4 ) / 2;
          IPTR* addr    = (IPTR*) (p - rem);
          BOOL  success = FALSE;

    while (!success)
    {
        CONST IPTR   old = *addr;
        CONST UWORD  w   = *p;
              union {
                  IPTR new;
                  WORD a[2];
              } un;
              
        un.new    = old;
        un.a[rem] = w & mask;
        
        success = set_atomic(addr, old, un.new);
    }
}

void atomic_or_w(UWORD* p, UWORD mask)
{
    CONST IPTR  rem     = (((IPTR) p) % 4 ) / 2;
          IPTR* addr    = (IPTR*) (p - rem);
          BOOL  success = FALSE;

    while (!success)
    {
        CONST IPTR   old = *addr;
        CONST UWORD  w   = *p;
              union {
                  IPTR new;
                  WORD a[2];
              } un;
        
        un.new    = old;
        un.a[rem] = w | mask;
        
        success = set_atomic(addr, old, un.new);
    }
}
