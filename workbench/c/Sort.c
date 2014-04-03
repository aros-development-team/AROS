/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Sorts the contents of a file
    Lang: English
*/
/*****************************************************************************

    NAME

        Sort

    SYNOPSIS

        FROM/A,TO/A,COLSTART/K,CASE/S,NUMERIC/S

    LOCATION

        C:
	   
    FUNCTION

	Sorts the contents of a text file
	
    INPUTS

	FROM      -- file to read from
	TO        -- file to output to
	COLSTART  -- column at which the comparison begins
	CASE      -- sort is case sensitive. Uppercase items are output first
	NUMERIC   -- lines are interpreted as numbers

    RESULT

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <clib/macros.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <libraries/locale.h>
#include <utility/tagitem.h>

#define DEBUG 0
#include <aros/debug.h>

#include <string.h>

#define TEMPLATE "FROM/A,TO/A,COLSTART/K,CASE/S,NUMERIC/S"

#define ARG_FROM	0
#define ARG_TO		1
#define ARG_COLSTART	2
#define ARG_CASE	3
#define ARG_NUMERIC	4

#define ARG_NUM		5

const TEXT version[] = "$VER: Sort 41.2 (3.4.2014)";

struct sorted_data
{
  struct sorted_data * next;
  UBYTE              * data;
  ULONG                len; // length of line including '\n'.
};

struct Locale * locale;

int compare(struct sorted_data * sd1, 
            struct sorted_data * sd2, 
            ULONG col, 
            BOOL case_on)
{
  ULONG len = MIN(sd1->len, sd2->len);
 
  /* FIXME:  It seems like StrnCmp of locale does not work. */
#if 1
  LONG retval = 0;
  
  if (case_on)
  {
    int i = col;
    
    while (i < len)
    {
      BOOL a,b;
      a = IsUpper(locale,(ULONG)*(sd1->data+col+i));
      b = IsUpper(locale,(ULONG)*(sd2->data+col+i));
      
      if (a == b)
      {
        if (0 != (retval = StrnCmp(locale,
                           sd1->data+col,
                           sd2->data+col,
                           1,
                           SC_COLLATE2))); 
          break;
      }
      else
      {
       retval = b - a;
       break;
      }
      
      i++;
    }
  }
  else
  {

    retval=StrnCmp(locale,
                   sd1->data+col,
                   sd2->data+col,
                   len,
                   SC_COLLATE2);  

    if (0 == retval)
    {
      if (sd1->len < sd2->len)
        retval = -100;
      else
        retval = +100;
    }
  }
  
  return retval;

#else  
  int i = col;
  char * str1 = sd1->data;
  char * str2 = sd2->data;

  while (i < len)
  {
    if (str1[i] != str2[i])
      return (int)(str1[i] - str2[i]);

    i++;
  }
  
  return (int)sd1->len - (int)sd2->len;
#endif
} 


struct sorted_data * sort(UBYTE * data,
                          ULONG   data_len,
                          STRPTR  colstart,
                          BOOL    case_on,
                          BOOL    is_numeric)
{
  ULONG pos = 0;
  ULONG col = 0;
  struct sorted_data * first = NULL;
  struct sorted_data * cur   = NULL;
  struct sorted_data * tooshort = NULL;
  struct sorted_data * tooshort_last = NULL;
  
  if (colstart)
  {
    while (*colstart >= '0' && *colstart <= '9')
      col = col * 10 + *colstart++ - '0';
    if (col > 0)
      col-=1;
  }

  while (pos < data_len)
  {
    ULONG begin = pos;
    ULONG len;

    while (pos < data_len && data[pos] != 0x0a)
      pos++;

    if (data[pos] == 0x0a || pos == data_len)
      len = pos++ - begin;
    else
      len = ++pos - begin;

    cur = AllocMem(sizeof(struct sorted_data), MEMF_ANY|MEMF_CLEAR);
      
    if (cur)
    {
      cur->data = data + begin;
      cur->len  = len;
    
      if (len > col)
      {
        /*
        ** Insert it into the list of sorted lines 
        */
        if (NULL != first)
        {
          /*
          ** To be first in the list?
          */
          if (compare(cur, first, col, case_on) < 0)
          {
            cur->next = first;
            first     = cur;
          }
          else
          {
            struct sorted_data * _cur = first;

            while (1)
            {
              /*
              ** Insert it after the current one and before the
              ** next one?
              */
              if (NULL == _cur->next)
              {
                _cur->next = cur;
                break;
              }
              if (compare(cur, _cur->next, col, case_on) < 0)
              {
                cur -> next = _cur->next;
                _cur-> next = cur;
                break;
              }
       
              _cur = _cur->next;
            }
          }
        } /* if (NULL != first) */
        else
          first = cur;
      }
      else
      {
        /* this line is too short to sort it in */
        if (NULL == tooshort)
        {
          tooshort = cur;
          tooshort_last = cur;
        }
        else
        {
          tooshort_last->next = cur;
          tooshort_last = cur;
        }
      }
    } /* if (cur) */
      
  }

  if (NULL != tooshort)
  {
    tooshort_last->next = first;
    first = tooshort;
  }

  return first;
}


LONG write_data(struct sorted_data * start, BPTR file_out)
{
  BOOL write = TRUE;
  LONG error = 0;
  LONG count;
  
  while (start)
  {
    struct sorted_data * next = start->next;

    if (write)
    {
      count = Write(file_out, start->data, start->len);
      if (count != -1 && start->data[start->len-1] != 0x0a)
        count = Write(file_out, "\n", 1);
      if (count == -1)
      {
        error = IoErr();
        write = FALSE;
      }
    }

    FreeMem(start, sizeof(struct sorted_data));
    start = next;
  }

  return error;
}

int __nocommandline;

int main (void)
{
  IPTR args[ARG_NUM] = { (IPTR) NULL, (IPTR) NULL, (IPTR) NULL, FALSE, FALSE};
  struct RDArgs *rda;
  LONG result = RETURN_OK;
  LONG error = 0;

  locale = OpenLocale(NULL);
  if (!locale)
  {
    PutStr("Could not open locale!\n");
    return RETURN_FAIL;
  }

  rda = ReadArgs(TEMPLATE, args, NULL);
  if (rda)
  {
    BPTR lock_in;
    lock_in = Lock((STRPTR)args[ARG_FROM], ACCESS_READ);

    if (lock_in)
    {
       BPTR file_out = Open((STRPTR)args[ARG_TO], MODE_NEWFILE);
       if (file_out == BNULL)
         error = IoErr();
       struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);
       if (fib == NULL)
         error = IoErr();

       if (error == 0)
       {
          UBYTE * data = NULL;
          BOOL success = Examine(lock_in, fib);

          /*
          ** Read the input file into memory
          */
          if (fib->fib_Size && success)
            data = AllocVec(fib->fib_Size, MEMF_ANY);

          if (data)
          {
            BPTR in;
            if ((in = OpenFromLock(lock_in))) {
                ULONG read = Read(in, data, fib->fib_Size);

                if (-1 != read)
                {
                  struct sorted_data * sd;
                  sd = sort(data, 
                            fib->fib_Size, 
                            (STRPTR)args[ARG_COLSTART],
                            (BOOL)args[ARG_CASE],
                            (BOOL)args[ARG_NUMERIC]);

                  error = write_data(sd, file_out);
                }
                Close(in);
                lock_in = BNULL;
            }
            FreeVec(data);
          }
          else
            error = IoErr();

          Close(file_out);  
       }
       FreeDosObject(DOS_FIB, fib);
       if (lock_in)
           UnLock(lock_in);
    }
    else
      error = IoErr();
    FreeArgs(rda);
  }
  else
    error = IoErr();

  if (error)
  {
    PrintFault(error, "Sort");
    result = RETURN_FAIL;
  }

  return result;
}
