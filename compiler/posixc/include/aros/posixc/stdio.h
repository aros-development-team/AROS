#ifndef _POSIXC_STDIO_H_
#define _POSIXC_STDIO_H_

/*
    Copyright © 1995-2021, The AROS Development Team. All rights reserved.
    $Id$

    POSIX.1-2008 header file stdio.h
*/

#include <aros/features.h>

/* C99 */
#include <aros/stdc/stdio.h>

#include <aros/types/off_t.h>
#include <aros/types/ssize_t.h>

#define L_ctermid	FILENAME_MAX	/* Max filename for controlling tty */

#define P_tmpdir	"T:"		/* Default temporary path */

__BEGIN_DECLS

#if !defined(NO_POSIX_WRAPPERS)
FILE *__posixc_fopen(const char * restrict filename, const char * restrict mode);
FILE *fopen64(const char * restrict filename, const char * restrict mode);
int __posixc_fclose(FILE * stream);
int __posixc_fflush(FILE *stream);
#if defined(__USE_FILE_OFFSET64)
static __inline__  FILE *fopen(const char * restrict filename, const char * restrict mode)
{
    return fopen64(filename, mode);
}
#else
static __inline__  FILE *fopen(const char * restrict filename, const char * restrict mode)
{
    return __posixc_fopen(filename, mode);
}
#endif
static __inline__  int fclose(FILE * stream)
{
    return __posixc_fclose(stream);
}
static __inline__ int fflush(FILE *stream)
{
    return __posixc_fflush(stream);
}
#else  /* NO_POSIX_WRAPPERS */
FILE *fopen(const char * restrict filename, const char * restrict mode);
FILE *fopen64(const char * restrict filename, const char * restrict mode);
int fclose(FILE * stream);
int fflush(FILE *stream);
#endif /* NO_POSIX_WRAPPERS */

/* NOTIMPL char	*ctermid(char *); */
/* NOTIMPL int dprintf(int, const char *restrict, ...) */
FILE *fdopen (int filedes, const char *mode);
int fileno(FILE *);
void flockfile(FILE *);
/* NOTIMPL FILE *fmemopen(void *restrict, size_t, const char *restrict) */
#if !defined(NO_POSIX_WRAPPERS)
int __posixc_fgetpos(FILE * restrict stream, fpos_t * restrict pos);
int __posixc_fsetpos(FILE *stream, const fpos_t *pos);
int fgetpos64(FILE * restrict stream, __fpos64_t * restrict pos);
int fsetpos64(FILE *stream, const __fpos64_t *pos);
#if defined(__USE_FILE_OFFSET64)
static __inline__  int fgetpos(FILE * restrict stream, fpos_t * restrict pos)
{
    return fgetpos64(stream, (__fpos64_t *)pos);
}
static __inline__  int fsetpos(FILE *stream, const fpos_t *pos)
{
    return fsetpos64(stream, (__fpos64_t *)pos);
}
#else
static __inline__  int fgetpos(FILE * restrict stream, fpos_t * restrict pos)
{
    return __posixc_fgetpos(stream, pos);
}
static __inline__  int fsetpos(FILE *stream, const fpos_t *pos)
{
    return __posixc_fsetpos(stream, pos);
}
#endif
int __posixc_fseeko(FILE *stream, off_t offset, int whence);
#if defined(__off64_t_defined)
int fseeko64(FILE *stream, off64_t offset, int whence);
#endif
#if defined(__USE_FILE_OFFSET64)
static __inline__  int fseeko(FILE *stream, off_t offset, int whence)
{
    return fseeko64(stream, (__off64_t)offset, whence);
}
#else
static __inline__  int fseeko(FILE *stream, off_t offset, int whence)
{
    return __posixc_fseeko(stream, offset, whence);
}
#endif
off_t __posixc_ftello(FILE *stream);
#if defined(__off64_t_defined)
off64_t ftello64(FILE *stream);
#endif
#if defined(__USE_FILE_OFFSET64)
static __inline__  off_t ftello(FILE *stream)
{
    return (off_t)ftello64(stream);
}
#else
static __inline__  off_t ftello(FILE *stream)
{
    return __posixc_ftello(stream);
}
#endif
int __posixc_feof(FILE *stream);
static __inline__ int feof(FILE *stream)
{
    return __posixc_feof(stream);
}
int __posixc_ferror(FILE *stream);
static __inline__ int ferror(FILE *stream)
{
    return __posixc_ferror(stream);
}
#else  /* NO_POSIX_WRAPPERS */
int fgetpos(FILE * restrict stream, fpos_t * restrict pos);
int fsetpos(FILE *stream, const fpos_t *pos);
int fgetpos64(FILE * restrict stream, __fpos64_t * restrict pos);
int fsetpos64(FILE *stream, const __fpos64_t *pos);
int fseeko(FILE *stream, off_t offset, int whence);
#if defined(__off64_t_defined)
int fseeko64(FILE *stream, off64_t offset, int whence);
#endif
off_t ftello(FILE *stream);
#if defined(__off64_t_defined)
off64_t ftello64(FILE *stream);
#endif
int feof(FILE *stream);
int ferror(FILE *stream);
#endif /* NO_POSIX_WRAPPERS */
/* NOTIMPL int ftrylockfile(FILE *); */
void funlockfile(FILE *);
int getc_unlocked(FILE *);
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

#if !defined(NO_POSIX_WRAPPERS)
int __posixc_vscanf(const char * restrict format, va_list arg);
static __inline__  int vscanf(const char * restrict format, va_list arg)
{
    __posixc_vscanf(format, arg);
}

int __posixc_vprintf(const char * restrict format, va_list arg);
static __inline__  int vprintf(const char * restrict format, va_list arg)
{
    return __posixc_vprintf(format, arg);
}

int __posixc_vfscanf(FILE * restrict stream, const char * restrict format,
	va_list arg);
static __inline__  int vfscanf(FILE * restrict stream, const char * restrict format,
	va_list arg)
{
    return __posixc_vfscanf(stream, format, arg);
}

int __posixc_vfprintf(FILE * restrict stream, const char * restrict format,
	va_list arg);
static __inline__  int vfprintf(FILE * restrict stream, const char * restrict format,
	va_list arg)
{
    return __posixc_vfprintf(stream, format, arg);
}

int __posixc_ungetc(int c, FILE *stream);
static __inline__  int ungetc(int c, FILE *stream)
{
    return __posixc_ungetc(c, stream);
}

FILE *__posixc_tmpfile(void);
static __inline__  FILE *tmpfile(void)
{
    return __posixc_tmpfile();
}

char *__posixc_tmpnam(char *s);
static __inline__  char *tmpnam(char *s)
{
    return __posixc_tmpnam(s);
}

int __posixc_setvbuf(FILE * restrict stream, char * restrict buf, int mode,
	size_t size);
static __inline__  int setvbuf(FILE * restrict stream, char * restrict buf, int mode,
	size_t size)
{
    return __posixc_setvbuf(stream, buf, mode, size);
}

void __posixc_setbuf(FILE * restrict stream, char * restrict buf);
static __inline__  void setbuf(FILE * restrict stream, char * restrict buf)
{
    __posixc_setbuf(stream, buf);
}

int __posixc_scanf(const char * restrict format, ...);
#if !defined(POSIXC_NOINLINE_VAARGS)
#if !defined(POSIXC_NO_VAARGS)
#define scanf(...) __posixc_scanf(__VA_ARGS__)
#else
#define scanf __posixc_scanf
#endif
#else
#if !defined(__cplusplus)
int scanf(const char * restrict format, ...);
#endif
#endif

void __posixc_rewind(FILE *stream);
static __inline__  void rewind(FILE *stream)
{
    __posixc_rewind(stream);
}

int __posixc_puts(const char *s);
static __inline__  int puts(const char *s)
{
    return __posixc_puts(s);
}

int __posixc_putchar(int c);
static __inline__  int putchar(int c)
{
    return __posixc_putchar(c);
}

int __posixc_printf(const char * restrict format, ...);
#if !defined(POSIXC_NOINLINE_VAARGS)
#if !defined(POSIXC_NO_VAARGS)
#define printf(...) __posixc_printf(__VA_ARGS__)
#else
#define printf __posixc_printf
#endif
#else
#if !defined(__cplusplus)
int printf(const char * restrict format, ...);
#endif
#endif

char *__posixc_gets(char *s);
static __inline__  char *gets(char *s)
{
    return __posixc_gets(s);
}

int __posixc_getchar(void);
static __inline__  int getchar(void)
{
    return __posixc_getchar();
}

size_t __posixc_fwrite(const void * restrict ptr, size_t size, size_t nmemb,
    FILE * restrict stream);
static __inline__  size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb,
    FILE * restrict stream)
{
    return __posixc_fwrite(ptr, size, nmemb, stream);
}

long int __posixc_ftell(FILE *stream);
static __inline__  long int ftell(FILE *stream)
{
    return __posixc_ftell(stream);
}

int __posixc_fseek(FILE *stream, long int offset, int whence);
static __inline__  int fseek(FILE *stream, long int offset, int whence)
{
    return __posixc_fseek(stream, offset, whence);
}

int __posixc_fscanf(FILE * restrict stream, const char * restrict format, ...);
#if !defined(POSIXC_NOINLINE_VAARGS)
#if !defined(POSIXC_NO_VAARGS)
#define fscanf(stream, ...) __posixc_fscanf(stream, __VA_ARGS__)
#else
#define fscanf __posixc_fscanf
#endif
#else
#if !defined(__cplusplus)
int fscanf(FILE * restrict stream, const char * restrict format, ...);
#endif
#endif

FILE *__posixc_freopen(const char * restrict filename, const char * restrict mode,
	FILE * restrict stream);
static __inline__  FILE *freopen(const char * restrict filename, const char * restrict mode,
	FILE * restrict stream)
{
    return __posixc_freopen(filename, mode, stream);
}

size_t __posixc_fread(void * restrict ptr, size_t size, size_t nmemb,
    FILE * restrict stream);
static __inline__  size_t fread(void * restrict ptr, size_t size, size_t nmemb,
    FILE * restrict stream)
{
    return __posixc_fread(ptr, size, nmemb, stream);
}

int __posixc_fputs(const char * restrict s, FILE * restrict stream);
static __inline__  int fputs(const char * restrict s, FILE * restrict stream)
{
    return __posixc_fputs(s, stream);
}

int __posixc_fputc(int c, FILE *stream);
static __inline__  int fputc(int c, FILE *stream)
{
    return __posixc_fputc(c, stream);
}

int __posixc_fprintf(FILE * restrict stream, const char * restrict format, ...);
#if !defined(POSIXC_NOINLINE_VAARGS)
#if !defined(POSIXC_NO_VAARGS)
#define fprintf(stream, ...) __posixc_fprintf(stream, __VA_ARGS__)
#else
#define fprintf __posixc_fprintf
#endif
#else
#if !defined(__cplusplus)
int fprintf(FILE * restrict stream, const char * restrict format, ...);
#endif
#endif

char *__posixc_fgets(char * restrict s, int n, FILE * restrict stream);
static __inline__  char *fgets(char * restrict s, int n, FILE * restrict stream)
{
    return __posixc_fgets(s, n, stream);
}
#else
int vscanf(const char * restrict format, va_list arg);
int vprintf(const char * restrict format, va_list arg);
int vfscanf(FILE * restrict stream, const char * restrict format,
	va_list arg);
int vfprintf(FILE * restrict stream, const char * restrict format,
	va_list arg);
int ungetc(int c, FILE *stream);
FILE *tmpfile(void);
char *tmpnam(char *s);
int setvbuf(FILE * restrict stream, char * restrict buf, int mode,
	size_t size);
void setbuf(FILE * restrict stream, char * restrict buf);
int scanf(const char * restrict format, ...);
void rewind(FILE *stream);
int puts(const char *s);
int putchar(int c);
int printf(const char * restrict format, ...);
char *gets(char *s);
int getchar(void);
size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb,
    FILE * restrict stream);
long int ftell(FILE *stream);
int fseek(FILE *stream, long int offset, int whence);
int fscanf(FILE * restrict stream, const char * restrict format, ...);
FILE *freopen(const char * restrict filename, const char * restrict mode,
	FILE * restrict stream);
size_t fread(void * restrict ptr, size_t size, size_t nmemb,
    FILE * restrict stream);
int fputs(const char * restrict s, FILE * restrict stream);
int fputc(int c, FILE *stream);
int fprintf(FILE * restrict stream, const char * restrict format, ...);
char *fgets(char * restrict s, int n, FILE * restrict stream);
#endif

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

#ifdef __cplusplus
#include <utility>
namespace posixc
{
using ::__posixc_scanf;
using ::__posixc_printf;
using ::__posixc_fscanf;
using ::__posixc_fprintf;
} // namespace posixc

#if (__cplusplus >= 201103L )
static auto& scanf = __posixc_scanf;
static auto& printf = __posixc_printf;
static auto& fscanf = __posixc_fscanf;
static auto& fprintf = __posixc_fprintf;
#else
template <typename... Args>
static int scanf(const char * restrict format, Args... args) {
    return posixc::__posixc_scanf(format, args...);
}
template <typename... Args>
static int printf(const char * restrict format, Args... args) {
    return posixc::__posixc_printf(format, args...);
}
template <typename... Args>
static int fscanf(FILE * restrict stream, const char * restrict format, Args... args) {
    return posixc::__posixc_fscanf(stream, format, args...);
}
template <typename... Args>
static int fprintf(FILE * restrict stream, const char * restrict format, Args... args) {
    return posixc::__posixc_fprintf(stream, format, args...);
}
#endif
#endif

#endif /* _POSIXC_STDIO_H_ */
