/*
    Copyright (C) 2006 The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: A very primitive clib replacements.
    Lang: English
*/

#include "support.h"

void *memset(void *ptr, int c, long len)
{
    void *p = ptr;
    long c32 = c | (c << 8) | (c << 16) | (c << 24);
        
    while (len >= 4)
    {
        *(unsigned long *)ptr = c32;
        len-=4;
        ptr+=4;
    }
    if (len >= 2)
    {
        *(unsigned short *)ptr = c32;
        ptr+=2;
        len-=2;
    }
    if (len == 1)
    {
        *(unsigned char *)ptr = c32;
    }    
    return p;
}

void *bzero(void *ptr, long len)
{
    return memset(ptr, 0, len);
}

void memcpy(void *dest, const void *src, long len)
{
    while (len >= 4)
    {
        *(unsigned long *)dest = *(unsigned long *)src;
        len-=4;
        dest+=4;
        src+=4;
    }
    if (len >= 2)
    {
        *(unsigned short *)dest = *(unsigned short *)src;
        dest+=2;
        src+=2;
        len-=2;
    }
    if (len == 1)
    {
        *(unsigned char *)dest = *(unsigned char *)src;      
    }
}

int strncmp(const char *s1, const char*s2, long length)
{
    if (!length)
        return 0;

    while (length-- && *s1 && *s2)
    {
        if (*s1 != *s2) return (*s1-*s2);
        s1++; s2++;
    }
    return 0;
}

int strlen(const char *s)
{
    int len=0;
    while(*s++ != 0) len++;
    return len;
}

const char *remove_path(const char *in)
{
    const char *p = &in[strlen(in)-1];
    while (p > in && p[-1] != '/' && p[-1] != ':') p--;
    return p;
}

char *strstr (const char * str, const char * search)
{
    long done;
    long len_s = strlen(search);
    const char * t;

    do
    {
        if (*search == *str)
        {
            done = len_s;

            t = search + 1;
            str++;
   
            while ((--done) && (*t == *str))
            {
                t++;
                str++;
            }

            if (!done)
            {
                str -= len_s;
                return ((char *)str);
            }
            else
            {
                str -= len_s - done;
            }
        }
    } while (*str++);

    return(0);
}

