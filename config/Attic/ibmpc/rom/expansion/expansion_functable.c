/*
        (C) 1995-96 AROS - The Amiga Replacement OS
        *** Automatic generated file. Do not edit ***
        Desc: Function table for Expansion
        Lang: english
*/
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif

void AROS_SLIB_ENTRY(open,Expansion) (void);
void AROS_SLIB_ENTRY(close,Expansion) (void);
void AROS_SLIB_ENTRY(expunge,Expansion) (void);
void AROS_SLIB_ENTRY(null,Expansion) (void);
void AROS_SLIB_ENTRY(AddConfigDev,Expansion) (void);
void AROS_SLIB_ENTRY(AddBootNode,Expansion) (void);
void AROS_SLIB_ENTRY(AllocBoardMem,Expansion) (void);
void AROS_SLIB_ENTRY(AllocConfigDev,Expansion) (void);
void AROS_SLIB_ENTRY(AllocExpansionMem,Expansion) (void);
void AROS_SLIB_ENTRY(ConfigBoard,Expansion) (void);
void AROS_SLIB_ENTRY(ConfigChain,Expansion) (void);
void AROS_SLIB_ENTRY(FindConfigDev,Expansion) (void);
void AROS_SLIB_ENTRY(FreeBoardMem,Expansion) (void);
void AROS_SLIB_ENTRY(FreeConfigDev,Expansion) (void);
void AROS_SLIB_ENTRY(FreeExpansionMem,Expansion) (void);
void AROS_SLIB_ENTRY(ReadExpansionByte,Expansion) (void);
void AROS_SLIB_ENTRY(ReadExpansionRom,Expansion) (void);
void AROS_SLIB_ENTRY(RemConfigDev,Expansion) (void);
void AROS_SLIB_ENTRY(WriteExpansionByte,Expansion) (void);
void AROS_SLIB_ENTRY(ObtainConfigBinding,Expansion) (void);
void AROS_SLIB_ENTRY(ReleaseConfigBinding,Expansion) (void);
void AROS_SLIB_ENTRY(SetCurrentBinding,Expansion) (void);
void AROS_SLIB_ENTRY(GetCurrentBinding,Expansion) (void);
void AROS_SLIB_ENTRY(MakeDosNode,Expansion) (void);
void AROS_SLIB_ENTRY(AddDosNode,Expansion) (void);

void *const Expansion_functable[]=
{
    AROS_SLIB_ENTRY(open,Expansion), /* 1 */
    AROS_SLIB_ENTRY(close,Expansion), /* 2 */
    AROS_SLIB_ENTRY(expunge,Expansion), /* 3 */
    AROS_SLIB_ENTRY(null,Expansion), /* 4 */
    AROS_SLIB_ENTRY(AddConfigDev,Expansion), /* 5 */
    AROS_SLIB_ENTRY(AddBootNode,Expansion), /* 6 */
    AROS_SLIB_ENTRY(AllocBoardMem,Expansion), /* 7 */
    AROS_SLIB_ENTRY(AllocConfigDev,Expansion), /* 8 */
    AROS_SLIB_ENTRY(AllocExpansionMem,Expansion), /* 9 */
    AROS_SLIB_ENTRY(ConfigBoard,Expansion), /* 10 */
    AROS_SLIB_ENTRY(ConfigChain,Expansion), /* 11 */
    AROS_SLIB_ENTRY(FindConfigDev,Expansion), /* 12 */
    AROS_SLIB_ENTRY(FreeBoardMem,Expansion), /* 13 */
    AROS_SLIB_ENTRY(FreeConfigDev,Expansion), /* 14 */
    AROS_SLIB_ENTRY(FreeExpansionMem,Expansion), /* 15 */
    AROS_SLIB_ENTRY(ReadExpansionByte,Expansion), /* 16 */
    AROS_SLIB_ENTRY(ReadExpansionRom,Expansion), /* 17 */
    AROS_SLIB_ENTRY(RemConfigDev,Expansion), /* 18 */
    AROS_SLIB_ENTRY(WriteExpansionByte,Expansion), /* 19 */
    AROS_SLIB_ENTRY(ObtainConfigBinding,Expansion), /* 20 */
    AROS_SLIB_ENTRY(ReleaseConfigBinding,Expansion), /* 21 */
    AROS_SLIB_ENTRY(SetCurrentBinding,Expansion), /* 22 */
    AROS_SLIB_ENTRY(GetCurrentBinding,Expansion), /* 23 */
    AROS_SLIB_ENTRY(MakeDosNode,Expansion), /* 24 */
    AROS_SLIB_ENTRY(AddDosNode,Expansion), /* 25 */
    (void *)-1L
};
