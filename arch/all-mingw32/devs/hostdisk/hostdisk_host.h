#ifdef __x86_64__
#define __stdcall __attribute__((ms_abi))
#else
#define __stdcall __attribute__((stdcall))
#endif

#define INVALID_HANDLE_VALUE (void *)-1

#define FILE_SHARE_READ		0x00000001
#define FILE_SHARE_WRITE	0x00000002
#define FILE_SHARE_VALID_FLAGS	0x00000007
#define GENERIC_READ            0x80000000
#define GENERIC_WRITE		0x40000000
#define OPEN_EXISTING           3
#define FILE_BEGIN              0

#define FILE_ATTRIBUTE_READONLY 0x00000001
#define FILE_ATTRIBUTE_DEVICE   0x00000040
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

#define CTL_CODE(t,f,m,a) (((t)<<16)|((a)<<14)|((f)<<2)|(m))
#define IOCTL_DISK_GET_DRIVE_GEOMETRY 0x00070000 //CTL_CODE(7, 0, 0, 0)

typedef enum _MEDIA_TYPE
{
  Unknown          = 0x00,
  F5_1Pt2_512      = 0x01,
  F3_1Pt44_512     = 0x02,
  F3_2Pt88_512     = 0x03,
  F3_20Pt8_512     = 0x04,
  F3_720_512       = 0x05,
  F5_360_512       = 0x06,
  F5_320_512       = 0x07,
  F5_320_1024      = 0x08,
  F5_180_512       = 0x09,
  F5_160_512       = 0x0a,
  RemovableMedia   = 0x0b,
  FixedMedia       = 0x0c,
  F3_120M_512      = 0x0d,
  F3_640_512       = 0x0e,
  F5_640_512       = 0x0f,
  F5_720_512       = 0x10,
  F3_1Pt2_512      = 0x11,
  F3_1Pt23_1024    = 0x12,
  F5_1Pt23_1024    = 0x13,
  F3_128Mb_512     = 0x14,
  F3_230Mb_512     = 0x15,
  F8_256_128       = 0x16,
  F3_200Mb_512     = 0x17,
  F3_240M_512      = 0x18,
  F3_32M_512       = 0x19 
} MEDIA_TYPE;

typedef struct _DISK_GEOMETRY
{
  UQUAD      Cylinders;
  MEDIA_TYPE MediaType;
  ULONG      TracksPerCylinder;
  ULONG      SectorsPerTrack;
  ULONG      BytesPerSector;
} DISK_GEOMETRY;

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
