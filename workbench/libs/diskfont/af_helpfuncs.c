/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Misc helpfuncs needed in AvailFonts()
    Lang: English.
*/


#include "diskfont_intern.h"



/****************************/
/* AllocFontNameNode        */
/****************************/

struct FontNameNode *AllocFontNameNode(STRPTR fontname, struct DiskfontBase_intern *DiskfontBase)
{
    
    struct FontNameNode *node;
  

    /* Allocate memory for the node */
    if 
    (
        node = AllocVec
        (
            sizeof(struct FontNameNode),
            MEMF_ANY
        )
          
    )
    {
        /* Allocate memory for the fontname itself */
        if 
        (
            node->FontName = AllocVec
            (
                strlen(fontname) + 1, /* + 1 because of null-termination */
                MEMF_ANY
            )
        )
        {
            /* Copy the fontname into the allocated memory area */
            strcpy( node->FontName, fontname );
      
            /* Successfull. Return pointer to the node. */
            return (node);
        }
  
        FreeVec(node);
    }
    return (FALSE);
}

/**********************/
/* AllocFontTagsNode  */
/**********************/

struct FontTagsNode *AllocFontTagsNode( struct TagItem *taglist, struct DiskfontBase_intern *DiskfontBase )
{
  
    struct FontTagsNode *node;
  
    /* Allocate a new fonttags node */
  
    if 
    (
        node = AllocVec
        (
            sizeof(struct FontTagsNode),
            MEMF_ANY
        )
    )
    {
        /* Clone the font tagitems and save pointer to the clones */
        if 
        (
            node->TagList = CloneTagItems( taglist )
        )
        {
            /* Success */
            return (node);
      
        }
    
        FreeVec(node);
    }
    return (FALSE);
}

/**************/
/* NumTags    */
/**************/

ULONG NumTags(struct TagItem *taglist, struct DiskfontBase_intern *DiskfontBase)
/* Counts the number of tags in at taglist including TAG_DONE */

{
  
    ULONG numtags = 0;
  
    for (; NextTagItem(&taglist); )
        numtags ++;

    numtags ++; /* Count TAG_DONE */
  
    return numtags;
}
/****************/
/* CopyTagItems */
/****************/

ULONG CopyTagItems
(
	struct TagItem *desttaglist, 
	struct TagItem* sourcetaglist, 
	struct DiskfontBase_intern *DiskfontBase
)
/* Copies tags from a taglist to another memory location, returning
  the number of tags copied */

{
    ULONG numtags=0;
  
    struct TagItem *tag;
  
    for (; tag = NextTagItem(&sourcetaglist); )
    {
        desttaglist->ti_Tag   = tag->ti_Tag;
        desttaglist->ti_Data  = tag->ti_Data;
    
        desttaglist++;
        numtags++;
    }

    /* Count TAG_DONE */
    desttaglist++;
    numtags++;
    
    /* Insert TAG_DONE */
    desttaglist->ti_Tag   = TAG_DONE;
    desttaglist->ti_Data  = 0L;
  
    return (numtags);
}


  

/**********************/
/* AF_FreeLists       */
/**********************/

VOID FreeAFLists(struct AF_Lists *lists, struct DiskfontBase_intern *DiskfontBase)
{
    
    /* Frees the nodes in the lists found inside the AF_Lists structures
    + the AF_Lists structure itself */
  
    struct MinNode *node, *nextnode;
  
    /* Free the fontinfolist */
  
    node = lists->FontInfoList.mlh_Head;
  
    while (nextnode = ((struct FontInfoNode*)node)->NodeHeader.mln_Succ)
    {
        FreeMem
        (
            node,
            sizeof (struct FontInfoNode)
        );
        node = nextnode;
    }
      
    /* Free fontnamelist */
    node = lists->FontNameList.mlh_Head;
  
    while (nextnode = ((struct FontNameNode*)node)->NodeHeader.mln_Succ)
    {
        FreeVec( ((struct FontNameNode*)node)->FontName ); 
        FreeMem
        (
            node,
            sizeof (struct FontNameNode)
        );
            node = nextnode;
    }
    /* Free fonttagslist */
  
    node=lists->FontTagsList.mlh_Head;
  
    while (nextnode = ((struct FontTagsNode*)node)->NodeHeader.mln_Succ)
    {
        /* Allocated with CloneTagItems() */
        FreeTagItems( ((struct FontTagsNode*)node)->TagList );
        FreeMem
        (
            node,
            sizeof (struct FontTagsNode)
        );
    
        node = nextnode;
    }
  
  return;
  }

