#ifndef _CRAWLING_PRIVATE_H_
#define _CRAWLING_PRIVATE_H_


#define CRAWLING_INITIAL_DELAY (5 * 10)

struct Crawling_DATA
{
    struct MUI_EventHandlerNode ehn;
    LONG    	    	    	ticker;
};

#endif /* _CRAWLING_PRIVATE_H_ */
