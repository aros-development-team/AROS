#ifndef ACDRBASE_H
#define ACDRBASE_H

#include <dos/bptr.h>
#include <exec/libraries.h>

struct ACDRBase {
	struct Library ab_Lib;
	BPTR ab_SegList;
};

#endif /* ACDRBASE_H */

