#ifndef _AROS_LOCALE_H
#define _AROS_LOCALE_H

#include <aros/system.h>

#define GetString(ID)                      \
({                                         \
    extern const char *__CONCAT(ID, _STR); \
                                           \
    __CONCAT(ID, _STR);                    \
})                                  
	
#endif /* !_AROS_LOCALE_H */
