/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2011 Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEST
#include <grub/priority_queue.h>
#include <grub/mm.h>
#include <grub/dl.h>

GRUB_MOD_LICENSE ("GPLv3+");

#else
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <queue>

using namespace std;

typedef size_t grub_size_t;
typedef int (*grub_comparator_t) (const void *a, const void *b);
typedef unsigned char grub_uint8_t;
#define grub_malloc malloc
#define grub_memcpy memcpy
#define grub_realloc realloc
#define grub_free free

typedef enum
  {
    GRUB_ERR_NONE,
    grub_errno
  } grub_err_t;
#endif

struct grub_priority_queue
{
  grub_size_t elsize;
  grub_size_t allocated;
  grub_size_t used;
  grub_comparator_t cmp;
  void *els;
};

#ifdef TEST
typedef struct grub_priority_queue *grub_priority_queue_t;
#endif

static inline void *
element (struct grub_priority_queue *pq, grub_size_t k)
{
  return ((grub_uint8_t *) pq->els) + k * pq->elsize;
}

static inline void
swap (struct grub_priority_queue *pq, grub_size_t m, grub_size_t n)
{
  grub_uint8_t *p1, *p2;
  grub_size_t l;
  p1 = (grub_uint8_t *) element (pq, m);
  p2 = (grub_uint8_t *) element (pq, n);
  for (l = pq->elsize; l; l--, p1++, p2++)
    {
      grub_uint8_t t;
      t = *p1;
      *p1 = *p2;
      *p2 = t;
    }
}

static inline grub_size_t
parent (grub_size_t v)
{
  return (v - 1) / 2;
}

static inline grub_size_t
left_child (grub_size_t v)
{
  return 2 * v + 1;
}

static inline grub_size_t
right_child (grub_size_t v)
{
  return 2 * v + 2;
}

void *
grub_priority_queue_top (grub_priority_queue_t pq)
{
  if (!pq->used)
    return 0;
  return element (pq, 0);
}

void
grub_priority_queue_destroy (grub_priority_queue_t pq)
{
  grub_free (pq->els);
  grub_free (pq);
}

grub_priority_queue_t
grub_priority_queue_new (grub_size_t elsize,
			 grub_comparator_t cmp)
{
  struct grub_priority_queue *ret;
  void *els;
  els = grub_malloc (elsize * 8);
  if (!els)
    return 0;
  ret = (struct grub_priority_queue *) grub_malloc (sizeof (*ret));
  if (!ret)
    {
      grub_free (els);
      return 0;
    }
  ret->elsize = elsize;
  ret->allocated = 8;
  ret->used = 0;
  ret->cmp = cmp;
  ret->els = els;
  return ret;
}

/* Heap property: pq->cmp (element (pq, p), element (pq, parent (p))) <= 0. */
grub_err_t
grub_priority_queue_push (grub_priority_queue_t pq, const void *el)
{
  grub_size_t p;
  if (pq->used == pq->allocated)
    {
      void *els;
      els = grub_realloc (pq->els, pq->elsize * 2 * pq->allocated);
      if (!els)
	return grub_errno;
      pq->allocated *= 2;
      pq->els = els;
    }
  pq->used++;
  grub_memcpy (element (pq, pq->used - 1), el, pq->elsize);
  for (p = pq->used - 1; p; p = parent (p))
    {
      if (pq->cmp (element (pq, p), element (pq, parent (p))) <= 0)
	break;
      swap (pq, p, parent (p));
    }

  return GRUB_ERR_NONE;
}

void
grub_priority_queue_pop (grub_priority_queue_t pq)
{
  grub_size_t p;
  
  swap (pq, 0, pq->used - 1);
  pq->used--;
  for (p = 0; left_child (p) < pq->used; )
    {
      grub_size_t c;
      if (pq->cmp (element (pq, left_child (p)), element (pq, p)) <= 0
	  && (right_child (p) >= pq->used
	      || pq->cmp (element (pq, right_child (p)), element (pq, p)) <= 0))
	break;
      if (right_child (p) >= pq->used
	  || pq->cmp (element (pq, left_child (p)),
		      element (pq, right_child (p))) > 0)
	c = left_child (p);
      else
	c = right_child (p);
      swap (pq, p, c);
      p = c;
    }
}

#ifdef TEST

static int 
compar (const void *a_, const void *b_)
{
  int a = *(int *) a_;
  int b = *(int *) b_;
  if (a < b)
    return -1;
  if (a > b)
    return +1;
  return 0;
}

int
main (void)
{
  priority_queue <int> pq;
  grub_priority_queue_t pq2;
  int counter;
  int s = 0;
  pq2 = grub_priority_queue_new (sizeof (int), compar);
  if (!pq2)
    return 1;
  srand (1);

  for (counter = 0; counter < 1000000; counter++)
    {
      int op = rand () % 10;
      if (s && *(int *) grub_priority_queue_top (pq2) != pq.top ())
	{
	  printf ("Error at %d\n", counter);
	  return 2;
	}
      if (op < 3 && s)
	{
	  grub_priority_queue_pop (pq2);
	  pq.pop ();
	  s--;
	}
      else
	{
	  int v = rand ();
	  int e;
	  pq.push (v);
	  e = grub_priority_queue_push (pq2, &v);
	  if (e)
	    return 3;
	  s++;
	}
    }
  while (s)
    {
      if (*(int *) grub_priority_queue_top (pq2) != pq.top ())
	{
	  printf ("Error at the end. %d elements remaining.\n", s);
	  return 4;
	}
      grub_priority_queue_pop (pq2);
      pq.pop ();
      s--;
    }
  printf ("All tests passed successfully\n");
  return 0;
}
#endif
