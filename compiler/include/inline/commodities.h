#ifndef _INLINE_COMMODITIES_H
#define _INLINE_COMMODITIES_H

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef COMMODITIES_BASE_NAME
#define COMMODITIES_BASE_NAME CxBase
#endif

#define ActivateCxObj(co, tf) \
	LP2(0x2a, LONG, ActivateCxObj, CxObj *, co, a0, long, tf, d0, \
	, COMMODITIES_BASE_NAME)

#define AddIEvents(events) \
	LP1NR(0xb4, AddIEvents, struct InputEvent *, events, a0, \
	, COMMODITIES_BASE_NAME)

#define AttachCxObj(headObj, co) \
	LP2NR(0x54, AttachCxObj, CxObj *, headObj, a0, CxObj *, co, a1, \
	, COMMODITIES_BASE_NAME)

#define ClearCxObjError(co) \
	LP1NR(0x48, ClearCxObjError, CxObj *, co, a0, \
	, COMMODITIES_BASE_NAME)

#define CreateCxObj(type, arg1, arg2) \
	LP3(0x1e, CxObj *, CreateCxObj, unsigned long, type, d0, long, arg1, a0, long, arg2, a1, \
	, COMMODITIES_BASE_NAME)

#define CxBroker(nb, error) \
	LP2(0x24, CxObj *, CxBroker, struct NewBroker *, nb, a0, LONG *, error, d0, \
	, COMMODITIES_BASE_NAME)

#define CxMsgData(cxm) \
	LP1(0x90, APTR, CxMsgData, CxMsg *, cxm, a0, \
	, COMMODITIES_BASE_NAME)

#define CxMsgID(cxm) \
	LP1(0x96, LONG, CxMsgID, CxMsg *, cxm, a0, \
	, COMMODITIES_BASE_NAME)

#define CxMsgType(cxm) \
	LP1(0x8a, ULONG, CxMsgType, CxMsg *, cxm, a0, \
	, COMMODITIES_BASE_NAME)

#define CxObjError(co) \
	LP1(0x42, LONG, CxObjError, CxObj *, co, a0, \
	, COMMODITIES_BASE_NAME)

#define CxObjType(co) \
	LP1(0x3c, ULONG, CxObjType, CxObj *, co, a0, \
	, COMMODITIES_BASE_NAME)

#define DeleteCxObj(co) \
	LP1NR(0x30, DeleteCxObj, CxObj *, co, a0, \
	, COMMODITIES_BASE_NAME)

#define DeleteCxObjAll(co) \
	LP1NR(0x36, DeleteCxObjAll, CxObj *, co, a0, \
	, COMMODITIES_BASE_NAME)

#define DisposeCxMsg(cxm) \
	LP1NR(0xa8, DisposeCxMsg, CxMsg *, cxm, a0, \
	, COMMODITIES_BASE_NAME)

#define DivertCxMsg(cxm, headObj, returnObj) \
	LP3NR(0x9c, DivertCxMsg, CxMsg *, cxm, a0, CxObj *, headObj, a1, CxObj *, returnObj, a2, \
	, COMMODITIES_BASE_NAME)

#define EnqueueCxObj(headObj, co) \
	LP2NR(0x5a, EnqueueCxObj, CxObj *, headObj, a0, CxObj *, co, a1, \
	, COMMODITIES_BASE_NAME)

#define InsertCxObj(headObj, co, pred) \
	LP3NR(0x60, InsertCxObj, CxObj *, headObj, a0, CxObj *, co, a1, CxObj *, pred, a2, \
	, COMMODITIES_BASE_NAME)

#define InvertKeyMap(ansiCode, event, km) \
	LP3(0xae, BOOL, InvertKeyMap, unsigned long, ansiCode, d0, struct InputEvent *, event, a0, struct KeyMap *, km, a1, \
	, COMMODITIES_BASE_NAME)

#define MatchIX(event, ix) \
	LP2(0xcc, BOOL, MatchIX, struct InputEvent *, event, a0, IX *, ix, a1, \
	, COMMODITIES_BASE_NAME)

#define ParseIX(description, ix) \
	LP2(0x84, LONG, ParseIX, STRPTR, description, a0, IX *, ix, a1, \
	, COMMODITIES_BASE_NAME)

#define RemoveCxObj(co) \
	LP1NR(0x66, RemoveCxObj, CxObj *, co, a0, \
	, COMMODITIES_BASE_NAME)

#define RouteCxMsg(cxm, co) \
	LP2NR(0xa2, RouteCxMsg, CxMsg *, cxm, a0, CxObj *, co, a1, \
	, COMMODITIES_BASE_NAME)

#define SetCxObjPri(co, pri) \
	LP2(0x4e, LONG, SetCxObjPri, CxObj *, co, a0, long, pri, d0, \
	, COMMODITIES_BASE_NAME)

#define SetFilter(filter, text) \
	LP2NR(0x78, SetFilter, CxObj *, filter, a0, STRPTR, text, a1, \
	, COMMODITIES_BASE_NAME)

#define SetFilterIX(filter, ix) \
	LP2NR(0x7e, SetFilterIX, CxObj *, filter, a0, IX *, ix, a1, \
	, COMMODITIES_BASE_NAME)

#define SetTranslate(translator, events) \
	LP2NR(0x72, SetTranslate, CxObj *, translator, a0, struct InputEvent *, events, a1, \
	, COMMODITIES_BASE_NAME)

#endif /* _INLINE_COMMODITIES_H */
