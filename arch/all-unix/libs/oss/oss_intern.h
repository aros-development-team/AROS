#include <exec/libraries.h>
#include <libcore/base.h>
#include <oop/oop.h>

struct OSSBase_intern
{
    struct LibHeader 	lh;
};

extern OOP_Object *unixio;
extern int audio_fd;
