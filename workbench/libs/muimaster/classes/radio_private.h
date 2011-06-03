#ifndef _RADIO_PRIVATE_H_
#define _RADIO_PRIVATE_H_

#include <intuition/classusr.h>

/*** Instance data **********************************************************/
struct Radio_DATA
{
    int      entries_active;
    int      entries_num;
    Object **buttons;
};

#endif /* _RADIO_PRIVATE_H_ */
