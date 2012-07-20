
#include <exec/types.h>
#include <proto/dummy_rel.h>

LONG DummyPrint4(LONG a, LONG b, LONG c, LONG d)
{
    return printx(4, a, b, c, d);
}
