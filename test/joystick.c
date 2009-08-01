#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/lowlevel.h>

#include <stdio.h>
#include <stdlib.h>


struct Library *LowLevelBase;

static void printbin(ULONG val)
{
    int i;
    
    for(i = 31; i >= 0; i--)
    {
    	printf("%d", (val & (1 << i)) ? 1 : 0);
    }    
    printf("\n");
}

int main(int argc, char **argv)
{
    int unit = 1;
    
    if (argc == 2) unit = atoi(argv[1]);
    
    LowLevelBase = OpenLibrary("lowlevel.library", 0);
    if (LowLevelBase)
    {
    	ULONG old = 0;
	
    	while(!CheckSignal(SIGBREAKF_CTRL_C))
	{
	    ULONG new;
	    
	    new = ReadJoyPort(unit);
	    if (new != old)
	    {
	    	old = new;
		
		printbin(new);
	    }
	    
	    Delay(1);
	}
    	CloseLibrary(LowLevelBase);
    }
    
    return 0;
}
