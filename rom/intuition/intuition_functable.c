/*
        (C) 1995-96 AROS - The Amiga Replacement OS
        *** Automatic generated file. Do not edit ***
        Desc: Funktion table for Intuition
        Lang: english
*/
#ifndef NULL
#define NULL ((void *)0)
#endif

void Intuition_open (void);
void Intuition_close (void);
void Intuition_expunge (void);
void Intuition_null (void);
void Intuition_CloseWindow (void);
void Intuition_ModifyIDCMP (void);
void Intuition_OpenWindow (void);

void *const Intuition_functable[]=
{
    Intuition_open, /* 1 */
    Intuition_close, /* 2 */
    Intuition_expunge, /* 3 */
    Intuition_null, /* 4 */
    NULL, /* 5 */
    NULL, /* 6 */
    NULL, /* 7 */
    NULL, /* 8 */
    NULL, /* 9 */
    NULL, /* 10 */
    NULL, /* 11 */
    Intuition_CloseWindow, /* 12 */
    NULL, /* 13 */
    NULL, /* 14 */
    NULL, /* 15 */
    NULL, /* 16 */
    NULL, /* 17 */
    NULL, /* 18 */
    NULL, /* 19 */
    NULL, /* 20 */
    NULL, /* 21 */
    NULL, /* 22 */
    NULL, /* 23 */
    NULL, /* 24 */
    Intuition_ModifyIDCMP, /* 25 */
    NULL, /* 26 */
    NULL, /* 27 */
    NULL, /* 28 */
    NULL, /* 29 */
    NULL, /* 30 */
    NULL, /* 31 */
    NULL, /* 32 */
    NULL, /* 33 */
    Intuition_OpenWindow, /* 34 */
    (void *)-1L
};

char Intuition_end;
