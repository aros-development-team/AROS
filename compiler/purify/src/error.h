#ifndef _ERROR
#define _ERROR

#define POSINFO_FORMAT	    "%s:%d in %s()"
#define POSINFO_ARG(pi)     (pi)->filename, (pi)->lineno, (pi)->functionname

extern int Purify_Error;

enum PurifyErrorCodes
{
#undef _ERROR_DEF
#define _ERROR_DEF(n,s) n,
#include "error.def"
};

void Purify_PrePrintError (void);
void Purify_PostPrintError (void);
void Purify_PrintError (const char * fmt, ...);
void Purify_PrintAccessError (const char * access, void * addr, int size);

#endif /* _ERROR */
