#include <proto/locale.h>
#include <proto/exec.h>
#include <utility/hooks.h>
#include <libraries/locale.h>
#include <dos/datetime.h>
#include <stdio.h>



  struct Library * LocaleBase;

void printchar(struct Hook * myhook,
               char c,
               struct Locale * locale)
{
  if ('\0' != c)
   printf("%c",c);
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

void formatstring_test(void)
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

void formatdate_test(void)
{
  struct Hook myhook;
  struct DateStamp date;

  myhook.h_Entry = (APTR)printchar;

  printf("Doing a simple FormatDate test!\n");  

  /*
  ** Just a simple test for FormatString
  */
  
  date.ds_Minute  = 59 + 60*8; // 8:59
  date.ds_Tick    = 50*30; // 30 seconds
  
  printf("\n\tNow: 8:59:30\n");
  FormatDate(NULL, "24hour style (leading 0s): %H\n", &date, &myhook);
  FormatDate(NULL, "12hour style (leading 0s): %I\n", &date, &myhook);
  FormatDate(NULL, "24hour style: %q\n", &date, &myhook);
  FormatDate(NULL, "12hour style: %Q\n", &date, &myhook);
  FormatDate(NULL, "Number of seconds (leading 0s): %S\n", &date, &myhook);
  FormatDate(NULL, "H:M:S style (T): %T\n", &date, &myhook);
  FormatDate(NULL, "H:M:S style (X): %X\n", &date, &myhook);


  date.ds_Minute  = 59 + 60*18; // 18:59
  date.ds_Tick    = 50*30; // 30 seconds
  
  printf("\n\tNow: 18:59:30\n");
  FormatDate(NULL, "24hour style (leading 0s): %H\n", &date, &myhook);
  FormatDate(NULL, "12hour style (leading 0s): %I\n", &date, &myhook);
  FormatDate(NULL, "24hour style: %q\n", &date, &myhook);
  FormatDate(NULL, "12hour style: %Q\n", &date, &myhook);
  FormatDate(NULL, "Number of seconds (leading 0s): %S\n", &date, &myhook);
  FormatDate(NULL, "H:M:S style (T): %T\n", &date, &myhook);
  FormatDate(NULL, "H:M:S style (X): %X\n", &date, &myhook);
}



int main(void)
{

  printf("Opening local.library\n");
  LocaleBase = OpenLibrary("locale.library",0);
  
  
  if (LocaleBase)
  {
    formatstring_test();
  
    formatdate_test();
    CloseLibrary(LocaleBase);
  }
  else
  {
    printf("Locale.library could not be opened!\n");
  }
  return 0;
}