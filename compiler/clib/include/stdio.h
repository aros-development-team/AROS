#ifndef _STDIO_H_
#define _STDIO_H_

/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file stdio.h
    Lang: english
*/
#include <aros/system.h>
#include <sys/arosc.h>

#include <aros/types/null.h>
#include <aros/types/size_t.h>
#include <aros/types/off_t.h>

typedef off_t fpos_t;

/*
    FIXME: We are supposed to declare it, without including the file.
    This is too compiler specific to handle at the moment.
*/
#include <stdarg.h>

/* Need to protect against standard Amiga includes */
#ifndef EOF
#   define EOF (-1)
#endif

/* Buffering methods that can be specified with setvbuf() */
#define _IOFBF 0 /* Fully buffered. */
#define _IOLBF 1 /* Line buffered. */
#define _IONBF 2 /* Not buffered. */

#ifndef BUFSIZ
#   define BUFSIZ	1024
#endif
#define FILENAME_MAX	256		/* Amiga files are 256 */
#define FOPEN_MAX	16		/* Must be > 8 */
#define TMP_MAX		10240		/* Must be > 10000 */
#define L_tmpnam	FILENAME_MAX	/* Max temporary filename */

#if !defined(_ANSI_SOURCE)
#define L_ctermid	FILENAME_MAX	/* Max filename for controlling tty */
#endif

#define P_tmpdir	"T:"		/* Default temporary path */

#ifndef __typedef_FILE
#   define __typedef_FILE
    /* I need a named struct for FILE, so that I can use it in wchar.h> */
    typedef struct __sFILE
    {
	int fd;
	int flags;
    } FILE;

#   define _STDIO_EOF    0x0001L
#   define _STDIO_ERROR  0x0002L
#   define _STDIO_WRITE  0x0004L
#   define _STDIO_READ   0x0008L
#   define _STDIO_RDWR   _STDIO_WRITE | _STDIO_READ
#   define _STDIO_APPEND 0x0010L
#endif

#include <aros/types/seek.h>            /* SEEK_SET, SEEK_CUR and SEEK_END */

#define stdin  (__get_arosc_userdata()->acud_stdin)
#define stdout (__get_arosc_userdata()->acud_stdout)
#define stderr (__get_arosc_userdata()->acud_stderr)

__BEGIN_DECLS


int remove(const char *filename);
int rename(const char *from, const char *to);
FILE *tmpfile(void);
char *tmpnam(char *s);
char *tempnam(const char *dir, const char *pfx);
int fclose(FILE *stream);
int fflush(FILE *stream);
FILE *fopen(const char * restrict filename, const char * restrict mode);
FILE *freopen(const char * restrict filename, const char * restrict mode,
	FILE * restrict stream);
void setbuf(FILE * restrict stream, char * restrict buf);
int setvbuf(FILE * restrict stream, char * restrict buf, int mode,
	size_t size);
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
int fgetc(FILE *stream);
char *fgets(char * restrict s, int n, FILE * restrict stream);
int fputc(int c, FILE *stream);
int fputs(const char * restrict s, FILE * restrict stream);
int getc(FILE *stream);
int getchar(void);
char *gets(char *s);
int putc(int c, FILE *stream);
int putchar(int c);
int puts(const char *s);
int ungetc(int c, FILE *stream);
size_t fread(void * restrict ptr, size_t size, size_t nmemb,
    FILE * restrict stream);
size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb,
    FILE * restrict stream);
int fgetpos(FILE * restrict stream, fpos_t * restrict pos);
int fseek(FILE *stream, long int offset, int whence);
int fseeko(FILE *stream, off_t offset, int whence);
int fsetpos(FILE *stream, const fpos_t *pos);
long int ftell(FILE *stream);
off_t ftello(FILE *stream);
void rewind(FILE *stream);
void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void perror(const char *s);

/* Internal functions */
int __vcformat (void * data, int (*outc)(int, void *),
		const char * format, va_list args);
int __vcscan (void * data, int (*getc)(void *),
		int (*ungetc)(int, void *),
		const char * format, va_list args);

/* AROS specific function to synchronise to keep DOS Input and Output in sync
 * with the C stdin, stdout and stderr
 */
void updatestdio(void);

#define putc(c, stream) fputc(c, stream)
#define getc(stream)    fgetc(stream)
#define getchar()       getc(stdin)

#if !defined(_ANSI_SOURCE)
/* Unix Specific */
FILE    *fdopen (int filedes, const char *mode);
int	 fileno(FILE *);
int      pclose(FILE *);
FILE    *popen(const char *, const char *);
FILE    *tmpfile(void);
char    *tmpnam(char *);
#endif /* !_ANSI_SOURCE */

#if __BSD_VISIBLE
void    setlinebuf(FILE *stream);
#endif

/* NOTIMPL char    *tempnam(const char *, const char *); */

/* NOTIMPL char	*ctermid(char *); */
/* NOTIMPL char	*ctermid_r(char *); */

/* NOTIMPL void     flockfile(FILE *); */
/* NOTIMPL int      ftrylockfile(FILE *); */
/* NOTIMPL void     funlockfile(FILE *); */

/* NOTIMPL int      getc_unlocked(FILE *); */
/* NOTIMPL int      getchar_unlocked(void); */
/* NOTIMPL int      putc_unlocked(int, FILE *); */
/* NOTIMPL int      putchar_unlocked(int); */

int      getw(FILE *stream);
int      putw(int word, FILE *stream);

__END_DECLS

#endif /* _STDIO_H_ */
