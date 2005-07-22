#ifndef _DIRLIST_PRIVATE_H_
#define _DIRLIST_PRIVATE_H_

struct Dirlist_DATA
{
    struct Hook construct_hook;
    struct Hook destruct_hook;
    struct Hook display_hook;
    struct Hook *filterhook;
    STRPTR acceptpattern;
    STRPTR rejectpattern;
    STRPTR directory;
    IPTR numbytes;
    ULONG numdrawers;
    ULONG numfiles;    
    BOOL drawersonly;
    BOOL filesonly;
    BOOL filterdrawers;
    BOOL multiseldirs;
    BOOL rejecticons;
    BOOL sorthighlow;
    BYTE sortdirs;
    BYTE sorttype;
    BYTE status;
    UBYTE size_string[20];
    UBYTE date_string[20];
    UBYTE time_string[20];
    UBYTE prot_string[8];
    STRPTR path;
};

struct Dirlist_Entry
{
    struct FileInfoBlock fib;
};

#endif /* _DIRLIST_PRIVATE_H_ */
