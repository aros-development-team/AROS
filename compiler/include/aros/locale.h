#ifndef _AROS_LOCALE_H
#define _AROS_LOCALE_H

#define GetString(ID)              \
({                                 \
    extern const char *ID ## _STR; \
                                   \
    ID ## _STR;                    \
})                                  
	
#endif /* !_AROS_LOCALE_H */
