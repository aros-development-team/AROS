/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    
    $Id$
*/

#ifndef _SIGNALHACK_H
#define _SIGNALHACK_H

#include <signal.h>

int sigaction( int signum, const struct sigaction *act, struct sigaction *oldact );
int sigaddset( sigset_t *set, int signum );
int sigdelset( sigset_t *set, int signum );
int sigemptyset( sigset_t *set );
int sigfillset( sigset_t *set );
int sigismember( const sigset_t *set, int signum );
int sigpending( sigset_t *set );
int sigprocmask( int how, const sigset_t *set, sigset_t *oldset );
int sigsuspend( const sigset_t *mask );

#endif /* _SIGNALHACK_H */
