/*
        (C) 1995-96 AROS - The Amiga Replacement OS
        *** Automatic generated file. Do not edit ***
        Desc: Funktion table for Gadtools
        Lang: english
*/
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif

void AROS_SLIB_ENTRY(open,Gadtools) (void);
void AROS_SLIB_ENTRY(close,Gadtools) (void);
void AROS_SLIB_ENTRY(expunge,Gadtools) (void);
void AROS_SLIB_ENTRY(null,Gadtools) (void);
void AROS_SLIB_ENTRY(CreateGadgetA,Gadtools) (void);
void AROS_SLIB_ENTRY(FreeGadgets,Gadtools) (void);
void AROS_SLIB_ENTRY(GT_SetGadgetAttrsA,Gadtools) (void);
void AROS_SLIB_ENTRY(CreateMenusA,Gadtools) (void);
void AROS_SLIB_ENTRY(LayoutMenuItemsA,Gadtools) (void);
void AROS_SLIB_ENTRY(LayoutMenusA,Gadtools) (void);
void AROS_SLIB_ENTRY(GT_GetIMsg,Gadtools) (void);
void AROS_SLIB_ENTRY(GT_ReplyIMsg,Gadtools) (void);
void AROS_SLIB_ENTRY(GT_RefreshWindow,Gadtools) (void);
void AROS_SLIB_ENTRY(GT_BeginRefresh,Gadtools) (void);
void AROS_SLIB_ENTRY(GT_EndRefresh,Gadtools) (void);
void AROS_SLIB_ENTRY(GT_FilterIMsg,Gadtools) (void);
void AROS_SLIB_ENTRY(GT_PostFilterIMsg,Gadtools) (void);
void AROS_SLIB_ENTRY(CreateContext,Gadtools) (void);
void AROS_SLIB_ENTRY(DrawBevelBoxA,Gadtools) (void);
void AROS_SLIB_ENTRY(GetVisualInfoA,Gadtools) (void);
void AROS_SLIB_ENTRY(FreeVisualInfo,Gadtools) (void);
void AROS_SLIB_ENTRY(GT_GetGadgetAttrsA,Gadtools) (void);

void *const Gadtools_functable[]=
{
    AROS_SLIB_ENTRY(open,Gadtools), /* 1 */
    AROS_SLIB_ENTRY(close,Gadtools), /* 2 */
    AROS_SLIB_ENTRY(expunge,Gadtools), /* 3 */
    AROS_SLIB_ENTRY(null,Gadtools), /* 4 */
    AROS_SLIB_ENTRY(CreateGadgetA,Gadtools), /* 5 */
    AROS_SLIB_ENTRY(FreeGadgets,Gadtools), /* 6 */
    AROS_SLIB_ENTRY(GT_SetGadgetAttrsA,Gadtools), /* 7 */
    AROS_SLIB_ENTRY(CreateMenusA,Gadtools), /* 8 */
    NULL, /* 9 */
    AROS_SLIB_ENTRY(LayoutMenuItemsA,Gadtools), /* 10 */
    AROS_SLIB_ENTRY(LayoutMenusA,Gadtools), /* 11 */
    AROS_SLIB_ENTRY(GT_GetIMsg,Gadtools), /* 12 */
    AROS_SLIB_ENTRY(GT_ReplyIMsg,Gadtools), /* 13 */
    AROS_SLIB_ENTRY(GT_RefreshWindow,Gadtools), /* 14 */
    AROS_SLIB_ENTRY(GT_BeginRefresh,Gadtools), /* 15 */
    AROS_SLIB_ENTRY(GT_EndRefresh,Gadtools), /* 16 */
    AROS_SLIB_ENTRY(GT_FilterIMsg,Gadtools), /* 17 */
    AROS_SLIB_ENTRY(GT_PostFilterIMsg,Gadtools), /* 18 */
    AROS_SLIB_ENTRY(CreateContext,Gadtools), /* 19 */
    AROS_SLIB_ENTRY(DrawBevelBoxA,Gadtools), /* 20 */
    AROS_SLIB_ENTRY(GetVisualInfoA,Gadtools), /* 21 */
    AROS_SLIB_ENTRY(FreeVisualInfo,Gadtools), /* 22 */
    NULL, /* 23 */
    NULL, /* 24 */
    NULL, /* 25 */
    NULL, /* 26 */
    NULL, /* 27 */
    NULL, /* 28 */
    AROS_SLIB_ENTRY(GT_GetGadgetAttrsA,Gadtools), /* 29 */
    (void *)-1L
};

char Gadtools_end;
