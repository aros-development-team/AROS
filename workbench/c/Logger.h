
#include <exec/types.h>

#define DISKLOG_BUFFER_SIZE  (64 * 1024) // 64KB buffer

struct DiskLogEntryHeader
{
    ULONG Flags;             // LOGMA_Flags
    struct DateStamp Stamp;     // LOGMA_DateStamp
    ULONG EventID;           // LOGMA_EventID
    ULONG OriginLen;         // strlen(Origin) + 1
    ULONG ComponentLen;      // strlen(Component) + 1
    ULONG SubComponentLen;   // strlen(SubComponent) + 1
    ULONG LogTagLen;         // strlen(LogTag) + 1
    ULONG EntryLen;          // strlen(Entry) + 1
    // Followed by: Origin[], Component[], SubComponent[], LogTag[], Entry[]
};
