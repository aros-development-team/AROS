#ifndef _STDIO_H
#define _STDIO_H

/*
    Copyright (C) 1995-2001 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI-C header file stdio.h
    Lang: english
*/
#include <stdarg.h>
#ifndef _SYS_TYPES_H
#   include <sys/types.h>
#endif

#ifndef NULL
#   ifdef __cplusplus
#	define NULL 0
#   else
#	define NULL (void*)0
#   endif /* __cplusplus */
#endif /* NULL */

#ifndef EOF
#   define EOF (-1)
#endif

/* Buffering methods that can be specified with setvbuf() */
#define _IOFBF 0 /* Fully buffered. */
#define _IOLBF 1 /* Line buffered. */
#define _IONBF 2 /* Not buffered. */

#ifndef BUFSIZ
#   define BUFSIZ 1024
#endif
#define FILENAME_MAX 1024

#ifndef __typedef_FILE
#   define __typedef_FILE
    typedef struct
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

#ifndef __typedef_fpos_t
#   define __typedef_fpos_t
typedef long fpos_t;
#endif

#ifndef _UNISTD_H    /*unistd.h has the same definitions */

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#endif

#ifndef _CLIB_KERNEL_
    extern FILE *stdin, *stdout, *stderr;
#endif

extern FILE *fopen (const char * name, const char * mode);
extern FILE *fdopen (int filedes, const char *mode);
extern FILE *freopen (const char *path, const char *mode, FILE *stream );
extern int fclose (FILE *);
extern int printf (const char * format, ...);
extern int vprintf (const char * format, va_list args);
extern int fprintf (FILE * fh, const char * format, ...);
extern int vfprintf (FILE * fh, const char * format, va_list args);
extern int fputc (int c, FILE * stream);
extern int fputs (const char * str, FILE * stream);
extern int putchar(int c);
extern int puts (const char * str);
extern int fflush (FILE * stream);
extern int fgetc (FILE * stream);
extern int ungetc (int c, FILE * stream);
extern char * fgets (char * buffer, int size, FILE * stream);
extern int feof (FILE * stream);
extern int ferror (FILE * stream);
extern int fileno (FILE * stream);
extern FILE *feropen (const char *path,	const char *mode, FILE *stream);

extern void clearerr (FILE * stream);
extern size_t fread (void *ptr, size_t size, size_t nmemb, FILE * stream);
extern size_t fwrite (void *ptr, size_t size, size_t nmemb, FILE * stream);
extern int rename (const char *from, const char *to);
extern int sprintf (char * str, const char * format, ...);
extern int vsprintf (char * str, const char * format, va_list args);
extern int snprintf (char * str, size_t n, const char * format, ...);
extern int vsnprintf (char * str, size_t n, const char * format, va_list args);

extern int scanf (const char * format, ...);
extern int vscanf (const char * format, va_list args);
extern int fscanf (FILE * fh, const char * format, ...);
extern int vfscanf (FILE * fh, const char * format, va_list args);
extern int sscanf (const char * str, const char * format, ...);
extern int vsscanf (const char * str, const char * format, va_list args);

extern int fseek (FILE * stream, long offset, int whence);
extern long ftell (FILE * stream);
extern void rewind (FILE * stream);
extern int fgetpos (FILE * stream, fpos_t * pos);
extern int fsetpos (FILE * stream, fpos_t * pos);

extern int remove(const char * pathname);

extern int setbuf(FILE *stream, char *buf);
extern int setlinebuf(FILE *stream);
extern int setvbuf(FILE *stream, char *buf, int mode, size_t size);

#ifdef AROS_ALMOST_COMPATIBLE
extern int __vcformat (void * data, int (*outc)(int, void *),
			const char * format, va_list args);
extern int __vcscan (void * data, int (*getc)(void *),
			int (*ungetc)(int, void *),
			const char * format, va_list args);
#endif

#define putc fputc
#define getc fgetc
#define getchar()   getc(stdin)
#define gets(s)     fgets(s, BUFSIZ, stdin)

#endif /* _STDIO_H */
