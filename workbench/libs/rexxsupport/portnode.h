#include <exec/ports.h>
#include <exec/libraries.h>
#include <rexx/storage.h>

struct PortNodeData
{
    struct Node node;
    struct RexxRsrc *self;
    struct MsgPort *port;
    struct List msgs;
};

struct PortNode
{
    struct RexxRsrc rsrc;
    struct PortNodeData data;
};

void portcleanup(struct Library *, struct PortNode *);
