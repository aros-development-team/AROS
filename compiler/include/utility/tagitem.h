#ifndef UTILITY_TAGITEM_H
#define UTILITY_TAGITEM_H

/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Tag-lists
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#include <stdarg.h>

typedef ULONG Tag;

struct TagItem
{
    Tag         ti_Tag __attribute__((aligned(sizeof(IPTR))));   /* Tag ID                */
    IPTR        ti_Data __attribute__((aligned(sizeof(IPTR))));  /* Tag-specific data     */
} __attribute__((packed));

#ifdef AROS_SLOWSTACKTAGS
__BEGIN_DECLS
    struct TagItem * GetTagsFromStack  (IPTR firstTag, va_list args);
    void             FreeTagsFromStack (struct TagItem * tags);
__END_DECLS
#endif

/* constants for Tag.ti_Tag, control tag values */
#define TAG_DONE   (0UL)   /* terminates array of TagItems. ti_Data unused */
#define TAG_END    (0UL)   /* synonym for TAG_DONE                         */
#define TAG_IGNORE (1UL)   /* ignore this item, not end of array           */
#define TAG_MORE   (2UL)   /* ti_Data is pointer to another array of TagItems
			     note that this tag terminates the current array */
#define TAG_SKIP   (3UL)   /* skip this and the next ti_Data items         */

/* What separates user tags from system tags */
#define TAG_USER    (1UL<<31)
#define TAG_OS	    (16UL)   /* The first tag used by the OS */

/* Tag-Offsets for the OS */
#define DOS_TAGBASE	    (TAG_OS)        /* Reserve 16k tags for DOS */
#define INTUITION_TAGBASE   (TAG_OS | 0x2000) /* Reserve 16k tags for Intuition */

/* Tag filter for FilterTagItems() */
#define TAGFILTER_AND 0 	/* exclude everything but filter hits	*/
#define TAGFILTER_NOT 1 	/* exclude only filter hits		*/

/* Mapping types for MapTags() */
#define MAP_REMOVE_NOT_FOUND 0	/* remove tags that aren't in mapList */
#define MAP_KEEP_NOT_FOUND   1	/* keep tags that aren't in mapList   */

/* Macro for syntactic sugar (and a little extra bug-resiliance) */
#define TAGLIST(args...) ((struct TagItem *)(IPTR []){ args, TAG_DONE })

/*
    Some macros to make it easier to write functions which operate on
    stacktags on every CPU/compiler/hardware.
*/
#ifndef AROS_TAGRETURNTYPE
#   define AROS_TAGRETURNTYPE IPTR
#endif

#ifdef AROS_SLOWSTACKTAGS
#	define AROS_NR_SLOWSTACKTAGS_PRE(arg)		\
	    va_list		  args;			\
	    struct TagItem	* tags;			\
							\
	    va_start (args, arg);			\
							\
	    if ((tags = GetTagsFromStack (arg, args)))	\
	    {

#	define AROS_SLOWSTACKTAGS_PRE_AS(arg, rettype)	\
	    rettype		  retval;		\
	    va_list		  args;			\
	    struct TagItem	* tags;			\
							\
	    va_start (args, arg);			\
							\
	    if ((tags = GetTagsFromStack (arg, args)))	\
	    {

#	define AROS_SLOWSTACKTAGS_PRE(arg)		\
	    AROS_SLOWSTACKTAGS_PRE_AS(arg, AROS_TAGRETURNTYPE)

#	define AROS_SLOWSTACKTAGS_ARG(arg) tags

#	define AROS_SLOWSTACKTAGS_POST			\
		FreeTagsFromStack (tags);		\
	    }						\
	    else					\
		retval = 0;				\
							\
	    va_end (args);				\
							\
	    return retval;
	    
#	define AROS_NR_SLOWSTACKTAGS_POST		\
		FreeTagsFromStack (tags);		\
	    }						\
							\
	    va_end (args);
#else
#	define AROS_NR_SLOWSTACKTAGS_PRE(arg)
#	define AROS_NR_SLOWSTACKTAGS_POST
#	define AROS_SLOWSTACKTAGS_PRE(arg) AROS_TAGRETURNTYPE retval;
#	define AROS_SLOWSTACKTAGS_PRE_AS(arg, rettype) rettype retval;
/*
 * In the following macro, we pretend to use a va_list, to stop GCC
 * discarding varargs given to inlined functions. If this stops working
 * in the future, we may have to enable slow stack tags for all archs.
 */
#	define AROS_SLOWSTACKTAGS_ARG(arg)		\
	    ((struct TagItem *)({			\
		va_list _dummy_args;			\
		va_start(_dummy_args, arg);		\
		va_end(_dummy_args);			\
		&(arg);					\
	    }))
#	define AROS_SLOWSTACKTAGS_POST     return retval;
#endif

#endif /* UTILITY_TAGITEM_H */
