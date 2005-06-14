#ifndef _BUSY_PRIVATE_H_
#define _BUSY_PRIVATE_H_

#include <exec/types.h>
#include <utility/date.h>

/*** Instance data **********************************************************/
struct Busy_DATA
{
    struct MUI_InputHandlerNode ihn;
    UWORD   	    	    	flags;
    UWORD   	    	    	speed;
    UWORD   	    	    	pos;
};

#define FLG_SHOWHIDEIH	1
#define FLG_SHOWN   	2
#define FLG_SETUP   	4

#endif /* _BUSY_PRIVATE_H_ */
