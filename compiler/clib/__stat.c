/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include "__time.h"
#include "__stat.h"

#include <sys/stat.h>
#include <aros/debug.h>

static mode_t __prot_a2u(ULONG protect);
static uid_t  __id_a2u(UWORD id);
static void hashlittle2(const void *key, size_t length, 
		                   uint32_t *pc, uint32_t *pb);
static void __fill_statbuffer(
    struct stat          *sb,
    char                 *buffer,
    struct FileInfoBlock *fib,
    int                  fallback_to_defaults,
    BPTR                 lock);

int __stat(BPTR lock, struct stat *sb, BOOL filehandle)
{
    struct FileInfoBlock *fib;
    UBYTE *buffer;
    int buffersize = 256;
    int fallback_to_defaults = 0;
    BOOL Examined;

    fib = AllocDosObject(DOS_FIB, NULL);

    if (!fib)
    {
        errno = __arosc_ioerr2errno(IoErr());

        return -1;
    }

    Examined = filehandle
             ? ExamineFH(lock, fib)
             : Examine(lock, fib);
    if (!Examined)
    {
	if(IoErr() == ERROR_NOT_IMPLEMENTED)
	{
	    fallback_to_defaults = 1;
	}
	else
	{
            errno = __arosc_ioerr2errno(IoErr());
            FreeDosObject(DOS_FIB, fib);
            return -1;
	}
    }

    /* Get the full path of the stated filesystem object and use it to
       compute hash value */
    do
    {
        BOOL GotName;

        if(!(buffer = AllocVec(buffersize, MEMF_ANY)))
        {
            errno = ENOMEM;
            FreeDosObject(DOS_FIB, fib);
            return -1;
        }

        GotName = filehandle
                ? NameFromFH(lock, buffer, buffersize)
                : NameFromLock(lock, buffer, buffersize);
        if(GotName)
            break;
        else if(   IoErr() == ERROR_OBJECT_IN_USE
                || IoErr() == ERROR_NOT_IMPLEMENTED
                || (IoErr() == ERROR_OBJECT_NOT_FOUND && fib->fib_EntryType == ST_PIPEFILE))
        {
            /* We can't retrieve name because lock is an exclusive lock
               or Examine is not implemented in this handler
               or the lock refers to an XPIPE: file having always empty name */
            buffer[0] = '\0';
            break;
        }
        else if(IoErr() != ERROR_LINE_TOO_LONG)
        {
            errno = __arosc_ioerr2errno(IoErr());
            FreeDosObject(DOS_FIB, fib);
            FreeVec(buffer);
            return -1;
        }
        FreeVec(buffer);
        buffersize *= 2;
    }
    while(TRUE);

    __fill_statbuffer(sb, (char*) buffer, fib, fallback_to_defaults, lock);

    FreeVec(buffer);
    FreeDosObject(DOS_FIB, fib);

    return 0;
}


int __stat_from_path(const char *path, struct stat *sb)
{
    int                  len;
    char                 *mypath;
    int                  cwdsize = FILENAME_MAX;
    char                 *cwd = NULL;
    char                 *abspath = NULL;
    char                 *filepart = NULL;
    char                 *split;
    struct FileInfoBlock *fib = NULL;
    BPTR                 lock = BNULL;
    BPTR                 cwdlock;
    int                  fallback_to_defaults = 0;
    BOOL                 loop;
    int                  res = -1;

    /* copy path and strip trailing slash */
    len = strlen(path);
    if (!(mypath = AllocVec(len + 1, MEMF_ANY)))
    {
        errno = ENOMEM;
        goto out;
    }
    strcpy(mypath, path);
    if (len && mypath[len-1] == '/')
        mypath[len-1] = '\0';

    /* do we have an absolute path */
    if (!strchr(mypath, ':'))
    {
        /* no, then create one */
        cwdlock = CurrentDir(BNULL);
        CurrentDir(cwdlock);
        do
        {
            if (!(cwd = AllocVec(cwdsize, MEMF_ANY)))
            {
                errno = ENOMEM;
                goto out;
            }

            if (NameFromLock(cwdlock, cwd, cwdsize))
                break;
            else if (IoErr() != ERROR_LINE_TOO_LONG)
            {
                errno = __arosc_ioerr2errno(IoErr());
                goto out;
            }

            FreeVec(cwd);
            cwdsize *= 2;
        }
        while (TRUE);

        /* get memory for current dir + '/' + input path + zero byte */
        len = strlen(cwd) + 1 + len + 1;
        abspath = AllocVec(len, MEMF_ANY);
        if (!abspath)
        {
            errno = ENOMEM;
            goto out;
        }

        strcpy(abspath, cwd);
        AddPart(abspath, mypath, len);
        FreeVec(mypath);
    }
    else
        abspath = mypath;

    /* split into path part and file part */
    split = FilePart(abspath);
    filepart = AllocVec(strlen(split) + 1, MEMF_ANY);
    if (!filepart)
    {
        errno = ENOMEM;
        goto out;
    }
    strcpy(filepart, split);
    *split = '\0';

    if (   !(fib = AllocDosObject(DOS_FIB, NULL))
        || !(lock = Lock(abspath, SHARED_LOCK)))
    {
        errno = __arosc_ioerr2errno(IoErr());
        goto out;
    }

    /* examine parent directory of object to stat */
    if (!Examine(lock, fib))
    {
        if (IoErr() == ERROR_NOT_IMPLEMENTED)
            fallback_to_defaults = 1;
        else
        {
            errno = __arosc_ioerr2errno(IoErr());
            goto out;
        }
    }

    if (*filepart == '\0' || fallback_to_defaults)
        __fill_statbuffer(sb, abspath, fib, fallback_to_defaults, lock);
    else
        /* examine entries of parent directory until we find the object to stat */
        do
        {
            loop = ExNext(lock, fib);

            if (loop)
            {
                if (stricmp(fib->fib_FileName, filepart) == 0)
                {
                    __fill_statbuffer(sb, abspath, fib, 0, lock);
                    res = 0;
                    break;
                }

                continue;
            }

            if (IoErr() != ERROR_NO_MORE_ENTRIES)
                errno = __arosc_ioerr2errno(IoErr());
            else
                /* nothing found to stat */
                errno = ENOENT;
        }
        while (loop);

out:
    if (lock)
        UnLock(lock);

    if (cwd)
        FreeVec(cwd);

    /* if we had absolute path as input, mypath is free'd here */
    if (abspath)
        FreeVec(abspath);

    if (filepart)
        FreeVec(filepart);

    if (fib)
        FreeDosObject(DOS_FIB, fib);

    return res;
}


static mode_t __prot_a2u(ULONG protect)
{
    mode_t uprot = 0000;

    if ((protect & FIBF_SCRIPT))
        uprot |= 0111;
    /* The following three flags are low-active! */
    if (!(protect & FIBF_EXECUTE))
        uprot |= 0100;
    if (!(protect & FIBF_WRITE))
        uprot |= 0200;
    if (!(protect & FIBF_READ))
        uprot |= 0400;
    if ((protect & FIBF_GRP_EXECUTE))
        uprot |= 0010;
    if ((protect & FIBF_GRP_WRITE))
        uprot |= 0020;
    if ((protect & FIBF_GRP_READ))
        uprot |= 0040;
    if ((protect & FIBF_OTR_EXECUTE))
        uprot |= 0001;
    if ((protect & FIBF_OTR_WRITE))
        uprot |= 0002;
    if ((protect & FIBF_OTR_READ))
        uprot |= 0004;

    return uprot;
}


static uid_t __id_a2u(UWORD id)
{
    switch(id)
    {
        case (UWORD)-1:
            return 0;

        case (UWORD)-2:
            return (UWORD)-1;

        case 0:
            return (UWORD)-2;

        default:
            return id;
    }
}

/* The hash function code below is adapted from Bob Jenkins public domain hash
   function, see http://burtleburtle.net/bob/hash/doobs.html for details */

#if AROS_BIG_ENDIAN
# define HASH_LITTLE_ENDIAN 0
#else
# define HASH_LITTLE_ENDIAN 1
#endif

#define hashsize(n) ((uint32_t)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

/*
 * hashlittle2() -- hash a variable-length key into two 32-bit values
 * k       : the key (the unaligned variable-length array of bytes)
 * length  : the length of the key, counting by bytes
 * pc      : IN: primary initval, OUT: primary hash
 * pb      : IN: secondary initval, OUT: secondary hash
 * 
 * Returns two 32-bit hash values.  This is good enough for hash table
 * lookup with 2^^64 buckets, or if you want a second hash if you're not
 * happy with the first, or if you want a probably-unique 64-bit ID for
 * the key.  *pc is better mixed than *pb, so use *pc first.  If you want
 * a 64-bit value do something like "*pc + (((uint64_t)*pb)<<32)".
 */
static void hashlittle2( 
  const void *key,       /* the key to hash */
  size_t      length,    /* length of the key */
  uint32_t   *pc,        /* IN: primary initval, OUT: primary hash */
  uint32_t   *pb)        /* IN: secondary initval, OUT: secondary hash */
{
  uint32_t a,b,c;                                          /* internal state */
  union { const void *ptr; size_t i; } u;     /* needed for Mac Powerbook G4 */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + *pc;
  c += *pb;

  u.ptr = key;
  if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0)) {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */
#ifdef VALGRIND
    const uint8_t  *k8;
#endif

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /* 
     * "k[2]&0xffffff" actually reads beyond the end of the string, but
     * then masks off the part it's not allowed to read.  Because the
     * string is aligned, the masked-off tail is in the same word as the
     * rest of the string.  Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this.  But VALGRIND will
     * still catch it and complain.  The masking trick does make the hash
     * noticably faster for short strings (like English words).
     */
#ifndef VALGRIND

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

#else /* make valgrind happy */

    k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=((uint32_t)k8[10])<<16;  /* fall through */
    case 10: c+=((uint32_t)k8[9])<<8;    /* fall through */
    case 9 : c+=k8[8];                   /* fall through */
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=((uint32_t)k8[6])<<16;   /* fall through */
    case 6 : b+=((uint32_t)k8[5])<<8;    /* fall through */
    case 5 : b+=k8[4];                   /* fall through */
    case 4 : a+=k[0]; break;
    case 3 : a+=((uint32_t)k8[2])<<16;   /* fall through */
    case 2 : a+=((uint32_t)k8[1])<<8;    /* fall through */
    case 1 : a+=k8[0]; break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

#endif /* !valgrind */

  } else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0)) {
    const uint16_t *k = (const uint16_t *)key;         /* read 16-bit chunks */
    const uint8_t  *k8;

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((uint32_t)k[1])<<16);
      b += k[2] + (((uint32_t)k[3])<<16);
      c += k[4] + (((uint32_t)k[5])<<16);
      mix(a,b,c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[4]+(((uint32_t)k[5])<<16);
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 11: c+=((uint32_t)k8[10])<<16;     /* fall through */
    case 10: c+=k[4];
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 9 : c+=k8[8];                      /* fall through */
    case 8 : b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 7 : b+=((uint32_t)k8[6])<<16;      /* fall through */
    case 6 : b+=k[2];
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 5 : b+=k8[4];                      /* fall through */
    case 4 : a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 3 : a+=((uint32_t)k8[2])<<16;      /* fall through */
    case 2 : a+=k[0];
             break;
    case 1 : a+=k8[0];
             break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }

  } else {                        /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((uint32_t)k[1])<<8;
      a += ((uint32_t)k[2])<<16;
      a += ((uint32_t)k[3])<<24;
      b += k[4];
      b += ((uint32_t)k[5])<<8;
      b += ((uint32_t)k[6])<<16;
      b += ((uint32_t)k[7])<<24;
      c += k[8];
      c += ((uint32_t)k[9])<<8;
      c += ((uint32_t)k[10])<<16;
      c += ((uint32_t)k[11])<<24;
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((uint32_t)k[11])<<24;
    case 11: c+=((uint32_t)k[10])<<16;
    case 10: c+=((uint32_t)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((uint32_t)k[7])<<24;
    case 7 : b+=((uint32_t)k[6])<<16;
    case 6 : b+=((uint32_t)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((uint32_t)k[3])<<24;
    case 3 : a+=((uint32_t)k[2])<<16;
    case 2 : a+=((uint32_t)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : *pc=c; *pb=b; return;  /* zero length strings require no mixing */
    }
  }

  final(a,b,c);
  *pc=c; *pb=b;
}

static void __fill_statbuffer(
    struct stat          *sb,
    char                 *buffer,
    struct FileInfoBlock *fib,
    int                  fallback_to_defaults,
    BPTR                 lock)
{
    struct aroscbase *aroscbase = __GM_GetBase();
    uint64_t hash;
    uint32_t pc = 1, pb = 1; /* initial hash values */

    hashlittle2(buffer, strlen((char*) buffer), &pc, &pb);
    hash = pc + (((uint64_t)pb)<<32);

    if(fallback_to_defaults)
    {
	/* Empty file, not protected, as old as it can be... */
	fib->fib_Size = 0;
	fib->fib_NumBlocks = 0;
	fib->fib_Date.ds_Days = 0;
	fib->fib_Date.ds_Minute = 0;
	fib->fib_Date.ds_Tick = 0;
	fib->fib_OwnerUID = 0;
	fib->fib_OwnerGID = 0;
	fib->fib_Protection = 0;
	/* Most probable value */
	fib->fib_DirEntryType = ST_PIPEFILE;
    }

    sb->st_dev     = (dev_t)((struct FileLock *)BADDR(lock))->fl_Volume;
    sb->st_ino     = hash;    /* hash value will be truncated if st_ino size is
                                 smaller than uint64_t, but it's ok */
    sb->st_size    = (off_t)fib->fib_Size;
    sb->st_atime   =
    sb->st_ctime   =
    sb->st_mtime   = (fib->fib_Date.ds_Days * 24*60 + fib->fib_Date.ds_Minute + aroscbase->acb_gmtoffset) * 60 +
	              fib->fib_Date.ds_Tick / TICKS_PER_SECOND + OFFSET_FROM_1970;
    sb->st_uid     = __id_a2u(fib->fib_OwnerUID);
    sb->st_gid     = __id_a2u(fib->fib_OwnerGID);
    sb->st_mode    = __prot_a2u(fib->fib_Protection);

    {
        struct InfoData info;

        if (Info(lock, &info))
        {
            sb->st_blksize = info.id_BytesPerBlock;
        }
        else
        {
            /* The st_blksize is just a guideline anyway, so we set it
               to 1024 in case Info() didn't succeed */
            sb->st_blksize = 1024;
        }
    }
    if(fib->fib_Size > 0 && sb->st_blksize > 0)
	sb->st_blocks = 
	    (1 + ((long) fib->fib_Size - 1) / sb->st_blksize) * 
	    (sb->st_blksize / 512);
    else
	sb->st_blocks  = 0;

    switch (fib->fib_DirEntryType)
    {
        case ST_PIPEFILE:
            /* don't use S_IFIFO, we don't have a mkfifo() call ! */
            sb->st_mode |= S_IFCHR;
            break;

        case ST_ROOT:
        case ST_USERDIR:
        case ST_LINKDIR:
            sb->st_nlink = 1;
            sb->st_mode |= S_IFDIR;
            break;

        case ST_SOFTLINK:
            sb->st_nlink = 1;
            sb->st_mode |= S_IFLNK;
            break;

        case ST_FILE:
        case ST_LINKFILE:
        default:
            sb->st_nlink = 1;
            sb->st_mode |= S_IFREG;
    }
}
