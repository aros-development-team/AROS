/*
** IconListClass - the left hand navigation list (a subclass of List.mui),
** mirroring Trident's IconListClass. Each entry is a page: a small icon
** image plus a label. The active entry drives the page group.
*/

#ifndef ICONLISTCLASS_H
#define ICONLISTCLASS_H

#include <exec/types.h>
#include <intuition/classusr.h>
#include "bluetoothprefs.h"
#include "icons.h"

/* One navigation entry. */
struct BtNavEntry
{
    CONST_STRPTR  label;    /* page name shown in the list */
    ULONG         page;     /* BTPAGE_# */
    ULONG         icon;     /* index into the icon table */
};

/* Each IconListClass instance builds its own copy of every icon as a MUI list
 * image; display hooks reach them via INST_DATA and render "\33O[image] text". */
struct IconListData
{
    Object *images[ICON_COUNT];   /* MUI list image handles */
    Object *bodies[ICON_COUNT];   /* the BodychunkObjects behind them */
};

/* helper for display hooks: the image handles of a given IconListClass list */
#define ICONLIST_IMAGES(listobj) \
    (((struct IconListData *)INST_DATA(IconListClass->mcc_Class, (listobj)))->images)

AROS_UFP3(IPTR, IconListDispatcher,
          AROS_UFPA(struct IClass *, cl, A0),
          AROS_UFPA(Object *, obj, A2),
          AROS_UFPA(Msg, msg, A1));

#endif /* ICONLISTCLASS_H */
