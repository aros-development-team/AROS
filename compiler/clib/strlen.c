/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:40:46  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
int strlen (const char * ptr)
{
    int len=0;

    while (*ptr++) len++;

    return len;
}

