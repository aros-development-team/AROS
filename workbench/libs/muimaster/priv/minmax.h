#ifndef __MINMAX_H__
#define __MINMAX_H__

/**** Flavio: why don't we use libraries/mui.h directly? ***/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

struct MUI_MinMax
{
	WORD MinWidth;
	WORD MinHeight;
	WORD MaxWidth;
	WORD MaxHeight;
	WORD DefWidth;
	WORD DefHeight;
};

#define MUI_MAXMAX 10000 /* use this if a dimension is not limited. */

#endif

