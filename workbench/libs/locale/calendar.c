#include <exec/types.h>
#include <dos/datetime.h>


#warning A few missing calendar functions.

/*
** DateStamp is relative to Jan. 1, 1978.
** Jan.1, 1978, was a Sunday.
*/

/*
** Which day of the month. Like Jan. X, 1978
*/
ULONG calendar_day(struct DateStamp * date)
{
  return 0;
}

/*
** Which month of the year. 0 = January
*/
ULONG calendar_month(struct DateStamp * date)
{
  return 0;
}

/*
** Which week of the year. Week starts with Sunday
*/
ULONG calendar_week(struct DateStamp *  date)
{
  return 0;
}

/*
** Which week of the year. Week starts with Monday
*/
ULONG calendar_weekmonday(struct DateStamp * date)
{
  return 0;
}

/*
** Which day of the week. 0 = Sunday
*/
ULONG calendar_weekday(struct DateStamp * date)
{
  return (date->ds_Days % 7);
}

/*
** Which year. Y2k ready!!
*/
ULONG calendar_year(struct DateStamp * date)
{
  return 0;
}