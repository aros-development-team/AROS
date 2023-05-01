#ifndef __DIRDESC_H
#define __DIRDESC_H

#include <dirent.h>

#include <aros/types/off_t.h>

struct __dirdesc
{
   int                  fd;
   void                 *priv;
   struct dirent        ent;
   struct dirent64      ent64;
   off_t                pos;
};

#endif /* __DIRDESC_H */
