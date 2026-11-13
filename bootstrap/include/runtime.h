/* Prototypes of runtime functions needed by this library */

#ifndef BOOTSTRAP_RUNTIME_H_
#define BOOTSTRAP_RUNTIME_H_

/* Print information message to the debug console */
void kprintf(const char *, ...);

/* Display critical error. On hosted flavours this can be host OS window */
void DisplayError(char *fmt, ...);

#endif
