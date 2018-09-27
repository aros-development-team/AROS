#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DEBUG 1
#include <aros/debug.h>

#define MYBUG_LEVEL 1
#define mybug(l, x) D(if ((l>=MYBUG_LEVEL)||(l==-1)) { do { { bug x; } } while (0); } )
#define mybug_unit(l, x) D(if ((l>=MYBUG_LEVEL)||(l==-1)) { do { { bug("%s %s: ", unit->name, __FUNCTION__); bug x; } } while (0); } )

#endif /* __DEBUG_H__ */
