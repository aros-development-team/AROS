#ifdef __x86_64__
#define __stdcall
#else
#define __stdcall __attribute__((stdcall))
#endif

#define INVALID_HANDLE_VALUE (void *)-1

#define FILE_SHARE_VALID_FLAGS	0x00000007
#define GENERIC_READ            0x80000000
#define OPEN_EXISTING           3
#define FILE_BEGIN              0

#define FILE_ATTRIBUTE_READONLY 0x00000001
#define FILE_ATTRIBUTE_NORMAL   0x00000080

#define ERROR_INVALID_FUNCTION		1L
#define ERROR_FILE_NOT_FOUND		2L
#define ERROR_PATH_NOT_FOUND		3L
#define ERROR_ACCESS_DENIED		5L
#define ERROR_NOT_ENOUGH_MEMORY 	8L
#define ERROR_NO_MORE_FILES		18L
#define ERROR_WRITE_PROTECT		19L
#define ERROR_SHARING_VIOLATION 	32L
#define ERROR_LOCK_VIOLATION		33L
#define ERROR_HANDLE_EOF		38L
#define ERROR_FILE_EXISTS		80L
#define ERROR_INVALID_NAME		123L
#define ERROR_DIR_NOT_EMPTY		145L
#define ERROR_IO_PENDING		997L

typedef void *file_t;

struct HostInterface
{
    void * __stdcall (*CreateFile)(char *lpFileName, ULONG dwDesiredAccess, ULONG dwShareMode, void *lpSecurityAttributes,
				   ULONG dwCreationDisposition, ULONG dwFlagsAndAttributes, void *hTemplateFile);
    ULONG  __stdcall (*CloseHandle)(void *hObject);
    ULONG  __stdcall (*ReadFile)(void *hFile, void *lpBuffer, ULONG nNumberOfBytesToRead, ULONG *lpNumberOfBytesRead, void *lpOverlapped);
    ULONG  __stdcall (*WriteFile)(void *hFile, void *lpBuffer, ULONG nNumberOfBytesToWrite, ULONG *lpNumberOfBytesWritten, void *lpOverlapped);
    ULONG  __stdcall (*SetFilePointer)(void *hFile, LONG lDistanceToMove, LONG *lpDistanceToMoveHigh, ULONG dwMoveMethod);
    ULONG  __stdcall (*GetFileAttributes)(STRPTR lpFileName);
    ULONG  __stdcall (*GetFileSize)(void *hFile, ULONG *lpFileSizeHigh);
    ULONG  __stdcall (*DeviceIoControl)(void *hDevice, ULONG dwIoControlCode, void *lpInBuffer, ULONG nInBufferSize,
					void *lpOutBuffer, ULONG nOutBufferSize, ULONG *lpBytesReturned, void *lpOverlapped);
    ULONG  __stdcall (*GetLastError)(void);
};
