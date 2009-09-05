/* list.h - header for grub list */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#ifndef GRUB_LIST_HEADER
#define GRUB_LIST_HEADER 1

#include <grub/symbol.h>
#include <grub/types.h>

struct grub_list
{
  struct grub_list *next;
};
typedef struct grub_list *grub_list_t;

typedef int (*grub_list_hook_t) (grub_list_t item);
typedef int (*grub_list_test_t) (grub_list_t new_item, grub_list_t item);

void EXPORT_FUNC(grub_list_push) (grub_list_t *head, grub_list_t item);
void * EXPORT_FUNC(grub_list_pop) (grub_list_t *head);
void EXPORT_FUNC(grub_list_remove) (grub_list_t *head, grub_list_t item);
int EXPORT_FUNC(grub_list_iterate) (grub_list_t head, grub_list_hook_t hook);
void EXPORT_FUNC(grub_list_insert) (grub_list_t *head, grub_list_t item,
				    grub_list_test_t test);

/* This function doesn't exist, so if assertion is false for some reason, the
   linker would fail.  */
#ifdef APPLE_CC
/* This approach fails with Apple's gcc. Use grub_abort.  */
#include <grub/misc.h>
static inline void *
grub_assert_fail (void)
{
	grub_abort ();
	return 0;
}
#else
extern void* grub_assert_fail (void);
#endif

#define GRUB_FIELD_MATCH(ptr, type, field) \
  ((char *) &(ptr)->field == (char *) &((type) (ptr))->field)

#define GRUB_AS_LIST(ptr) \
  (GRUB_FIELD_MATCH (ptr, grub_list_t, next) ? \
   (grub_list_t) ptr : grub_assert_fail ())

#define GRUB_AS_LIST_P(pptr) \
  (GRUB_FIELD_MATCH (*pptr, grub_list_t, next) ? \
   (grub_list_t *) (void *) pptr : grub_assert_fail ())

struct grub_named_list
{
  struct grub_named_list *next;
  const char *name;
};
typedef struct grub_named_list *grub_named_list_t;

void * EXPORT_FUNC(grub_named_list_find) (grub_named_list_t head,
					  const char *name);

#define GRUB_AS_NAMED_LIST(ptr) \
  ((GRUB_FIELD_MATCH (ptr, grub_named_list_t, next) && \
    GRUB_FIELD_MATCH (ptr, grub_named_list_t, name))? \
   (grub_named_list_t) ptr : grub_assert_fail ())

#define GRUB_AS_NAMED_LIST_P(pptr) \
  ((GRUB_FIELD_MATCH (*pptr, grub_named_list_t, next) && \
    GRUB_FIELD_MATCH (*pptr, grub_named_list_t, name))? \
   (grub_named_list_t *) (void *) pptr : grub_assert_fail ())

#define GRUB_PRIO_LIST_PRIO_MASK	0xff
#define GRUB_PRIO_LIST_FLAG_ACTIVE	0x100

struct grub_prio_list
{
  struct grub_prio_list *next;
  const char *name;
  int prio;
};
typedef struct grub_prio_list *grub_prio_list_t;

void EXPORT_FUNC(grub_prio_list_insert) (grub_prio_list_t *head,
					 grub_prio_list_t item);

static inline void
grub_prio_list_remove (grub_prio_list_t *head, grub_prio_list_t item)
{
  if ((item->prio & GRUB_PRIO_LIST_FLAG_ACTIVE) && (item->next))
    item->next->prio |= GRUB_PRIO_LIST_FLAG_ACTIVE;
  grub_list_remove (GRUB_AS_LIST_P (head), GRUB_AS_LIST (item));
}

#define GRUB_AS_PRIO_LIST(ptr) \
  ((GRUB_FIELD_MATCH (ptr, grub_prio_list_t, next) && \
    GRUB_FIELD_MATCH (ptr, grub_prio_list_t, name) && \
    GRUB_FIELD_MATCH (ptr, grub_prio_list_t, prio))? \
   (grub_prio_list_t) ptr : grub_assert_fail ())

#define GRUB_AS_PRIO_LIST_P(pptr) \
  ((GRUB_FIELD_MATCH (*pptr, grub_prio_list_t, next) && \
    GRUB_FIELD_MATCH (*pptr, grub_prio_list_t, name) && \
    GRUB_FIELD_MATCH (*pptr, grub_prio_list_t, prio))? \
   (grub_prio_list_t *) (void *) pptr : grub_assert_fail ())

#endif /* ! GRUB_LIST_HEADER */
