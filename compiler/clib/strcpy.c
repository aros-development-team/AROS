/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:40:46  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
char * strcpy (char * dest, const char * src)
{
    char * ptr = dest;

    while ((*ptr++ = *src ++));

    return dest;
}

