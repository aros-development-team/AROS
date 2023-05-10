
#ifndef _LOGVIEW_H
#define _LOGVIEW_H

#include <exec/types.h>

struct LogListEntry
{
    struct Node  node;
    struct Node *entry;
};

void CreateLogEntryList();
void FreeLogEntryList();

#endif /* _LOGVIEW_H */
