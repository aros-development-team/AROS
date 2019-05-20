/************************************************************
* MultiUser - MultiUser Task/File Support System				*
* ---------------------------------------------------------	*
* Segment Management														*
* ---------------------------------------------------------	*
* © Copyright 1993-1994 Geert Uytterhoeven						*
* All Rights Reserved.													*
************************************************************/

/*
 *		Private Segment Node
 */

struct secSegNode {
	struct MinNode Node;
	BPTR SegList;
	struct secExtOwner Owner;		/* Only uid/gid, no secondary gids */
};


/*
*		Private Function Prototypes
*/

extern void InitSegList(struct SecurityBase *secBase);
