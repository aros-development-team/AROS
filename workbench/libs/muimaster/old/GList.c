/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <muimaster_private.h>

#include <zunepriv.h>

// TODO: allocate nodes from a pool

AROS_LH2(GList *, g_list_append,
    AROS_LHA(GList  *, list, A0),
    AROS_LHA(gpointer, data, A1),
    struct Library *, MUIMasterBase, 40, MUIMaster)
{
    GList *node = (GList *)AllocVec(sizeof(GList), MEMF_ANY);
    GList *temp;

    node->next = 0;
    node->prev = 0;
    node->data = data;

    if (list == 0)
        return node;

    for (temp = list; temp->next != 0; temp = temp->next);
    temp->next = node;
    node->prev = temp;

    return list;
}

AROS_LH2(GList *, g_list_prepend,
    AROS_LHA(GList  *, list, A0),
    AROS_LHA(gpointer, data, A1),
    struct Library *, MUIMasterBase, 41, MUIMaster)
{
    GList *node = (GList *)AllocVec(sizeof(GList), MEMF_ANY);

    node->next = list;
    node->prev = 0;
    node->data = data;

    if (list != 0)
        list->prev = node;

    return node;
}

AROS_LH2(GList *, g_list_find,
    AROS_LHA(GList  *, list, A0),
    AROS_LHA(gpointer, data, A1),
    struct Library *, MUIMasterBase, 42, MUIMaster)
{
    while (list != 0)
    {
        if (list->data == data)
            return list;
        list = list->next;
    }

    return 0;
}

AROS_LH2(GList *, g_list_remove,
    AROS_LHA(GList  *, list, A0),
    AROS_LHA(gpointer, data, A1),
    struct Library *, MUIMasterBase, 43, MUIMaster)
{
    GList *node = g_list_find(list, data);

    if (node != 0)
    {
        if (node == list)
            list = node->next;

        if (node->next != 0)
            node->next->prev = node->prev;
        if (node->prev != 0)
            node->prev->next = node->next;
        FreeVec(node);
    }

    return list;
}

/*
 * Reverse list
 * (simply exchange next and prev)
 */
AROS_LH1(GList *, g_list_reverse,
    AROS_LHA(GList *, list, A0),
    struct Library *, MUIMasterBase, 44, MUIMaster)
{
    GList *node = 0;

    while (list != 0)
    {
        // stacco il primo nodo
        node = list;
        list = list->next;

        // inserisco nella nuova lista
        node->next = node->prev;
        node->prev = list;
    }

    return node;
}

AROS_LH1(int, g_list_length,
    AROS_LHA(GList *, list, A0),
    struct Library *, MUIMasterBase, 45, MUIMaster)
{
    int i = 0;

    while (list != 0)
    {
        i++;
        list = list->next;
    }

    return i;
}

AROS_LH1(GList *, g_list_first,
    AROS_LHA(GList *, list, A0),
    struct Library *, MUIMasterBase, 46, MUIMaster)
{
    return list;
}

AROS_LH1(GList *, g_list_last,
    AROS_LHA(GList *, list, A0),
    struct Library *, MUIMasterBase, 47, MUIMaster)
{
    while (list != 0)
        list = list->next;

    return list;
}

AROS_LH3(void, g_list_foreach,
    AROS_LHA(GList   *, list,  A0),
    AROS_LHA(VOID_FUNC, func,  A1),
    AROS_LHA(gpointer , param, A2),
    struct Library *, MUIMasterBase, 48, MUIMaster)
{
    void (*f)(gpointer data, gpointer param) = (void (*)(gpointer, gpointer))func;

    while (list != 0)
    {
        f(list->data, param);
        list = list->next;
    }
}

AROS_LH1(void, g_list_free,
    AROS_LHA(GList *, list, A0),
    struct Library *, MUIMasterBase, 49, MUIMaster)
{
    GList *temp;

    while (list != 0)
    {
        temp = list;
        list = list->next;

        FreeVec(temp);
    }
}

/*** EOF ***/
