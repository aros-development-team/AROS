#define SEARCH_NEW 1
#define SEARCH_NEXT 2
#define SEARCH_PREV 3

/*******************************************************************************/

extern struct IntuitionBase *IntuitionBase;
extern struct GfxBase *GfxBase;

extern struct Screen *scr;
extern struct DrawInfo *dri;

extern ULONG gotomask, findmask;

/*******************************************************************************/

void CleanupRequesters(void);

void Make_Goto_Requester(void);
BOOL Handle_Goto_Requester(LONG *line);
void Kill_Goto_Requester(void);

void Make_Find_Requester(void);
WORD Handle_Find_Requester(char **text);
void Kill_Find_Requester(void);


