#ifndef __SIGNAL_H
#define __SIGNAL_H 1

void (*signal(int,void (*)(int)))(int);
int raise(int);

#define SIGABRT 1
#define SIGFPE  2
#define SIGILL  3
#define SIGINT  4
#define SIGSEGV 5
#define SIGTERM 6

#define SIG_ERR (void (*)(int))0
#define SIG_DFL (void (*)(int))1
#define SIG_IGN (void (*)(int))2

#endif

