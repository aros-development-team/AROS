#define VECTORS_NUM 5

#include <exec/libraries.h>

struct CIABase
{
    struct Library    lib;
    struct Interrupt *Vectors[VECTORS_NUM];
};
