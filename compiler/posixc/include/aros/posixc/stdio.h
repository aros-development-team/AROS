#ifndef _POSIXC_STDIO_H_
#define _POSIXC_STDIO_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 header file stdio.h
*/

/* C99 */
#include <aros/stdc/stdio.h>

#include <aros/types/off_t.h>
#include <aros/types/ssize_t.h>

#define L_ctermid	FILENAME_MAX	/* Max filename for controlling tty */

#define P_tmpdir	"T:"		/* Default temporary path */

__BEGIN_DECLS

/* NOTIMPL char	*ctermid(char *); */
/* NOTIMPL int dprintf(int, const char *restrict, ...) */
FILE *fdopen (int filedes, const char *mode);
int fileno(FILE *);
/* NOTIMPL void flockfile(FILE *); */
/* NOTIMPL FILE *fmemopen(void *restrict, size_t, const char *restrict) */
int fseeko(FILE *stream, off_t offset, int whence);
off_t ftello(FILE *stream);
/* NOTIMPL int ftrylockfile(FILE *); */
/* NOTIMPL void funlockfile(FILE *); */
/* NOTIMPL int getc_unlocked(FILE *); */
/* NOTIMPL int getchar_unlocked(void); */
/* NOTIMPL ssize_t getdelim(char **restrict, size_t *restrict, int, FILE *restrict); */
/* NOTIMPL ssize_t getline(char **restrict, size_t *restrict, FILE *restrict); */
/* NOTIMPL FILE *open_memstream(char **, size_t *); */
int pclose(FILE *);
FILE *popen(const char *, const char *);
/* NOTIMPL int      putc_unlocked(int, FILE *); */
/* NOTIMPL int      putchar_unlocked(int); */
/* NOTIMPL int      renameat(int, const char *, int, const char *); */
char *tempnam(const char *dir, const char *pfx);
/* NOTIMPL int      vdprintf(int, const char *restrict, va_list); */

/* Following function is here for clib2 compatibility and abc-shell
   Don't use in new code
*/
int __get_default_file (int file_descriptor, long * file_handle);

/* Implement deprecated POSIX.1-2001 functions as static inline functions. */
static __inline__ int getw(FILE *stream)
{
    int word;
    
    if (fread(&word, sizeof(word), 1, stream) > 0) return word;
    else                                           return EOF;
}

static __inline__ int putw(int word, FILE *stream)
{
    if (fwrite(&word, sizeof(word), 1, stream) > 0) return 0;
    else                                            return EOF;
}

__END_DECLS

#endif /* _POSIXC_STDIO_H_ */
