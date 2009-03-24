#include <libraries/codesets.h>
#include <proto/codesets.h>

#include <stdio.h>

int main()
{
    struct codeset *cs;
    
    if (!CodesetsBase) {
        printf("Autoopen failed!\n");
        return 1;
    }
    cs = CodesetsFindA(NULL, NULL);
    
    if (cs)
        printf("Default codeset for your system is %s\n", cs->name);
    else
        printf("Unable to query default codeset\n");
    return 0;
}
