#include <proto/locale.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <utility/hooks.h>
#include <libraries/locale.h>
#include <dos/datetime.h>
#include <utility/date.h>
#include <aros/asmcall.h>
#include <stdio.h>



struct LocaleBase * LocaleBase;
struct UtilityBase * UtilityBase;

AROS_UFH3(void, printchar,
    AROS_UFHA(struct Hook *, myhook, A0),
    AROS_UFHA(struct Locale *, locale, A2),
    AROS_UFHA(char, c, A1))
{
    AROS_USERFUNC_INIT

    if ('\0' != c) printf("%c",c);

    AROS_USERFUNC_EXIT
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

void formatstring_test(struct Locale * locale)
{
  struct Hook myhook;
  char hello[]      = {"Hello"};
  char local[]      = {"Locale"};
  char welcomes[]   = {"welcomes"};
  char you[]        = {"you"};
  
  struct Data dataStream = 
  {  hello,
     local,
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
  FormatString(locale, "%s! %s %s %s %u %u %lx!\n", &dataStream, &myhook);
  FormatString(locale, "%4$s! (%5$u %7$lX %6$u %6$U %7$lU)  %1$.3s %2$20s %3$s %4$s!\n", &dataStream, &myhook);
}

void formatdate_test(struct Locale * locale)
{
  struct Hook myhook;
  struct DateStamp date;
  struct ClockData cdata;
  ULONG seconds;

  myhook.h_Entry = (APTR)printchar;

  printf("Doing a simple FormatDate test!\n");  

  /*
  ** Just a simple test for FormatString
  */
  
  cdata.sec   = 30;
  cdata.min   = 59;
  cdata.hour  = 8;
  cdata.mday  = 17;
  cdata.month = 1;
  cdata.year  = 2000;
  cdata.wday  = 0; // don't care
  
  seconds = Date2Amiga(&cdata);
  date.ds_Days = seconds / 86400;
  seconds = seconds % 86400;
  date.ds_Minute = seconds / 60;
  seconds = seconds % 60;
  date.ds_Tick = seconds * 50;
  
  FormatDate(locale, "24hour style (leading 0s): %H\n", &date, &myhook);
  FormatDate(locale, "12hour style (leading 0s): %I\n", &date, &myhook);
  FormatDate(locale, "24hour style: %q\n", &date, &myhook);
  FormatDate(locale, "12hour style: %Q\n", &date, &myhook);
  FormatDate(locale, "Number of seconds (leading 0s): %S\n", &date, &myhook);
  FormatDate(locale, "H:M:S style (T): %T\n", &date, &myhook);
  FormatDate(locale, "H:M:S style (X): %X\n", &date, &myhook);


  cdata.sec   = 30;
  cdata.min   = 59;
  cdata.hour  = 18;
  cdata.mday  = 30;
  cdata.month = 7;
  cdata.year  = 2000;
  cdata.wday  = 0; // don't care
  
  seconds = Date2Amiga(&cdata);
  date.ds_Days = seconds / 86400;
  seconds = seconds % 86400;
  date.ds_Minute = seconds / 60;
  seconds = seconds % 60;
  date.ds_Tick = seconds * 50;
  
  FormatDate(locale, "24hour style (leading 0s): %H\n", &date, &myhook);
  FormatDate(locale, "12hour style (leading 0s): %I\n", &date, &myhook);
  FormatDate(locale, "24hour style: %q\n", &date, &myhook);
  FormatDate(locale, "12hour style: %Q\n", &date, &myhook);
  FormatDate(locale, "Number of seconds (leading 0s): %S\n", &date, &myhook);
  FormatDate(locale, "%%H:%%M:%%S style (T): %T\n", &date, &myhook);
  FormatDate(locale, "%%H:%%M:%%S style (X): %X\n", &date, &myhook);

  printf("\n");  
  FormatDate(locale, "%%a %%b %%d %%h:%%m:%%s %%y style: %c\n", &date, &myhook);
  FormatDate(locale, "%%a %%b %%e %%T %%Z %%Y style: %C\n", &date, &myhook);
  FormatDate(locale, "%%m/%%d/%%y style: %D\n", &date, &myhook);

  FormatDate(locale, "Week number - Sunday first day of week: %U\n", &date, &myhook);
  FormatDate(locale, "Week number - Monday first day of week: %W\n", &date, &myhook);
  
}


void getstringtest(struct Locale * locale)
{
  struct Catalog * cat;
  char catname[256];
  
  printf("Please enter name of catalog: ");
  scanf("%s",catname);
   
  cat = OpenCatalogA(locale,catname,NULL);
  if (cat)
  {
    int i = 0;
    while (i < 65535)
    { 
      CONST_STRPTR str = GetCatalogStr(cat, i, NULL);
      if (str)
        printf("ID: %d - string : %s\n",i,str);

      i++;
    }
  }
  CloseCatalog(cat);
}


int main(void)
{
  LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library",0);
  UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library",0);  
  
  if (LocaleBase)
  {
    struct Locale * locale;
    locale = OpenLocale(NULL); 
    formatstring_test(locale);
    formatdate_test(locale);
    getstringtest(locale);
    
    if (locale) CloseLocale(locale);
  }
  else
  {
    printf("A library could not be opened!\n");
  }

  if (LocaleBase) CloseLibrary((struct Library *)LocaleBase);
  if (UtilityBase) CloseLibrary((struct Library *)UtilityBase);
  return 0;
}
