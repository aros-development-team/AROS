#ifndef ___OPEN_H
#define ___OPEN_H

/*
    Copyright 2001 AROS - The Amiga Research OS
    $Id$

    Desc: file descriptors handling internals - header file
    Lang: english
*/

typedef struct
{
    void *fh;
    int  flags;
    unsigned int opencount;
} fdesc;

extern void *__stdfiles[3];

fdesc *__getfdesc(register int fd);
void __setfdesc(register int fd, fdesc *fdesc);
int __getfdslot(int wanted_fd);
int __getfirstfd(register int startfd);

#endif