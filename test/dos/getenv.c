#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    char c;
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s <varname>\n", argv[0]);
        return 20;
    }
	
    if (GetVar(argv[1], &c, 1, GVF_BINARY_VAR) == 0)
    {
        LONG len = IoErr();
	char *buf = malloc(len + 1);
	if (!buf)
	{
	    PrintFault(ERROR_NO_FREE_STORE, argv[0]);
	    return 20;
	}
	
	printf("IoErr() says the len of the value of the var '%s' is: %ld\n", argv[1], len);
	
	
	len = GetVar(argv[1], buf, len+1, GVF_BINARY_VAR);
	
	printf("GetVar() says the len of the value of the var '%s' is: %ld - its value is '%s'\n",
	       argv[1], len, buf);
	
	free(buf);
	
	return 0;
    }

    PrintFault(IoErr(), argv[1]);    
    
    return 20;
}
