/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: dos.library ScanVars() function test.
*/

#include <string.h>
#include <stdio.h>

#include <proto/dos.h>
#include <proto/utility.h>
#include <dos/var.h>

LONG print_var(struct Hook *hook, APTR userdata, struct ScanVarsMsg *message)
{
	printf("Hook called with userdata: %s\n", (char*) userdata);
    printf("%s=%.*s\n", message->sv_Name, message->sv_VarLen, message->sv_Var);
    return 0;
}

LONG print_var_break(struct Hook *hook, APTR userdata, struct ScanVarsMsg *message)
{
	printf("Hook called with userdata: %s\n", (char*) userdata);
    printf("%s=%.*s\n", message->sv_Name, message->sv_VarLen, message->sv_Var);
    if(!strncmp("var2", message->sv_Name, 4))
    {
    	printf("Scanned var2 variable!\n");
    	return 1;
    }
    return 0;
}

int main(void)
{
    struct Hook hook;
    char userdata[] = "Some user data...";
    LONG ret;
    
    memset(&hook, 0, sizeof(struct Hook));
    hook.h_Entry = print_var;

    printf("Scanning local variables:\n");
    ret = ScanVars(&hook, GVF_LOCAL_ONLY, userdata);
    printf("ScanVars returned %d\n", ret);

    printf("Adding some new local variables:\n");
    SetVar("var1","Value of variable 1", -1, GVF_LOCAL_ONLY);
    SetVar("var2","Value of variable 2", -1, GVF_LOCAL_ONLY);
    SetVar("var3","Value of variable 3", -1, GVF_LOCAL_ONLY);
    printf("Scanning local variables again:\n");
    ret = ScanVars(&hook, GVF_LOCAL_ONLY, userdata);
    printf("ScanVars returned %d\n", ret);

    printf("Trying to print all variables up to var2:\n");
    hook.h_Entry = print_var_break;
    ret = ScanVars(&hook, GVF_LOCAL_ONLY, userdata);
    printf("ScanVars returned %d\n", ret);
}
