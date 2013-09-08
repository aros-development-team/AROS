#ifndef __DIRDESC_H
#define __DIRDESC_H

#include <dirent.h>

#include <aros/types/off_t.h>

struct __dirdesc
{
   int    fd;
   struct dirent ent;
   off_t  pos;
   void   *priv;
};

#endif /* __DIRDESC_H */
