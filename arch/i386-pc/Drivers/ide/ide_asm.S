#include "ide.i"

/*
    TestDevice is called with following parameters:
	struct timerequest*	tr
	char			drive_id
	word			io_base
*/
#define tr		8
#define	drive_id	12
#define	io_base		16

			.text
		        .globl	TestDevice
			.type	TestDevice,@function

			.align	16

TestDevice:		pushl	%ebp
			movl	%esp,%ebp
			
			pushl	%esi
			pushl	%ebx
			pushl	%edi

/* Test whether this is a valid IO port */

			movl	io_base(%ebp),%edx	// edx = ioBase
			addl	$ata_DevHead,%edx
			movl	drive_id(%ebp),%eax	// eax = devsel
			
			outb	%al,%dx			// send command
			inb	%dx,%al
			cmp	$0xff,%al
			je	.Error

/* This is a valid IDE IO port. Now, do we have a device attached to it? */

			movl	$100,%ecx		// loop
			movl	drive_id(%ebp),%ebx	// ebx - drive_id
			andl	$0x10,%ebx
.DevLoop:
			movb	%bl,%al
			outb	%al,%dx			// ata_DevHead = drive_id
			inb	%dx,%al			// Get ata_DevHead
			andb	$0x10,%al
			cmpb	%al,%bl
			je	.DevFound
			decl	%ecx
			jne	.DevLoop
			jmp	.Error
.DevFound:	/* Do standard initialisation od the device */
			movl	tr(%ebp),%ebx
			leal	tr_time(%ebx),%eax	// timerequest->tr_time
			movl	io_Device(%ebx),%edx	// timerequest->io_Device
			pushl	%edx
			pushl	%eax
			call	*GetSysTime(%edx)	// Call timer.device
			popl	%eax			// Get tr_time back
			addl	$4,%esp
			
			movl	tv_secs(%eax),%esi	// Get seconds
			addl	$5,%esi			// We'll wait up to 5s
			
.WaitReset:		movl	io_base(%ebp),%edx	// Get drive status
			addl	$ata_Status,%edx
			inb	%dx,%al
			testb	%al,%al
			je	.TestMaster
			testb	$0x80,%al		// BUSY = 1?
			jne	.TestTime		// Yes - wait
			testb	$0x40,%al		// RDY = 1?
			jne	.AutoDetect		// Yes - detect drive
.TestTime:
			leal	tr_time(%ebx),%eax
			movl	io_Device(%ebx),%edx
			pushl	%edx
			pushl	%eax
			call	*GetSysTime(%edx)	// Call timer.device
			popl	%eax			// Get tr_time back
			addl	$4,%esp
			cmpl	tv_secs(%eax),%esi
			jne	.WaitReset
			
/* An error has occured. No drive. */

.Error:
			movl	$100,%ecx
			movl	io_base(%ebp),%edx
			addl	$ata_DevHead,%edx
.DesLoop:
			movb	$0,%al
			outb	%al,%dx
			inb	%dx,%al
			testb	%al,%al
			je	.EndLoop
			decl	%ecx
			jne	.DesLoop
.EndLoop:
			movl	$0,%eax
			popl	%edi
			popl	%ebx
			popl	%esi
			movl	%ebp,%esp
			popl	%ebp
			ret

			
.TestMaster:
			movl	drive_id(%ebp),%edx
			testb	$0x10,%dl
			je	.TestTime

/* Autodetect drive */
.AutoDetect:
			movl	drive_id(%ebp),%edi
			andl	$0x7f,%edi		// ATA - bit 7 = 0
			movl	io_base(%ebp),%esi
			lea	ata_DevHead(%esi),%edx	// set device
			movl	%edi,%eax
			outb	%al,%dx
			lea	ata_Command(%esi),%edx	// Command register
			movb	$ATA_IDENTDEV,%al	// Sned ATA_INITDEV
			outb	%al,%dx			// Do it
			
			movl	$ATA_TimeOut*8,%ecx
			lea	ata_Status(%esi),%edx
.BusyLoop1:
			inb	%dx,%al
			testb	$0x80,%al		// Check BUSY
			je	.BusyOk1		// not busy
			decl	%ecx			// Wait some more time
			jne	.BusyLoop1
			jmp	.Error			// Timeout?
.BusyOk1:
			testb	$0x01,%al		// No error?
			je	.DevOk			// Great!

			orl	$0x80,%edi		// ATAPI - bit 7 = 1
			lea	ata_DevHead(%esi),%edx	// set device
			movl	%edi,%eax
			outb	%al,%dx
			lea	ata_Command(%esi),%edx	// set command
			movb	$ATAPI_IDENTDEV,%al	// ATAPI init
			outb	%al,%dx
			
			movl	$ATAPI_TimeOut*8,%ecx
			lea	ata_Status(%esi),%edx
.BusyLoop2:
			inb	%dx,%al
			testb	$0x80,%al		// Check BUSY
			je	.BusyOk2		// not busy
			decl	%ecx			// Wait some more time
			jne	.BusyLoop2
			jmp	.Error			// Timeout?
.BusyOk2:
			testb	$0x01,%al
			je	.DevOk
			
			andl	$0x10,%edi		// There were some problems
			jmp	.InitArray		// But init as ATA device

/* Get data from buffer. We don't use it  but we have to empty this buffer */
.DevOk:
			movl	%esi,%edx		// Data port
			movl	$128,%ecx		// 128 longwords
.Loop:			inl	%dx,%eax
			decl	%ecx
			jne	.Loop

.InitArray:
			testl	$0x80,%edi
			jne	.AtapiDev
			call	TestLBA
.AtapiDev:
			orl	$0xf0000000,%edi	/* This way even if we have */
			movl	%edi,%eax		/* ATAPI MASTER (code 0x00) */
							/* We'll return non 0 value */
			popl	%edi
			popl	%ebx
			popl	%esi
			movl	%ebp,%esp
			popl	%ebp
			ret
			
/* Tests, whether drive can use LBA mode. Esi points to io_base, Edi is dev_id */

			.type	TestLBA,@function
			.align	16

TestLBA:		leal	ata_DevHead(%esi),%edx
			orl	$0x40,%edi		// Select LBA mode
			movl	%edi,%eax
			outb	%al,%dx			// Select device
			movb	$0,%al
			leal	ata_SectorNum(%esi),%edx
			outb	%al,%dx
			leal	ata_CylinderL(%esi),%edx
			outb	%al,%dx
			leal	ata_CylinderH(%esi),%edx
			outb	%al,%dx
			incb	%al
			leal	ata_SectorCnt(%esi),%edx
			outb	%al,%dx
			movb	$ATA_READ,%al
			leal	ata_Command(%esi),%edx
			outb	%al,%dx

			movl	$ATA_TimeOut*8,%ecx
			leal	ata_Status(%esi),%edx
.BusyLoop3:
			inb	%dx,%al
			testb	$0x80,%al
			je	.BusyOk3
			decl	%ecx
			jne	.BusyLoop3
			jmp	.NoLBA
.BusyOk3:
			testb	$0x08,%al
			je	.NoLBA

			pushl	%eax	
/* Empty the buffer */
			movl	%esi,%edx		// Data port
			movl	$128,%ecx		// 128 longwords
.Loop1:			inl	%dx,%eax
			decl	%ecx
			jne	.Loop1

			popl	%eax
			testb	$0x01,%al
			jne	.NoLBA
			
			movl	$ATA_TimeOut*8,%ecx
			leal	ata_Status(%esi),%edx
.BusyLoop4:
			inb	%dx,%al
			testb	$0x80,%al
			je	.End
			decl	%ecx
			jne	.BusyLoop4
.NoLBA:
			andl	$0xbf,%edi
.End:
			ret

/*
    TestMirror called only with one parameter - the io_base
*/

		        .globl	TestMirror
			.type	TestMirror,@function

			.align	16

TestMirror:		pushl	%ebp
			movl	%esp,%ebp
			pushl	%edx
			pushl	%ecx
			pushl	%ebx
			
			movl	8(%ebp),%edx
			addl	$ata_DevHead,%edx
			movb	$0xa0,%al
			outb	%al,%dx
			movl	8(%ebp),%edx
			addl	$ata_Command,%edx
			movb	$ATA_NOP,%al
			outb	%al,%dx
			
			movl	$ATA_TimeOut*8,%ecx
			movl	8(%ebp),%edx
			addl	$ata_Status,%edx
.BusyLoopTM:
			inb	%dx,%al
			testb	$0x80,%al
			je	.BusyOkTM
			decl	%ecx
			jne	.BusyLoopTM
			jmp	.DecOk
.BusyOkTM:
			movb	%al,%bl
			movl	8(%ebp),%edx
			addl	$ata_DevHead,%edx
			movb	$0xb0,%al
			outb	%al,%dx
			movl	8(%ebp),%edx
			addl	$ata_Status,%edx
			inb	%dx,%al
			cmpb	%al,%bl
			jne	.DecOk
			
			movl	$0,%eax
			jmp	.DecOK1
.DecOk:
			movl	$-1,%eax

.DecOK1:		popl	%ebx
			popl	%ecx
			popl	%edx
			movl	%ebp,%esp
			popl	%ebp
			ret
