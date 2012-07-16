/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.

    Desc: Code for the handling of a list of strings
*/

#ifndef STRINGLIST_H
#define STRINGLIST_H

struct stringlist {
    struct stringlist *next;
    char *s;
};

struct stringlist *slist_append(struct stringlist **list, const char *s);
struct stringlist *slist_prepend(struct stringlist **list, const char *s);
int slist_remove(struct stringlist **list, struct stringlist *node);
int slist_length(struct stringlist *list);

#endif //STRINGLIST_H
