#ifndef IMG_H
#define IMG_H

#ifndef TOOLLIB_TOOLLIB_H
#   include <toollib/toollib.h>
#endif

int IMG_GetSize PARAMS ((const char * filename, int * width, int * height));

#endif /* IMG_H */
