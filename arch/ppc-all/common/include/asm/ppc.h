/*
    Copyright C 2003, The AROS Development Team. All rights reserved
    $Id$

    Desc: PPC definitions and macros
    Lang: English
*/

#ifndef ASM_PPC_H
#define ASM_PPC_H

/* Machine State Register */
#define MSR_POW	0x00040000
#define MSR_ILE	0x00010000
#define MSR_EE	0x00008000
#define MSR_PR	0x00004000
#define MSR_FP	0x00002000
#define MSR_ME	0x00001000
#define MSR_FE0	0x00000800
#define MSR_SE	0x00000400
#define MSR_BE	0x00000200
#define MSR_FE1	0x00000100
#define MSR_IP  0x00000040
#define MSR_IR	0x00000020
#define MSR_DR	0x00000010
#define MSR_RI	0x00000002
#define MSR_LE	0x00000001

/* SPR values */
#define	XER	1
#define LR	8
#define CTR	9
#define DSISR	18
#define	DAR	19
#define DEC	22
#define SDR1	25
#define SRR0	26
#define SRR1	27
#define SPRG0	272
#define SPRG1	273
#define SPRG2	274
#define SPRG3	275
#define EAR	282
#define TBL	284
#define TBU	285
#define	IBAT0U	528
#define IBAT0L	529
#define IBAT1U	530
#define IBAT1L	531
#define IBAT2U	532
#define IBAT2L	533
#define IBAT3U	534
#define IBAT3L	535
#define DBAT0U	536
#define DBAT0L	537
#define DBAT1U	538
#define DBAT1L	539
#define DBAT2U	540
#define DBAT2L	541
#define DBAT3U	542
#define DBAT3L	543
#define DABR	1013

#endif /* ASM_PPC_H */

