#ifndef _CRAWLING_PRIVATE_H_
#define _CRAWLING_PRIVATE_H_

#include <exec/types.h>
#include <libraries/mui.h>

#define CRAWLING_INITIAL_DELAY (5 * 10)

struct Crawling_DATA
{
    struct MUI_EventHandlerNode ehn;
    LONG    	    	    	ticker;
};

#endif /* _CRAWLING_PRIVATE_H_ */
