/************************************************************
* MultiUser - MultiUser Task/File Support System				*
* ---------------------------------------------------------	*
* Monitoring																*
* ---------------------------------------------------------	*
* © Copyright 1993-1994 Geert Uytterhoeven						*
* All Rights Reserved.													*
************************************************************/

/*
 *      Private Function Prototypes
 */

extern void InitMonList(struct SecurityBase *secBase);
extern void CallMonitors(struct SecurityBase *secBase, ULONG triggerbit, UWORD from, UWORD to, char *userid);
extern void FreeRepliedMonMsg(struct SecurityBase *secBase);
