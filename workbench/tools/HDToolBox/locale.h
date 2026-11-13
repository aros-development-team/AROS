#ifndef HDTBLOCALE_H
#define HDTBLOCALE_H

void InitLocale(STRPTR, ULONG);
void CleanupLocale(void);
CONST_STRPTR MSG(ULONG);
CONST_STRPTR MSG_STD(ULONG);

#endif

