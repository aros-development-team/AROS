#ifndef _CALENDAR_PRIVATE_H_
#define _CALENDAR_PRIVATE_H_

#include <exec/types.h>
#include <utility/date.h>
#include <libraries/mui.h>

/*** Instance data **********************************************************/
struct Calendar_DATA
{
    struct MUI_EventHandlerNode ehn;
    struct ClockData	    	clockdata;
    CONST_STRPTR  	    	*daylabels;
    CONST_STRPTR  	    	defdaylabels[7];
    WORD    	    	    	cellwidth, base_cellwidth;
    WORD    	    	    	cellheight, base_cellheight;
    WORD    	    	    	mwday; /* weekday of 1st of month */
    WORD    	    	    	old_mday;
    LONG                        firstweekday;
};

#endif /* _CALENDAR_PRIVATE_H_ */
