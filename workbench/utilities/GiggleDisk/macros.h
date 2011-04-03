#ifndef MACROS_H
#define MACROS_H 1

/*************************************************************************/

#include <string.h>

/*************************************************************************/

/*
** Exec List
*/

#define List_Init(q)  (q)->lh_Head = (struct Node *) &(q)->lh_Tail; (q)->lh_Tail = NULL; (q)->lh_TailPred = (struct Node *) &(q)->lh_Head;

/*
** endian
*/

#define swap16(x) ((((x)<<8) & 0xffff) | ((x)>>8))
#define swap32(x) (((x)<<24)|(((x) & 0xff00)<<8)|(((x)>>8) & 0xff00)|(((ULONG) (x))>>24))

/*
** CLib alias
*/

#define String_GetSize(a) (strlen(a)+1)
#define String_Copy(a,b)  strcpy( (char *)b, (const char *)a)
#define String_CopySize(a,b,s)  strncpy( (char *)b, (const char *)a, s )
#define String_Append(a,b) strcat( b,a )
#define String_AppendSize(a,b,s) strcat( b,a,s )
#define Memory_Clear( addr,size) memset(addr,0,size)

/*************************************************************************/

#endif /* MACROS_H */

