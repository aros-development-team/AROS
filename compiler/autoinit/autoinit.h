/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: header exporting support functions from libautoinit.a
*/

#ifndef AUTOINIT_H
#define AUTOINIT_H

#include <sys/cdefs.h>

__BEGIN_DECLS

/* Shows an error message to the user. Depending on whether the program
   has been started from CLI or from Workbench(TM) the message will be
   respectively shown as a message on the cli or as an Intuition requester.

   To make it always open a requester define a global variable like this:

       int __forceerrorrequester = 1;  */
void __showerror __P((char *format, ...)) __stackparm;

__END_DECLS

#endif /* !AUTOINIT_H */
