
struct MinNode *	Node_Next(APTR node);
struct MinNode *	Node_Prev(APTR node);
struct MinNode *	List_First(APTR list);
struct MinNode *	List_Last(APTR list);
ULONG 			List_Length(APTR list);
struct MinNode *	List_Find(APTR list, ULONG num);
STRPTR 			StrCopy( const STRPTR str );
STRPTR 			StrCopyPool( APTR pool, const STRPTR str );
STRPTR 			StrNCopyPool( APTR pool, const STRPTR str, LONG len);
LONG 			GetFileSize( BPTR fileh );
struct IFFHandle *	PrepareClipboard(void);
VOID 			FreeClipboard(struct IFFHandle *iff);

struct Region *installclipregion (struct Layer *l, struct Region *r);
