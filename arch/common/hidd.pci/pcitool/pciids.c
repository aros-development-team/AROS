#include <exec/types.h>

#include <proto/dos.h>

#include "pciids.h"

#include <ctype.h>
#include <stdio.h>

#include <aros/debug.h>

// it's supposed to become a shared library one day ...
// 
// current implementation:
// on pciids_Open(), the file is read in memory, then
// an index is built (in computeVendorIndexes()), as an array
// of couples (vendor_id, offset)
// where offset is the offset where the vendor stuff begins
// in the memory file. This array is ascending sorted by vendor_id,
// so a binary search can be done to retrieve the vendor offset
// given its id. This search is done in getVendorIndex().
// All the stringification functions first call this search func,
// then parse the memory:
// 1234  VendorName (so: s[0] = hex digit,  s[4] == ' ', s[6+] == name)
// <tab>1234  DeviceName (same with a tab on linestart)
// todo: subvendor/subdevice parsing
// todo: proper memory reallocation, currently the index is fixed
// to 2000 vendors (around 1700 exist on 2004-02-08)

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
    LONG j = 0;

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
		FreeVec(vendor_index);
		vendor_index = NULL;
		return FALSE;
	    }
	}
	i = skip_line(buffer, size, i);
    }
    vi_number = j - 1;
    return TRUE;
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

static LONG getDeviceIndex(LONG vendorIndex, UWORD deviceID)
{
    LONG i = vendorIndex;

    if (i < 0)
	return i;

    i = skip_line(mem, memsize, i); // skip vendor
    while ((i < memsize) && ((mem[i] == '\t') || (mem[i] == '#')))
    {
	UWORD did;

	if (mem[i] != '#')
	{
	    if ((i + 6 < memsize) && (mem[i + 5] == ' ')
		&& (sscanf(mem + i + 1, "%hx", &did) == 1) && (did == deviceID))
	    {
		return i;
	    }
	}
	i = skip_line(mem, memsize, i);
    }
    return -1;
}

static LONG getSubDeviceIndex(LONG deviceIndex, UWORD subVendorID, UWORD subDeviceID)
{
    LONG i = deviceIndex;

    if (i < 0)
	return i;

    i = skip_line(mem, memsize, i);
    while ((i < memsize) && ((mem[i] == '\t') || (mem[i] == '#')))
    {
	UWORD subvid, subdid;

	if ((mem[i] != '#') && (i + 1 < memsize) && (mem[i+1] == '\t'))
	{
	    if ((i + 11 < memsize)
		&& (mem[i + 6] == ' ')
		&& (sscanf(mem + i, "%hx", &subvid) == 1)
		&& (subvid == subVendorID)
		&& (mem[i + 11] == ' ')
		&& (sscanf(mem + i + 7, "%hx", &subdid) == 1)
		&& (subdid == subDeviceID))
	    {
		return i;
	    }
	}
	i = skip_line(mem, memsize, i);
    }
    return -1;
}

void pciids_Open(void)
{
    APTR fh;
    LONG size;

    fh = Open("DEVS:pci.ids", MODE_OLDFILE);
    if (!fh)
	goto err_open_ids;

    Seek(fh, 0, OFFSET_END);
    size = Seek(fh, 0, OFFSET_CURRENT);
    if (size <= 0)
	goto err_size;

    memsize = (ULONG)size;
    Seek(fh, 0, OFFSET_BEGINNING);

    mem = AllocVec(memsize, MEMF_ANY);
    if (NULL == mem)
	goto err_mem;

    if (Read(fh, mem, memsize) != size)
	goto err_read;

    if (!computeVendorIndexes(mem, memsize))
	goto err_index;

    // success !
    return;

  err_index:
  err_read:
    FreeVec(mem);
    mem = NULL;
  err_mem:
  err_size:
    Close(fh);
  err_open_ids:
    return;
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

STRPTR pciids_GetVendorName(UWORD vendorID, STRPTR buf, ULONG bufsize)
{
    LONG i = getVendorIndex(vendorID);

    buf[0] = 0;
    if (i < 0)
	return buf;

    copy_until_eol(mem, memsize, i + 6, buf, bufsize);

    return buf;
}

STRPTR pciids_GetDeviceName(UWORD vendorID, UWORD deviceID, STRPTR buf,
			    ULONG bufsize)
{
    LONG i = getVendorIndex(vendorID);

    buf[0] = 0;
    if (i < 0) // unknown vendor
	return buf;

    i = getDeviceIndex(i, deviceID);
    if (i < 0) // unknown device
	return buf;

    copy_until_eol(mem, memsize, i + 7, buf, bufsize);
    return buf;
}

STRPTR pciids_GetSubDeviceName(UWORD vendorID, UWORD deviceID, UWORD subVendorID,
			       UWORD subDeviceID, STRPTR buf, ULONG bufsize)
{
    LONG i;
    LONG j;
    LONG copied;

    buf[0] = 0;

    if ((0 == subVendorID) && (0 == subDeviceID))
	return buf;

    i = getVendorIndex(vendorID);
    if (i < 0) // unknown vendor
	return buf;

    i = getDeviceIndex(i, deviceID);
    if (i < 0) // unknown device
	return buf;

    j = getVendorIndex(subVendorID);
    if (j < 0) // unknown subvendor
	return buf;

    copied = copy_until_eol(mem, memsize, j + 6, buf, bufsize);

    if (copied + 4 < bufsize)
    {
	strcpy(buf + copied, " : ");

	i = getSubDeviceIndex(i, subVendorID, subDeviceID);
	if (i < 0) // unknown subdevice
	{
	    if (bufsize - copied - 3 > 6)
		sprintf(buf + copied + 3, "0x%04x", subDeviceID);
	    return buf;
	}

	copy_until_eol(mem, memsize, i + 13, buf + copied + 3,
		       bufsize - copied - 3);
    }

    return buf;
}
