#include <proto/locale.h>
#include <proto/exec.h>
#include <utility/hooks.h>
#include <libraries/locale.h>
#include <stdio.h>


  struct Library * LocaleBase;

void printchar(struct Hook * myhook,
               char c,
               struct Locale * locale)
{
  if ('\0' != c)
   printf("%c",c);
  else
   printf("(NUL)");  
}

struct Data
{
  char * a;
  char * b;
  char * c;
  char * d;
  UWORD  e;
  UWORD  f;
  ULONG  g;
  
};

void locale_test(void)
{
  struct Hook myhook;
  char hello[]      = {"Hello"};
  char locale[]     = {"Locale"};
  char welcomes[]   = {"welcomes"};
  char you[]        = {"you"};
  
  struct Data dataStream = 
  {  hello,
     locale,
     welcomes,
     you,
     1000,
     20000,
     0x1234ABCD};
  
  myhook.h_Entry = (APTR)printchar;

  printf("Doing a simple FormatString test!\n");  

  /*
  ** Just a simple test for FormatString
  */
  FormatString(NULL, "%s! %s %s %s %u %u %lx!\n", &dataStream, &myhook);
  FormatString(NULL, "%4$s! (%5$u %7$lX %6$u %6$U %7$lU)  %1$.3s %2$s %3$s %4$s!\n", &dataStream, &myhook);
}

int main(void)
{

  printf("Opening local.library\n");
  LocaleBase = OpenLibrary("locale.library",0);
  
  
  if (LocaleBase)
  {
    locale_test();
  
    CloseLibrary(LocaleBase);
  }
  else
  {
    printf("Locale.library could not be opened!\n");
  }
  return 0;
}