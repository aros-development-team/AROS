/*
    Copyright 2009-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <proto/dos.h>

#define DEBUG 0
#include <aros/debug.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <limits.h>

/* 
    The purpose of this file is to provide implementation for C functions which 
    are used by a module and are not located in librom.a

    
    A module cannot use arosc.library because arosc.library context is bound 
    with caller task and gets destroyed when the task exits. After the
    caller task exists, the module becomes instable
*/

#define IMPLEMENT()  bug("------IMPLEMENT(%s)\n", __func__)

/* malloc/calloc/realloc/free */

/* This is a copy of implementation from arosc.library so that mesa.library uses its 
   own space for malloc/calloc/realloc calls */

#define MEMALIGN_MAGIC ((size_t) 0x57E2CB09)
APTR __mempool = NULL;

void * malloc (size_t size)
{
    UBYTE *mem = NULL;

    D(bug("arosc_emul_malloc\n"));
    /* Allocate the memory */
    mem = AllocPooled (__mempool, size + AROS_ALIGN(sizeof(size_t)));
    if (mem)
    {
        *((size_t *)mem) = size;
        mem += AROS_ALIGN(sizeof(size_t));
    }
    else
        errno = ENOMEM;

    return mem;
}

void free (void * memory)
{
    D(bug("arosc_emul_free\n"));
    if (memory)
    {
        unsigned char *mem;
        size_t         size;

        mem = ((UBYTE *)memory) - AROS_ALIGN(sizeof(size_t));

        size = *((size_t *) mem);
        if (size == MEMALIGN_MAGIC)
            free(((void **) mem)[-1]);
        else 
        {
            size += AROS_ALIGN(sizeof(size_t));
            FreePooled (__mempool, mem, size);
        }
    }
}

void * calloc (size_t count, size_t size)
{
    ULONG * mem;
    D(bug("arosc_emul_calloc\n"));
    size *= count;

    /* Allocate the memory */
    mem = malloc (size);

    if (mem)
        memset (mem, 0, size);

    return mem;
}

void * realloc (void * oldmem, size_t size)
{
    
    UBYTE * mem, * newmem;
    size_t oldsize;
    
    D(bug("arosc_emul_realloc\n"));
    
    if (!oldmem)
        return malloc (size);

    mem = (UBYTE *)oldmem - AROS_ALIGN(sizeof(size_t));
    oldsize = *((size_t *)mem);

    /* Reduce or enlarge the memory ? */
    if (size < oldsize)
    {
        /* Don't change anything for small changes */
        if ((oldsize - size) < 4096)
            newmem = oldmem;
        else
            goto copy;
    }
    else if (size == oldsize) /* Keep the size ? */
        newmem = oldmem;
    else
    {
copy:
        newmem = malloc (size);

        if (newmem)
        {
            CopyMem (oldmem, newmem, size);
            free (oldmem);
        }
    }

    return newmem;
}

char *getenv (const char *name)
{ 
    /* This function is not thread-safe */
    static TEXT buff[128] = { 0 };
    
    D(bug("arosc_emul_getenv: %s\n", name));

    if (GetVar(name, buff, 128, GVF_BINARY_VAR) == -1)
        return NULL;
    else
        return buff;
}

struct timezone;
int gettimeofday (struct timeval * tv,struct timezone * tz)
{
    IMPLEMENT();
    return 0;
}

int usleep (useconds_t usec)
{
    IMPLEMENT();
    return 0;
}

__noreturn void abort (void)
{
    IMPLEMENT();
    for (;;);
}
 
__noreturn void exit (int code)
{
    IMPLEMENT();
    for (;;);
}

int puts (const char * str)
{
    bug("%s\n", str);
    return 1;
}

int putchar(int c)
{
    bug("%c", c);
    return c;
}

char * strdup (const char * orig)
{
    char * copy;
    char * ptr;

    if ((copy = malloc (strlen (orig)+1)))
    {
        ptr = copy;

        while ((*ptr ++ = *orig ++));
    }

    return copy;
}

int printf (const char * format, ...)
{
    IMPLEMENT();
    return EOF;
}

#undef getc
int getc (FILE * stream)
{
    IMPLEMENT();
    return EOF;
}

/* File operations */
FILE * fopen (const char * pathname, const char * mode)
{
    IMPLEMENT();
    return NULL;
}

int fclose (FILE * stream)
{
    IMPLEMENT();
    return EOF;
}

int fputc (int c, FILE * stream)
{
    IMPLEMENT();
    return EOF;
}

int fputs (const char * str, FILE * stream)
{
    IMPLEMENT();
    return EOF;
}

int fgetc (FILE * stream)
{
    IMPLEMENT();;
    return EOF;
}

size_t fwrite (const void * restrict buf, size_t size, size_t nblocks, FILE * restrict stream)
{
    ULONG i;
    for (i = 0; i < size * nblocks; i++)
        bug("%c", ((char*)buf)[i]);

    return 0;
}

int fflush (FILE * stream)
{
    IMPLEMENT();
    return EOF;
}

size_t fread (void * buf, size_t size, size_t nblocks, FILE * stream)
{
    IMPLEMENT();
    return 0;
}

int ferror (FILE * stream)
{
    IMPLEMENT();
    return TRUE;
}

void clearerr (FILE * stream)
{
    IMPLEMENT();
}

int fprintf (FILE * fh, const char * format, ...)
{
    IMPLEMENT();
    return 0;
}

int vfprintf(FILE * restrict stream, const char * restrict format,
    va_list arg)
{
    IMPLEMENT();
    return 0;
}

FILE *__stdio_getstdin(void)
{
    IMPLEMENT();
    return NULL;
}

FILE *__stdio_getstderr(void)
{
    IMPLEMENT();
    return NULL;
}

FILE *__stdio_getstdout(void)
{
    IMPLEMENT();
    return NULL;
}

double strtod (const char * str,char ** endptr)
{
    /* Unit tests available in : tests/clib/strtod.c */
    /* FIXME: implement NAN handling */
    double  val = 0, precision;
    int     exp = 0;
    char    c = 0, c2 = 0;
    int     digits = 0;      

    /* skip all leading spaces */
    while (isspace (*str))
        str ++;

    /* start with scanning the floting point number */
    if (*str)
    {
        /* Is there a sign? */
        if (*str == '+' || *str == '-')
            c = *str ++;

        /* scan numbers before the dot */
        while (isdigit(*str))
        {
            digits++;
            val = val * 10 + (*str - '0');
            str ++;
        }

        /* see if there is the dot and there were digits before it or there is 
            at least one digit after it */
        if ((*str == '.') && ((digits > 0) || (isdigit(*(str + 1)))))
        {
            str++;
            /* scan the numbers behind the dot */
            precision = 0.1;
            while (isdigit (*str))
            {
                digits++;
                val += ((*str - '0') * precision) ;
                str ++;
                precision = precision * 0.1;
            }
        }

        /* look for a sequence like "E+10" or "e-22" if there were any digits up to now */
        if ((digits > 0) && (tolower(*str) == 'e'))
        {
            int edigits = 0;
            str++;

            if (*str == '+' || *str == '-')
                c2 = *str ++;

            while (isdigit (*str))
            {
                edigits++;
                exp = exp * 10 + (*str - '0');
                str ++;
            }
            
            if (c2 == '-')
                exp = -exp;
            
            if (edigits == 0) 
            {
                /* there were no digits after 'e' - rollback pointer */
                str--; if (c2 != 0) str--;
            }
            
            val *= pow (10, exp);
        }

        if (c == '-')
            val = -val;

        if ((digits == 0) && (c != 0))
        {
            /* there were no digits but there was sign - rollback pointer */
            str--;
        }
    }

    if (endptr)
        *endptr = (char *)str;

    return val;
} /* strtod */

char * strchr (const char * str, int c)
{
    do
    {
        /* those casts are needed to compare chars > 127 */
        if ((unsigned char)*str == (unsigned char)c)
            return ((char *)str);
    } while (*(str++));

    return NULL;
} /* strchr */

size_t strcspn (const char * str, const char * reject)
{
    size_t n = 0; /* Must set this to zero */

    while (*str && !strchr (reject, *str))
    {
        str ++;
        n ++;
    }

    return n;
} /* strcspn */

char * strtok (char * str, const char * sep)
{
    static char * t;

    if (str != NULL)
        t = str;
    else
        str = t;

    str += strspn (str, sep);

    if (*str == '\0')
        return NULL;

    t = str;

    t += strcspn (str, sep);

    if (*t != '\0')
        *t ++ = '\0';

    return str;
} /* strtok */

double atof (const char * str)
{
    return strtod (str, (char **)NULL);
} /* atof */

int fscanf (FILE * fh,const char * format, ...)
{
    IMPLEMENT();
    return 0;
}

/*
    This implementation of atexit is different than the definition of atexit
    function due to how libraries work in AROS.
   
    Under Linux, when an .so file is used by an application, the library's code
    is being shared but the library's data (global, static variables) are COPIED for
    each process. Then, an atexit call inside .so will only operate on COPY of data
    and thus can for example free memory allocated by one process without
    influencing other processes.
   
    Under AROS, when a .library file is used by an application, library code AND
    library data is shared. This means, an atexit call inside .library which was
    initially coded under Linux cannot be executed when process is finishing 
    (for example at CloseLibrary) because such call will most likely free shared 
    data which will make other processes crash. The best approximation of atexit
    in case of .library is to call the atexit functions at library expunge/exit.
*/

static struct exit_list {
    struct exit_list *next;
    void (*func)(void);
} *exit_list = NULL;

int atexit(void (*function)(void))
{
    struct exit_list *el;

    el = malloc(sizeof(*el));
    if (el == NULL)
        return -1;

    el->next = exit_list;
    el->func = function;
    exit_list = el;

    return 0;
}

int __init_emul(void)
{
    /* malloc/calloc/realloc/free */
    __mempool = CreatePool(MEMF_ANY | MEMF_SEM_PROTECTED, 65536L, 4096L);

    if (!__mempool)
    {
        return 0;
    }

    return 1;
}

void __exit_emul(void)
{
    while (exit_list) {
        struct exit_list *el = exit_list->next;

        exit_list->func();
        free(exit_list);
        exit_list = el;
    }

    /* malloc/calloc/realloc/free */
    if (__mempool)
    {
        DeletePool(__mempool);
    }
}

ADD2INIT(__init_emul, 0);
ADD2EXIT(__exit_emul, 0);
