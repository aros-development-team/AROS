/*
        (C) 1995-96 AROS - The Amiga Replacement OS
        *** Automatic generated file. Do not edit ***
        Desc: Function table for Mathieeesingtrans
        Lang: english
*/
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif

void AROS_SLIB_ENTRY(open,Mathieeesingtrans) (void);
void AROS_SLIB_ENTRY(close,Mathieeesingtrans) (void);
void AROS_SLIB_ENTRY(expunge,Mathieeesingtrans) (void);
void AROS_SLIB_ENTRY(null,Mathieeesingtrans) (void);
void AROS_SLIB_ENTRY(SPSqrt,Mathieeesingtrans) (void);

void *const Mathieeesingtrans_functable[]=
{
    AROS_SLIB_ENTRY(open,Mathieeesingtrans), /* 1 */
    AROS_SLIB_ENTRY(close,Mathieeesingtrans), /* 2 */
    AROS_SLIB_ENTRY(expunge,Mathieeesingtrans), /* 3 */
    AROS_SLIB_ENTRY(null,Mathieeesingtrans), /* 4 */
    NULL, /* 5 */
    NULL, /* 6 */
    NULL, /* 7 */
    NULL, /* 8 */
    AROS_SLIB_ENTRY(SPSqrt,Mathieeesingtrans), /* 9 */
    (void *)-1L
};
