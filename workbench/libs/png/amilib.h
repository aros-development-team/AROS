#ifndef _AMILIB_H
#define _AMILIB_H

#include <exec/libraries.h>

struct PngBase
{
   struct Library _lib;
   struct Library *_aroscbase;
   struct Library *_zbase;
};

#endif /* _AMILIB_H */
