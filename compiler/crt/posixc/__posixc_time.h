#include "__posixc_intbase.h"

#define TimerBase (PosixCBase->timerBase)

void __init_timerbase(struct PosixCIntBase *PosixCBase);

/* 2922 is the number of days between 1.1.1970 and 1.1.1978 (2 leap
   years and 6 normal). The former number is the start value
   for time(), the latter the start time for the AmigaOS
   time functions.
*/
#define OFFSET_FROM_1970 2922*24*60*60
