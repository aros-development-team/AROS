#include <string.h>
#include <stdio.h>
#include <proto/dos.h>

int main(void)
{
       char* var="abc";
       char* val="cde";

       char buffer[10];
       LONG len;

       printf("test setvar '%s'\n", val);
       if (SetVar(var, val, strlen(val), 0) == DOSFALSE)
               printf("error setvar\n");
       if ((len = GetVar(var, buffer, sizeof(buffer), 0)) < 0)
               printf("error getvar %d\n", len);
       printf("test getvar '%s'\n", buffer);

       val="";
       printf("test1 setvar '%s'\n", val);
       if (SetVar(var, val, strlen(val), 0) == DOSFALSE)
               printf("error setvar\n");
       if ((len = GetVar(var, buffer, sizeof(buffer), 0)) < 0)
               printf("error getvar %d\n", len);
       printf("test1 getvar '%s'\n", buffer);

       val="abc";
       printf("test2 setvar '%s'\n", val);
       if (SetVar(var, val, strlen(val), 0) == DOSFALSE)
               printf("error setvar\n");
       if ((len = GetVar(var, buffer, sizeof(buffer), 0)) < 0)
               printf("error getvar %d\n", len);
       printf("test2 getvar '%s'\n", buffer);

       val="";
       printf("test3 setvar '%s'\n", val);
       if (SetVar(var, val, strlen(val), 0) == DOSFALSE)
               printf("error setvar\n");
       if ((len = GetVar(var, buffer, sizeof(buffer), 0)) < 0)
               printf("error getvar %d\n", len);
       printf("test3 getvar '%s'\n", buffer);

       val="";
       printf("test4 setvar '%s'\n", val);
       if (SetVar(var, val, strlen(val), 0) == DOSFALSE)
               printf("error setvar\n");
       if ((len = GetVar(var, buffer, sizeof(buffer), 0)) < 0)
               printf("error getvar %d\n", len);
       printf("test4 getvar '%s'\n", buffer);

       return 0;
}
