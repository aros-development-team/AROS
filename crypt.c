/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/16 14:01:40  digulla
    The salt may be specified now

    Revision 1.2  1996/08/01 17:40:38  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <unistd.h>
#include <stdio.h>

int main (int argc, char ** argv)
{
    char salt[3];
    char * set = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";

    srand (time (NULL));

    salt[0] = set[getpid() % sizeof(set)];
    salt[1] = set[rand() % sizeof(set)];
    salt[2] = 0;

    if (argc > 2)
    {
	salt[0] = argv[2][0];
	salt[1] = argv[2][1];
    }

    printf ("Encrypting %s: %s\n", argv[1], crypt (argv[1], salt));
}

