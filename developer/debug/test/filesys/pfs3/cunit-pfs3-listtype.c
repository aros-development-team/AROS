#include <exec/types.h>

#include <CUnit/CUnit.h>

#include "blocks.h"
#include "struct.h"

void TestListTypeLayout(void)
{
	listtype type;

	type.value = ET_FILEENTRY | ET_EXCLWRITE;
	CU_ASSERT_EQUAL(type.flags.type, ETF_FILEENTRY);
	CU_ASSERT_EQUAL(type.flags.access, ET_EXCLWRITE);
	CU_ASSERT_EQUAL(type.flags.dir, 0);

	type.value = 0;
	type.flags.type = ETF_LOCK;
	type.flags.access = ET_EXCLREAD;
	type.flags.dir = 1;
	CU_ASSERT_EQUAL(type.value, ET_LOCK | ET_EXCLREAD | 0x10);
}
