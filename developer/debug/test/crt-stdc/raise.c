/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Test program for the arosc's raise() and signal() functions.
*/
#include <signal.h>
#include <stdio.h>

void sigill(int sig)
{
    (void)sig;

    puts("Catched illegal instruction signal");
}

void sigsegv(int sig)
{
    static int call = 0;

    (void)sig;

    call++;
    printf("sigsegv call %d\n", call);

    if (call == 1)
        raise(SIGSEGV);
}

int main(void)
{
    puts("Setting signals");
    signal(SIGFPE, SIG_IGN);
    signal(SIGILL, sigill);
    signal(SIGSEGV, sigsegv);

    puts("\nRaising SIGFPE (ignored)");
    raise(SIGFPE);

    puts("\nRaising SIGILL (catched)");
    raise(SIGILL);

    puts("\nRaising SIGSEGV (nested 2 times)");
    raise(SIGSEGV);

    puts("\nRaising SIGABRT (not catched)");
    raise(SIGABRT);

    return 0;
}
