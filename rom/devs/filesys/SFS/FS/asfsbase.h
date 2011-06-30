#ifndef ASFSBASE_H
#define ASFSBASE_H

#include <exec/libraries.h>
#include <dos/bptr.h>

struct ASFSBase {
	struct Library ab_Lib;
	BPTR ab_SegList;
};

#endif /* ASFSBASE_H */
