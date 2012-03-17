#include <locale.h>
#include <string.h>

char *setlocale(int category, const char *locale)
{
    if (category < LC_ALL || category > LC_TIME)
        return NULL;

    if (locale == NULL || strcmp(locale, "C") == 0)
        return "C";
    
    return NULL;
}
