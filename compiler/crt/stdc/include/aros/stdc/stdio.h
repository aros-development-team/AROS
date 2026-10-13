#ifndef _STDC_STDIO_H_
#define _STDC_STDIO_H_

/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AROS implementation of the C Standard Input and Output Header (C89/C99/GNU)
*/

#include <aros/system.h>
#include <stdarg.h>
#include <aros/types/size_t.h>
#include <aros/types/fpos_t.h>
#include <aros/types/null.h>
#include <aros/types/seek.h>

#define _IOFBF 0 /* Fully buffered. */
#define _IOLBF 1 /* Line buffered. */
#define _IONBF 2 /* Not buffered. */

#ifndef BUFSIZ
#   define BUFSIZ 1024
#endif
#ifndef EOF
#   define EOF (-1)
#endif
#define FOPEN_MAX 16
#define FILENAME_MAX 256
#define L_tmpnam FILENAME_MAX
#define TMP_MAX 10240

#include <aros/types/file_t.h>

__BEGIN_DECLS

#if defined(_POSIXC_STDIO_H_)
FILE *__posixc_getstdin(void);
FILE *__posixc_getstdout(void);
FILE *__posixc_getstderr(void);
#define stdin __posixc_getstdin()
#define stdout __posixc_getstdout()
#define stderr __posixc_getstderr()
#else
FILE *__stdio_getstdin(void);
FILE *__stdio_getstdout(void);
FILE *__stdio_getstderr(void);
#define stdin __stdio_getstdin()
#define stdout __stdio_getstdout()
#define stderr __stdio_getstderr()
#endif

int __vcformat(void *data, int (*outc)(int, void *), const char *format, va_list args);
int __vcscan(void *data, int (*getc)(void *), int (*ungetc)(int, void *), const char *format, va_list args);

/* Standard operations on files */
int remove(const char *filename);
int rename(const char *from, const char *to);
FILE *tmpfile(void);
char *tmpnam(char *s);

/* File access */
int fclose(FILE *stream);
int fflush(FILE *stream);
FILE *fopen(const char * __restrict filename, const char * __restrict mode);
FILE *freopen(const char * __restrict filename, const char * __restrict mode, FILE * __restrict stream);
void setbuf(FILE * __restrict stream, char * __restrict buf);
int setvbuf(FILE * __restrict stream, char * __restrict buf, int mode, size_t size);

/* Formatted I/O */
int fprintf(FILE * __restrict stream, const char * __restrict format, ...);
int fscanf(FILE * __restrict stream, const char * __restrict format, ...);
int printf(const char * __restrict format, ...);
int scanf(const char * __restrict format, ...);
int snprintf(char * __restrict s, size_t n, const char * __restrict format, ...);
int sprintf(char * __restrict s, const char * __restrict format, ...);
int sscanf(const char * __restrict s, const char * __restrict format, ...);
int vfprintf(FILE * __restrict stream, const char * __restrict format, va_list arg);
int vfscanf(FILE * __restrict stream, const char * __restrict format, va_list arg);
int vprintf(const char * __restrict format, va_list arg);
int vscanf(const char * __restrict format, va_list arg);
int vsnprintf(char * __restrict s, size_t n, const char * __restrict format, va_list arg);
int vsprintf(char * __restrict s, const char * __restrict format, va_list arg);
int vsscanf(const char * __restrict s, const char * __restrict format, va_list arg);

/* GNU extensions */
#if defined(__cplusplus) || (!defined(__STRICT_ANSI__) && defined(_GNU_SOURCE))
int asprintf(char ** __restrict str, const char * __restrict format, ...);
int vasprintf(char ** __restrict str, const char * __restrict format, va_list args);
#endif

/* Character I/O */
int fgetc(FILE *stream);
char *fgets(char * __restrict s, int n, FILE * __restrict stream);
int fputc(int c, FILE *stream);
int fputs(const char * __restrict s, FILE * __restrict stream);
int getc(FILE *stream);
int getchar(void);
char *gets(char *s);
int putchar(int c);
int puts(const char *s);
int ungetc(int c, FILE *stream);
int putc(int c, FILE *stream);

/* Direct I/O */
size_t fread(void * __restrict ptr, size_t size, size_t nmemb, FILE * __restrict stream);
size_t fwrite(const void * __restrict ptr, size_t size, size_t nmemb, FILE * __restrict stream);

/* File positioning */
int fgetpos(FILE * __restrict stream, fpos_t * __restrict pos);
int fsetpos(FILE *stream, const fpos_t *pos);
int fseek(FILE *stream, long int offset, int whence);
long int ftell(FILE *stream);
void rewind(FILE *stream);

/* Error handling */
void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void perror(const char *s);

/* AROS specific function */
void updatestdio(void);

__END_DECLS

#endif /* _STDC_STDIO_H_ */
