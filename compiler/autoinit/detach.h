/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DETACH_H
#define DETACH_H

#include <sys/cdefs.h>

__BEGIN_DECLS

/* When you link your program with the startup file detach.o, you might
   want to take control over the detaching procedure, by being able to 
   tell the detacher process exactly when to detach. In this case,
   simply use the Detach() function wherever you want the detaching to
   happen.  If you don't use this function, then the program will be detached
   before the main() function is reached.  */
void Detach __P((void));

__END_DECLS

#endif /* !DETACH_H */
