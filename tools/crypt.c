/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Utility for crypt(3).
    Lang: english
*/

/*
    This is the sourcecode for crypt. It is a small program which makes
    it more convenient to create Unix passwords with crypt(3).

    To compile:

	cc crypt.c -o crypt

    If you get an error during link which says that "crypt" is an
    unknown symbol, try this:

	cc crypt.c -o crypt -lcrypt

    Then run this with your password as the first argument. If you
    want to test if it really works, try it like this:

	crypt test xx

    which must print:

	Encrypting test: xx1LtbDbOY4/E

    If it prints something else, then your version of crypt(3) is not
    compatible.
*/

#define _XOPEN_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main (int argc, char ** argv)
{
    char salt[3];
    char set[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";


    if (argc < 2 || argc > 3)
    {
	printf("Usage: %s <password> [<salt>]\n", argv[0]);
	return 1;
    }

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

    return 0;
}

