#ifndef _INLINE_ALIB_H
#define _INLINE_ALIB_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#define BeginIO(ioReq) \
({									\
	struct IORequest *_BeginIO_request = (ioReq);			\
	LP1NR(0x1e, BeginIO, struct IORequest *, _BeginIO_request, a1,	\
	, _BeginIO_request->io_Device);					\
})

#define NewList(list) \
({									\
  struct List *_NewList_list = (list);					\
  _NewList_list->lh_TailPred = (struct Node *)_NewList_list;		\
  _NewList_list->lh_Head = (struct Node *)&_NewList_list->lh_Tail;	\
  _NewList_list->lh_Tail = 0;						\
})

#endif /* _INLINE_ALIB_H */
