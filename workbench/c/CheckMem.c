#include <exec/memory.h>
#include <proto/exec.h>

int main(void)
{
    AvailMem(MEMF_CLEAR);
    
    return 0;
}
