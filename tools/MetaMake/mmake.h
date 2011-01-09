#ifndef MMAKE_H
#define MMAKE_H

extern int verbose;
extern int quiet;
extern int debug;
extern char ** mflags;
extern int mflagc;
extern char *mm_srcdir;
extern char *mm_builddir;

void error(char * fmt, ...);

#endif
