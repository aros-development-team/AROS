#ifndef MMAKE_H
#define MMAKE_H

extern int verbose;
extern int quiet;
extern int debug;
extern int logfailed;
extern char ** mflags;
extern int mflagc;
extern char *mm_srcdir;
extern char *mm_builddir;
extern char *mm_envtarget;
extern FILE *mm_faillogfh;

void error(char * fmt, ...);

#endif
