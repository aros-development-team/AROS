/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:41:42  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#ifndef NULL
#define NULL ((void *)0)
#endif

void Utility_open();
void Utility_close();
void Utility_expunge();
void Utility_null();
void Utility_FindTagItem();
void Utility_Stricmp();
void Utility_Strnicmp();
void Utility_ToLower();
void Utility_ApplyTagChanges();

void *const Utility_functable[]=
{
    Utility_open, /* 1 */
    Utility_close,
    Utility_expunge,
    Utility_null,
    Utility_FindTagItem,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, /* 10 */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, /* 20 */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    Utility_Stricmp,
    Utility_Strnicmp,
    NULL,
    Utility_ToLower, /* 30 */
    Utility_ApplyTagChanges,
    (void *)-1
};

const char Utility_end=0;
