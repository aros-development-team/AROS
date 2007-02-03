/*
    Copyright © 1997-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Atomic access functions to be used by macros in atomic.h.
    Lang: english
*/

#include <proto/exec.h>

void atomic_inc_l(LONG* p)
{
    __asm__ __volatile__ (
    "lincl: lwarx  11,  0, %[p] \n\t" /* load from memory and reserve storage location      */
    "       addi   11, 11,    1 \n\t" 
    "       stwcx. 11,  0, %[p] \n\t" /* check that storage was not changed by other thread */
    "       bne-   lincl        \n\t" /* in the meantime, then store, otherwise try again   */
    :
    : [p] "r"(p)
    : "memory",
      "cc",
      "11");                          /* we use r11, don't let the compiler choose it       */
}

void atomic_dec_l(LONG* p)
{
    __asm__ __volatile__ (
    "ldecl: lwarx  11,  0, %[p] \n\t"
    "       subi   11, 11, 1    \n\t"
    "       stwcx. 11,  0, %[p] \n\t"
    "       bne-   ldecl        \n\t"
    :
    : [p] "r"(p)
    : "memory",
      "cc",
      "11");
}

void atomic_and_l(LONG* p, ULONG mask)
{
    __asm__ __volatile__ (
    "landl: lwarx  11,  0, %[p]    \n\t"
    "       and    11, 11, %[mask] \n\t"
    "       stwcx. 11,  0, %[p]    \n\t"
    "       bne-   landl           \n\t"
    :
    : [mask] "r" (mask),
      [p]    "r" (p)
    : "memory",
      "cc",
      "11");
}

void atomic_or_l(LONG* p, ULONG mask)
{
    __asm__ __volatile__ (
    "lorl: lwarx  11,  0, %[p]    \n\t"
    "      or     11, 11, %[mask] \n\t"
    "      stwcx. 11,  0, %[p]    \n\t"
    "      bne-   lorl            \n\t"
    :
    : [mask] "r" (mask),
      [p]    "r" (p)
    : "memory",
      "cc",
      "11");
}

void atomic_inc_b(BYTE* p)
{
    const IPTR   rem   = ((IPTR) p) % 4; /* get pointer to 4 byte aligned base   */
    const UBYTE* u     = p - rem;        /* address of byte                      */
    const IPTR   shift = 24 - rem * 8;   /* shift right for operation            */
          IPTR   bmask = 0xff << shift;  /* clear everything except bits of byte */

    __asm__ __volatile__ (
    "lincb: lwarx  11,        0,      %[u]     \n\t" /* get data                    */
    "       and    12,       11,      %[bmask] \n\t" /* clear                       */
    "       srw    12,       12,      %[shift] \n\t" /* shift right                 */
    "       addi   12,       12,      1        \n\t" /* operation                   */
    "       slw    12,       12,      %[shift] \n\t" /* shift left                  */
    "       not    %[bmask], %[bmask]          \n\t" /* invert mask                 */
    "       and    11,       11,      %[bmask] \n\t" /* clear byte in original data */
    "       or     11,       11,      12       \n\t" /* insert modified byte        */
    "       stwcx. 11,        0,      %[u]     \n\t"
    "       bne-   lincb                       \n\t"
    : [bmask] "+r"(bmask)
    : [u]     "r" (u),
      [shift] "r" (shift)
    : "memory",
      "cc",
      "11", "12");
}

void atomic_dec_b(BYTE* p)
{
    const IPTR   rem   = ((IPTR) p) % 4;
    const UBYTE* u     = p - rem;
    const IPTR   shift = 24 - rem * 8;
          IPTR   bmask = 0xff << shift;

    __asm__ __volatile__ (
    "ldecb: lwarx  11,        0,      %[u]     \n\t"
    "       and    12,       11,      %[bmask] \n\t"
    "       srw    12,       12,      %[shift] \n\t"
    "       subi   12,       12,      1        \n\t"
    "       slw    12,       12,      %[shift] \n\t"
    "       not    %[bmask], %[bmask]          \n\t"
    "       and    11,       11,      %[bmask] \n\t"
    "       or     11,       11,      12       \n\t"
    "       stwcx. 11,        0,      %[u]     \n\t"
    "       bne-   ldecb                       \n\t"
    : [bmask] "+r"(bmask)
    : [u]     "r" (u),
      [shift] "r" (shift)
    : "memory",
      "cc",
      "11", "12");
}

void atomic_and_b(BYTE* p, UBYTE mask)
{
    const IPTR   rem   = ((IPTR) p) % 4;
    const UBYTE* u     = p - rem;
    const IPTR   shift = 24 - rem * 8;
          IPTR   bmask = 0xff << shift;

    __asm__ __volatile__ (
    "landb: lwarx  11,        0,      %[u]     \n\t"
    "       and    12,       11,      %4       \n\t"
    "       srw    12,       12,      %[shift] \n\t"
    "       and    12,       12,      %[mask]  \n\t"
    "       slw    12,       12,      %[shift] \n\t"
    "       not    %[bmask], %[bmask]          \n\t"
    "       and    11,       11,      %[bmask] \n\t"
    "       or     11,       11,      12       \n\t"
    "       stwcx. 11,        0,      %[u]     \n\t"
    "       bne-   landb                       \n\t"
    : [bmask] "+r"(bmask)
    : [mask]  "r" (mask),
      [u]     "r" (u),
      [shift] "r" (shift)
    : "memory",
      "cc",
      "11", "12");
}

void atomic_or_b(BYTE* p, UBYTE mask)
{
    const IPTR   rem   = ((IPTR) p) % 4;
    const UBYTE* u     = p - rem;
    const IPTR   shift = 24 - rem * 8;
          IPTR   bmask = 0xff << shift;
	
    __asm__ __volatile__ (
    "lorb: lwarx  11,        0,      %[u]     \n\t"
    "      and    12,       11,      %[bmask] \n\t"
    "      srw    12,       12,      %[shift] \n\t"
    "      or     12,       12,      %[mask]  \n\t"
    "      slw    12,       12,      %[shift] \n\t"
    "      not    %[bmask], %[bmask]          \n\t"
    "      and    11,       11,      %[bmask] \n\t"
    "      or     11,       11,      12       \n\n"
    "      stwcx. 11,        0,      %[u]     \n\t"
    "      bne-   lorb                        \n\t"
    : [bmask] "+r"(bmask)
    : [mask]  "r" (mask),
      [u]     "r" (u),
      [shift] "r" (shift)
    : "memory",
      "cc",
      "11", "12");
}

void atomic_inc_w(WORD* p)
{
    const IPTR   rem   = (((IPTR) p) % 4) / 2; /* size of UWORD / 2 = size of UBYTE */
    const UWORD* u     = p - rem;
    const IPTR   shift = 16 - rem * 16;
          IPTR   wmask = 0xffff << shift;

    __asm__ __volatile__ (
    "lincw: lwarx  11,        0,      %[u]     \n\t"
    "       and    12,       11,      %[wmask] \n\t"
    "       srw    12,       12,      %[shift] \n\t"
    "       addi   12,       12,      1        \n\t"
    "       slw    12,       12,      %[shift] \n\t"
    "       not    %[wmask], %[wmask]          \n\t"
    "       and    11,       11,      %[wmask] \n\t"
    "       or     11,       11,      12       \n\n"
    "       stwcx. 11,        0,      %[u]     \n\t"
    "       bne-   lincw                       \n\t"
    : [wmask] "+r"(wmask)
    : [u]     "r" (u),
      [shift] "r" (shift)
    : "memory",
      "cc",
      "11", "12");
}

void atomic_dec_w(WORD* p)
{
    const IPTR   rem   = (((IPTR) p) % 4) / 2;
    const UWORD* u     = p - rem;
    const IPTR   shift = 16 - rem * 16;
          IPTR   wmask = 0xffff << shift;

    __asm__ __volatile__ (
    "ldecw: lwarx  11,        0,      %[u]     \n\t"
    "       and    12,       11,      %[wmask] \n\t"
    "       srw    12,       12,      %[shift] \n\t"
    "       subi   12,       12,      1        \n\t"
    "       slw    12,       12,      %[shift] \n\t"
    "       not    %[wmask], %[wmask]          \n\t"
    "       and    11,       11,      %[wmask] \n\t"
    "       or     11,       11,      12       \n\t"
    "       stwcx. 11,        0,      %[u]     \n\t"
    "       bne-   ldecw                       \n\t"
    : [wmask] "+r"(wmask)
    : [u]     "r" (u),
      [shift] "r" (shift)
    : "memory",
      "cc",
      "11", "12");
}

void atomic_and_w(WORD* p, UWORD mask)
{
    const IPTR   rem   = (((IPTR) p) % 4) / 2;
    const UWORD* u     = p - rem;
    const IPTR   shift = 16 - rem * 16;
          IPTR   wmask = 0xffff << shift;

    __asm__ __volatile__ (
    "landw: lwarx  11,        0,      %[u]     \n\t"
    "       and    12,       11,      %[wmask] \n\t"
    "       srw    12,       12,      %[shift] \n\t"
    "       and    12,       12,      %[mask]  \n\t"
    "       slw    12,       12,      %[shift] \n\t"
    "       not    %[wmask], %[wmask]          \n\t"
    "       and    11,       11,      %[wmask] \n\t"
    "       or     11,       11,      12       \n\t"
    "       stwcx. 11,        0,      %[u]     \n\t"
    "       bne-   landw                       \n\t"
    : [wmask] "+r"(wmask)
    : [mask]  "r" (mask),
      [u]     "r" (u),
      [shift] "r" (shift)
    : "memory",
      "cc",
      "11", "12");
}

void atomic_or_w(WORD* p, UWORD mask)
{
    const IPTR   rem   = (((IPTR) p) % 4) / 2;
    const UWORD* u     = p - rem;
    const IPTR   shift = 16 - rem * 16;
          IPTR   wmask = 0xffff << shift;

    __asm__ __volatile__ (
    "lorw: lwarx  11,        0,      %[u]     \n\t"
    "      and    12,       11,      %[wmask] \n\t"
    "      srw    12,       12,      %[shift] \n\t"
    "      or     12,       12,      %[mask]  \n\t"
    "      slw    12,       12,      %[shift] \n\t"
    "      not    %[wmask], %[wmask]          \n\t"
    "      and    11,       11,      %[wmask] \n\t"
    "      or     11,       11,      12       \n\n"
    "      stwcx. 11,        0,      %[u]     \n\t"
    "      bne-   lorw                        \n\t"
    : [wmask] "+r"(wmask)
    : [mask]  "r" (mask),
      [u]     "r" (u),
      [shift] "r" (shift)
    : "memory",
      "cc",
      "11", "12");
}
