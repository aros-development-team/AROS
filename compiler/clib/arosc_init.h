#ifndef _AROSC_INIT_H
#define _AROSC_INIT_H

#include "__arosc_privdata.h"

int arosc_internalinit(struct arosc_privdata **privdata_ptr);
int arosc_internalexit( void );

#endif /* _AROSC_INIT_H */
