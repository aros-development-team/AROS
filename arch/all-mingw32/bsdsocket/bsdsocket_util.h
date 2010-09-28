void SetError(int error, struct TaskBase *libPtr);
ULONG SetDTableSize(ULONG size, struct TaskBase *taskBase);
int GetFreeFD(struct TaskBase *taskBase);
struct Socket *GetSocket(int s, struct TaskBase *taskBase);
struct Socket *IntCloseSocket(int s, struct TaskBase *taskBase);
