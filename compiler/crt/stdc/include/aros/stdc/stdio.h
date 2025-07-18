#ifndef _STDC_STDIO_H_
#define _STDC_STDIO_H_

/*
    Copyright © 1995-2025, The AROS Development Team. All rights reserved.
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

__BEGIN_DECLS

#ifndef _AROS_TYPES_FILE_S_H
struct __sFILE;
#endif
typedef struct __sFILE FILE;

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
FILE *fopen(const char * restrict filename, const char * restrict mode);
FILE *freopen(const char * restrict filename, const char * restrict mode, FILE * restrict stream);
void setbuf(FILE * restrict stream, char * restrict buf);
int setvbuf(FILE * restrict stream, char * restrict buf, int mode, size_t size);

/* Formatted I/O */
int fprintf(FILE * restrict stream, const char * restrict format, ...);
int fscanf(FILE * restrict stream, const char * restrict format, ...);
int printf(const char * restrict format, ...);
int scanf(const char * restrict format, ...);
int snprintf(char * restrict s, size_t n, const char * restrict format, ...);
int sprintf(char * restrict s, const char * restrict format, ...);
int sscanf(const char * restrict s, const char * restrict format, ...);
int vfprintf(FILE * restrict stream, const char * restrict format, va_list arg);
int vfscanf(FILE * restrict stream, const char * restrict format, va_list arg);
int vprintf(const char * restrict format, va_list arg);
int vscanf(const char * restrict format, va_list arg);
int vsnprintf(char * restrict s, size_t n, const char * restrict format, va_list arg);
int vsprintf(char * restrict s, const char * restrict format, va_list arg);
int vsscanf(const char * restrict s, const char * restrict format, va_list arg);

/* GNU extensions */
#if defined(__cplusplus) || (!defined(__STRICT_ANSI__) && defined(_GNU_SOURCE))
int asprintf(char ** restrict str, const char * restrict format, ...);
int vasprintf(char ** restrict str, const char * restrict format, va_list args);
#endif

/* Character I/O */
int fgetc(FILE *stream);
char *fgets(char * restrict s, int n, FILE * restrict stream);
int fputc(int c, FILE *stream);
int fputs(const char * restrict s, FILE * restrict stream);
int getc(FILE *stream);
int getchar(void);
char *gets(char *s);
int putchar(int c);
int puts(const char *s);
int ungetc(int c, FILE *stream);
int putc(int c, FILE *stream);

/* Direct I/O */
size_t fread(void * restrict ptr, size_t size, size_t nmemb, FILE * restrict stream);
size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb, FILE * restrict stream);

/* File positioning */
int fgetpos(FILE * restrict stream, fpos_t * restrict pos);
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
