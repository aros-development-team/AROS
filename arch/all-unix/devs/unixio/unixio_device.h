struct Queue
{
    struct IOStdReq *Active;
    APTR             Data;
    ULONG            Length;
};  

struct UnitData
{
    struct Queue readQueue;
    struct Queue writeQueue;
    void (*errorCallback)(struct IOStdReq *req, int err);
    BOOL stopped;
    unsigned char *termarray;
};
