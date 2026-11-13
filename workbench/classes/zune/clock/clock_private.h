#ifndef _CLOCK_PRIVATE_H_
#define _CLOCK_PRIVATE_H_

#include <exec/types.h>
#include <utility/date.h>
#include <libraries/mui.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>

/*** Instance data **********************************************************/
struct Clock_DATA
{
    struct MUI_InputHandlerNode ihn;
    struct ClockData	    	clockdata;
    struct BitMap   	    	*clockbm;
    struct RastPort 	    	clockrp;
    struct AreaInfo 	    	clockai;
    struct TmpRas   	    	clocktr;
    APTR    	    	    	clockraster;
    WORD    	    	    	areabuf[30];
    WORD    	    	    	clockbmr, clockbmw, clockbmh;
    WORD    	    	    	edithand;
    WORD    	    	    	editpen;
    BOOL    	    	    	frozen;
};

#endif /* _CLOCK_PRIVATE_H_ */
