#ifndef UTILITY_TAGITEM_H
#define UTILITY_TAGITEM_H

/*
    (C) 1995-96 AROS - The Amiga Replacement OS

    $Id$

    Desc: Tag-lists
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

typedef STACKULONG Tag;

struct TagItem
{
    Tag       ti_Tag;  /* What is this ? */
    STACKIPTR ti_Data; /* Tag-specific data */
};

/* constants for Tag.ti_Tag, control tag values */
#define TAG_DONE   (0L)   /* terminates array of TagItems. ti_Data unused */
#define TAG_END    (0L)   /* synonym for TAG_DONE                         */
#define TAG_IGNORE (1L)   /* ignore this item, not end of array           */
#define TAG_MORE   (2L)   /* ti_Data is pointer to another array of TagItems
			     note that this tag terminates the current array */
#define TAG_SKIP   (3L)   /* skip this and the next ti_Data items         */

/* What separates user tags from system tags */
#define TAG_USER    ((ULONG)(1L<<31))
#define TAG_OS	    (16L)   /* The first tag used by the OS */

/* Tag-Offsets for the OS */
#define DOS_TAGBASE	    (TAG_OS)        /* Reserve 16k tags for DOS */
#define INTUITION_TAGBASE   (TAG_OS | 0x2000) /* Reserve 16k tags for Intuition */

/* Tag filter for FilterTagItems() */
#define TAGFILTER_AND 0 	/* exclude everything but filter hits	*/
#define TAGFILTER_NOT 1 	/* exclude only filter hits		*/

/* Mapping types for MapTags() */
#define MAP_REMOVE_NOT_FOUND 0	/* remove tags that aren't in mapList */
#define MAP_KEEP_NOT_FOUND   1	/* keep tags that aren't in mapList   */

#endif /* UTILITY_TAGITEM_H */
