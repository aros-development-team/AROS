#ifndef AUTOINIT_H
#define AUTOINIT_H

#include <sys/cdefs.h>

__BEGIN_DECLS

/* Show an error message to the user. Depending on whether the program
   has been started from CLI or from Workbench(TM) the message will be
   respectively shown as a message on the cli or in a Intuition's requester.

   To make it always open a requester define a global variable like this:

       int __forceerrorrequester = 1;  */
void __showerror __P((char *title, char *format, ...));

__END_DECLS

#endif /* !AUTOINIT_H */