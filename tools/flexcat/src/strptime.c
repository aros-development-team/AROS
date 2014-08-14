/*
 * $Id$
 *
 * Copyright (C) 1993-1999 by Jochen Wiedmann and Marcin Orlowski
 * Copyright (C) 2002-2014 by the FlexCat Open Source Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <string.h>
#include <time.h>
#include <stdio.h>

/// strptime
// parse a date string produced by strftime() and put the success in a struct tm
enum ScanDateState
{
  SDS_DEFAULT = 0,
  SDS_SPECIFIER,
  SDS_DONE,
  SDS_SECOND,
  SDS_MINUTE,
  SDS_HOUR,
  SDS_DAY_OF_MONTH,
  SDS_MONTH,
  SDS_YEAR,
  SDS_DAY_OF_WEEK,
  SDS_DAY_YEAR,
  SDS_IS_DST,
};

#define FLG_SEC         (1<<0)
#define FLG_MIN         (1<<1)
#define FLG_HOUR        (1<<2)
#define FLG_MDAY        (1<<3)
#define FLG_MON         (1<<4)
#define FLG_YEAR        (1<<5)
#define FLG_WDAY        (1<<6)
#define FLG_YDAY        (1<<7)
#define FLG_ISDST       (1<<8)
#define FLG_4DIGIT_YEAR (1<<9)

char *strptime(const char *string, const char *fmt, struct tm *res)
{
  int success = 1;
  char fc;
  char sc;
  enum ScanDateState state = SDS_DEFAULT;
  int flags = 0;

  // start with the first character in both strings
  fc = *fmt++;
  sc = *string++;

  while(state != SDS_DONE)
  {
    if(fc == '\0' && sc == '\0')
      state = SDS_DONE;

    switch(state)
    {
      case SDS_DEFAULT:
      {
        if(fc == '%')
        {
          state = SDS_SPECIFIER;
          fc = *fmt++;
        }
        else
        {
          // the format string seems to be malformed, bail out
          state = SDS_DONE;
        }
      }
      break;

      case SDS_SPECIFIER:
      {
        switch(fc)
        {
          case 'd': // %d  - day number with leading zeros (01-31)
          case 'e': // %e  - day number with leading spaces ( 1-31)
          {
            flags |= FLG_MDAY;
            state = SDS_DAY_OF_MONTH;
            fc = *fmt++;
          }
          break;

          case 'm': // %m  - month number with leading zeros (01-12)
          {
            flags |= FLG_MON;
            state = SDS_MONTH;
            fc = *fmt++;
          }
          break;

          case 'Y': // %Y  - year using four digits with leading zeros
          {
            flags |= FLG_4DIGIT_YEAR;
          }
          // we fall through here

          case 'y': // %y  - year using two digits with leading zeros (00-99)
          {
            flags |= FLG_YEAR;
            state = SDS_YEAR;
            fc = *fmt++;
          }
          break;

          case '-':
          {
            // ignore any switches between with/without leading zeros/spaces
            fc = *fmt++;
          }
          break;

          default:
          {
            // unknown specifier, bail out
            state = SDS_DONE;
          }
          break;
        }
      }
      break;

      case SDS_DAY_OF_MONTH:
      {
        if(sc == fc)
        {
          // next separator in format string found
          state = SDS_DEFAULT;
          fc = *fmt++;
          sc = *string++;
        }
        else if(sc == ' ')
        {
          // ignore any spaces within the day spec
          sc = *string++;
        }
        else if(sc >= '0' && sc <= '9')
        {
          // valid number found, add it to the day of month
          res->tm_mday = res->tm_mday * 10 + sc - '0';
          sc = *string++;
        }
        else
        {
          // unexpected character, bail out
          state = SDS_DONE;
        }
      }
      break;

      case SDS_MONTH:
      {
        if(sc == fc)
        {
          // next separator in format string found
          state = SDS_DEFAULT;
          fc = *fmt++;
          sc = *string++;
        }
        else if(sc >= '0' && sc <= '9')
        {
          // valid number found, add it to the month
          res->tm_mon = res->tm_mon * 10 + sc - '0';
          sc = *string++;
        }
        else
        {
          // unexpected character, bail out
          state = SDS_DONE;
        }
      }
      break;

      case SDS_YEAR:
      {
        if(sc == fc)
        {
          // next separator in format string found
          state = SDS_DEFAULT;
          fc = *fmt++;
          sc = *string++;
        }
        else if(sc >= '0' && sc <= '9')
        {
          // valid number found, add it to the year
          res->tm_year = res->tm_year * 10 + sc - '0';
          sc = *string++;
        }
        else
        {
          // unexpected character, bail out
          state = SDS_DONE;
        }
      }
      break;

      default:
        // nothing to do
      break;
    }
  }

  // finally check if the calculated values are correct, but only those which
  // were specified in the format string
  if((flags & FLG_MDAY) || strstr(fmt, "%d") != NULL || strstr(fmt, "%-d") != NULL || strstr(fmt, "%e") != NULL)
  {
    if(res->tm_mday >= 1 && res->tm_mday <= 31)
    {
      // nothing to adjust
    }
    else
    {
      success = 0;
    }
  }
  if((flags & FLG_MON) || strstr(fmt, "%m") != NULL || strstr(fmt, "%-m") != NULL)
  {
    if(res->tm_mon >= 1 && res->tm_mon <= 12)
    {
      // tm_mon counts from 0 to 11
      res->tm_mon--;
    }
    else
    {
      success = 0;
    }
  }
  if((flags & FLG_YEAR) || strstr(fmt, "%y") != NULL || strstr(fmt, "%-y") != NULL || strstr(fmt, "%Y") != NULL || strstr(fmt, "%-Y") != NULL)
  {
    if((flags & FLG_4DIGIT_YEAR) || strstr(fmt, "%Y") != NULL || strstr(fmt, "%-Y") != NULL)
    {
      if(res->tm_year >= 1900)
      {
        // tm_year counts the years from 1900
        res->tm_year -= 1900;
      }
      else
      {
        // year numbers less than 1900 are not supported
        success = 0;
      }
    }
    else
    {
      // 2 digit year number, must be less than 100
      if(res->tm_year < 100)
      {
        if(res->tm_year < 40)
        {
          // tm_year counts the years from 1900
          // if the year number is less than 40 we assume a year between
          // 2000 and 2039 instead of between 1900 and 1939 to allow a user
          // age of at least ~70 years.
          res->tm_year += 100;
        }
      }
      // Although we expect a two digit year number for %y we got one with more digits.
      // Better not fail at this even if the entered string is wrong. People tend to
      // forget the correct formatting.
      else if(res->tm_year >= 1900)
      {
        // tm_year counts the years from 1900
        res->tm_year -= 1900;
      }
      else
      {
        // numbers between 100 and 1899 are definitely not allowed
        success = 0;
      }
    }
  }

  // finally check if the day value is correct
  if(success == 1 && (flags & FLG_MDAY))
  {
    if(res->tm_mon == 1)
    {
      // February has 29 days at most, but we don't check for leap years here
      if(res->tm_mday > 29)
      {
        success = 0;
      }
    }
    else if(res->tm_mon ==  3 ||
            res->tm_mon ==  5 ||
            res->tm_mon ==  8 ||
            res->tm_mon == 10)
    {
      // April, June, September and November have 30 days
      if(res->tm_mday > 30)
      {
        success = 0;
      }
    }
  }

  return (char *)string;
}

///
