#include <exec/types.h>

#include <proto/dos.h>

#include "pciids.h"

#include <ctype.h>
#include <stdio.h>

#include <aros/debug.h>

static STRPTR mem = NULL;
static ULONG memsize = 0;

struct vendor_cell
{
    UWORD vendorID;
    LONG  offset;
};

static struct vendor_cell *vendor_index = NULL;
static ULONG vi_allocated = 0;
static UWORD vi_number = 0;

static LONG skip_line(const char *buffer, LONG size, LONG pos)
{
    buffer += pos;
    while (pos < size)
    {
	if (*buffer++ == '\n')
	{
	    pos++;
	    break;
	}
	pos++;
    }
    return pos;
}

static LONG copy_until_eol(STRPTR m, ULONG msize, LONG pos, STRPTR buf,
			   ULONG bufsize)
{
    int j = 0;

    m += pos;
    while ((pos < msize) && (j < bufsize - 1) && (*m != '\n'))
    {
	buf[j++] = *m++;
    }
    buf[j] = 0;
    return j;
}

static BOOL computeVendorIndexes(const char *buffer, LONG size)
{
    LONG i, j;

    vi_allocated = 2000;
    vendor_index = AllocVec(vi_allocated * sizeof(struct vendor_cell), MEMF_ANY);
    if (NULL == vendor_index)
	return FALSE;

    i = 0;
    j = 0;

    while (i < size)
    {
	// dont use isxdigit, beware of uppercase letter
	if ((isdigit(buffer[i]) || (buffer[i] >= 'a' && buffer[i] <= 'f'))
	    && (i + 4 < size) && (buffer[i + 4] == ' '))
	{
	    if (sscanf(buffer + i, "%hx", &(vendor_index[j].vendorID)) != 1)
		return FALSE;
	    vendor_index[j].offset = i;
	    //bug("%ld: %x => %ld\n", j, vendor_index[j].vendorID, vendor_index[j].offset);
	    j++;
	    if (j > vi_allocated)
	    {
		bug("[pcitool] pciids.c:computeVendorIndexes: vendor_index overflow\n");
		return FALSE;
	    }
	}
	i = skip_line(buffer, size, i);
    }
    vi_number = j - 1;
    return TRUE;
}

void pciids_Open(void)
{
    APTR fh;

    fh = Open("DEVS:pci.ids", MODE_OLDFILE);
    if (fh)
    {
	LONG size;

	Seek(fh, 0, OFFSET_END);
	size = Seek(fh, 0, OFFSET_CURRENT);
	if (size > 0)
	{
	    memsize = (ULONG)size;
	    Seek(fh, 0, OFFSET_BEGINNING);

	    mem = AllocVec(memsize, MEMF_ANY);
	    if (mem)
	    {
		if (Read(fh, mem, memsize) == size)
		{
		    computeVendorIndexes(mem, memsize);
		}
		else
		{
		    FreeVec(mem);
		    mem = NULL;
		}
	    }
	}
	Close(fh);
    }
}

void pciids_Close(void)
{
    if (vendor_index)
    {
	FreeVec(vendor_index);
	vendor_index = NULL;
    }

    if (mem)
    {
	FreeVec(mem);
	mem = NULL;
    }
}

static LONG getVendorIndex(UWORD vendorID)
{
    LONG lower = 0;
    LONG upper = vi_number;

    if (!mem || !vendor_index)
	return -1;

    while (upper != lower)
    {
	UWORD vid;

	vid = vendor_index[(upper + lower) / 2].vendorID;
	if (vid == vendorID)
	    return vendor_index[(upper + lower) / 2].offset;
	if (vendorID > vid)
	    lower = (upper + lower) / 2;
	else
	    upper = (upper + lower) / 2;
    }
    return -1;
}

STRPTR pciids_GetVendorName(UWORD vendorID, STRPTR buf, ULONG bufsize)
{
    LONG i = getVendorIndex(vendorID);

    if (i >= 0)
    {
	i += 6;
	copy_until_eol(mem, memsize, i, buf, bufsize);
    }
    else
	buf[0] = 0;
    return buf;
}

STRPTR pciids_GetDeviceName(UWORD vendorID, UWORD deviceID, STRPTR buf,
			    ULONG bufsize)
{
    LONG i = getVendorIndex(vendorID);
    if (i < 0)
    {
	buf[0] = 0;
	return buf;
    }
    i = skip_line(mem, memsize, i);
    while ((i < memsize) && ((mem[i] == '\t') || (mem[i] == '#')))
    {
	UWORD did;

	i++;
	if ((i + 4 < memsize) && (mem[i + 4] == ' ')
	    && (sscanf(mem + i, "%hx", &did) == 1) && (did == deviceID))
	{
	    i += 6;
	    copy_until_eol(mem, memsize, i, buf, bufsize);
	    return buf;
	}
	i = skip_line(mem, memsize, i);
    }
    return buf;
}

STRPTR pciids_GetSubDeviceName(UWORD vendorID, UWORD deviceID, UWORD subVendorID,
			       UWORD subDeviceID, STRPTR buf, ULONG bufsize)
{
    LONG i = getVendorIndex(vendorID);
    if (i < 0)
    {
	buf[0] = 0;
	return buf;
    }

    return buf;
}


