/* The main purpose is to test alert address detection and stack trace */

#include <exec/alerts.h>
#include <proto/exec.h>

#include <string.h>

int main(int argc, char **argv)
{
    ULONG n = AN_InitAPtr;

    if ((argc > 1) && !stricmp(argv[1], "deadend"))
	n = AN_BogusExcpt;

    Alert(n);
    
    return 0;
}
