#ifndef AHI_Drivers_AC97_DriverData_h
#define AHI_Drivers_AC97_DriverData_h

#include <exec/libraries.h>
#include <dos/dos.h>
#include <oop/oop.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <proto/exec.h>
#include <hidd/pci.h>

#include "DriverBase.h"

struct ac97Base
{
    struct DriverBase	driverbase;
    struct Library*	dosbase;
    struct OOPBase*	oopbase;
    struct ExecBase*	sysbase;
    BOOL		cardfound;
    ULONG		mixerbase;
    ULONG		dmabase;
    ULONG		irq_num;

    /* card specific data */
    ULONG       off_po_sr;
    ULONG       off_po_picb;
    ULONG       size_shift;
    /* card specific data ends */

    struct {
	APTR		sample_address;
	ULONG		sample_size;
    }			*PCM_out;

    void		(*mixer_set_reg)(struct ac97Base *, ULONG reg, UWORD value);
    UWORD		(*mixer_get_reg)(struct ac97Base *, ULONG reg);
};

#define DRIVERBASE_SIZEOF (sizeof (struct ac97Base))

#define DOSBase         ((struct DosLibrary*)ac97Base->dosbase)
#define OOPBase		((struct OOPBase *)ac97Base->oopbase)

struct AC97Data
{
    struct DriverData   driverdata;
    UBYTE		flags;
    UBYTE		pad1;
    BYTE		mastersignal;
    BYTE		slavesignal;
    struct Process*	mastertask;
    struct Process*	slavetask;
    struct ac97Base*	ahisubbase;
    APTR		mixbuffer;
    UWORD		old_SR;
    
    struct Interrupt 	irq;

    ULONG		out_volume;
};

/* AC97 mixer registers */
#define AC97_RESET		0x00
#define AC97_MASTER_VOL		0x02
#define AC97_HEADPHONE_VOL	0x04
#define AC97_MASTER_MONO_VOL	0x06
#define AC97_TONE		0x08
#define AC97_PCBEEP_VOL		0x0a
#define AC97_PHONE_VOL		0x0c
#define AC97_MIC_VOL		0x0e
#define AC97_LINEIN_VOL		0x10
#define AC97_CD_VOL		0x12
#define AC97_VIDEO_VOL		0x14
#define AC97_AUX_VOL		0x16
#define AC97_PCM_VOL		0x18
#define AC97_RECORD_SEL		0x1a
#define AC97_RECORD_GAIN	0x1c
#define AC97_RECORD_GAIN_MIX	0x1e
#define AC97_POWERDOWN		0x26

#define PO_BDBAR		0x10
#define PO_CIV			0x14
#define PO_LVI			0x15
#define DEFAULT_PO_SR   0x16
#define DEFAULT_PO_PICB 0x18
#define PO_PIV			0x1a
#define PO_CR			0x1b

#define GLOB_CNT		0x2c
#define GLOB_STA		0x30
#define ACC_SEMA		0x34

#endif /* AHI_Drivers_AC97_DriverData_h */
