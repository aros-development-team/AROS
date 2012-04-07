#ifndef _STDIO_H_
#define _STDIO_H_

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: C99 & POSIX.1-2008 header file stdio.h
*/
#include <aros/system.h>


/* C99 */
/*
    FIXME: We are supposed to declare it, without including the file.
    This is too compiler specific to handle at the moment.
*/
#include <stdarg.h>

#include <sys/arosc.h>

#include <aros/types/size_t.h>

struct __sFILE;
typedef struct __sFILE FILE;

#include <aros/types/fpos_t.h>

#include <aros/types/null.h>
/* Buffering methods that can be specified with setvbuf() */
#define _IOFBF 0 /* Fully buffered. */
#define _IOLBF 1 /* Line buffered. */
#define _IONBF 2 /* Not buffered. */

/* Need to protect against standard Amiga includes */
#ifndef BUFSIZ
#   define BUFSIZ	1024
#endif
#ifndef EOF
#   define EOF (-1)
#endif
#define FOPEN_MAX	16		/* Must be > 8 */
#define FILENAME_MAX	256		/* Amiga files are 256 */
#define L_tmpnam	FILENAME_MAX	/* Max temporary filename */

#include <aros/types/seek.h>            /* SEEK_SET, SEEK_CUR and SEEK_END */

#define TMP_MAX		10240		/* Must be > 10000 */

#define stderr (__get_arosc_userdata()->acud_stderr)
#define stdin  (__get_arosc_userdata()->acud_stdin)
#define stdout (__get_arosc_userdata()->acud_stdout)

__BEGIN_DECLS

/* Internal functions */
int __vcformat (void * data, int (*outc)(int, void *),
		const char * format, va_list args);
int __vcscan (void * data, int (*getc)(void *),
		int (*ungetc)(int, void *),
		const char * format, va_list args);

/* Operations on files */
int remove(const char *filename);
int rename(const char *from, const char *to);
FILE *tmpfile(void);
char *tmpnam(char *s);

/* File access functions */
int fclose(FILE *stream);
int fflush(FILE *stream);
FILE *fopen(const char * restrict filename, const char * restrict mode);
FILE *freopen(const char * restrict filename, const char * restrict mode,
	FILE * restrict stream);
void setbuf(FILE * restrict stream, char * restrict buf);
int setvbuf(FILE * restrict stream, char * restrict buf, int mode,
	size_t size);

/* Formatted input/output functions */
int fprintf(FILE * restrict stream, const char * restrict format, ...);
int fscanf(FILE * restrict stream, const char * restrict format, ...);
int printf(const char * restrict format, ...);
int scanf(const char * restrict format, ...);
int snprintf(char * restrict s, size_t n, const char * restrict format, ...);
int sprintf(char * restrict s, const char * restrict format, ...);
int sscanf(const char * restrict s, const char * restrict format, ...);
int vfprintf(FILE * restrict stream, const char * restrict format,
	va_list arg);
int vfscanf(FILE * restrict stream, const char * restrict format,
	va_list arg);
int vprintf(const char * restrict format, va_list arg);
int vscanf(const char * restrict format, va_list arg);
int vsnprintf(char * restrict s, size_t n, const char * restrict format,
	va_list arg);
int vsprintf(char * restrict s, const char * restrict format,
	va_list arg);
int vsscanf(const char * restrict s, const char * restrict format,
	va_list arg);

/* Character input/output functions */
int fgetc(FILE *stream);
char *fgets(char * restrict s, int n, FILE * restrict stream);
int fputc(int c, FILE *stream);
int fputs(const char * restrict s, FILE * restrict stream);
int getc(FILE *stream);
int getchar(void);
#ifndef _STDIO_H_NOMACRO
#define getchar()       fgetc(stdin)
#endif
char *gets(char *s);
int putc(int c, FILE *stream);
int putchar(int c);
int puts(const char *s);
int ungetc(int c, FILE *stream);

/* Direct input/output functions */
size_t fread(void * restrict ptr, size_t size, size_t nmemb,
    FILE * restrict stream);
size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb,
    FILE * restrict stream);

/* File positioning functions */
int fgetpos(FILE * restrict stream, fpos_t * restrict pos);
int fseek(FILE *stream, long int offset, int whence);
int fsetpos(FILE *stream, const fpos_t *pos);
long int ftell(FILE *stream);
void rewind(FILE *stream);

/* Error-handling functions */
void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void perror(const char *s);

/* AROS specific function to synchronise to keep DOS Input and Output in sync
 * with the C stdin, stdout and stderr
 */
void updatestdio(void);

__END_DECLS


/* POSIX.1-2008 */
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

/* Deprecated functions, not present in POSIX.1-2008 */
int getw(FILE *stream);
int putw(int word, FILE *stream);

__END_DECLS

#endif /* _STDIO_H_ */
