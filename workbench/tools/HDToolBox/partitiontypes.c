/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <strings.h>
#include <proto/exec.h>
#include "platform.h"
#define DEBUG 1
#include "debug.h"

#include "partitiontypes.h"

extern struct PartitionTypeNode n00,n01,n02,n03,n04,n05,n06,n07,n08,n09,n0A,n0B,n0C,n0D,n0E,n0F,
	n10,n11,n12,n13,n14,n15,n16,n17,n18,n19,n1A,n1B,n1C,n1D,n1E,n1F,
	n20,n21,n22,n23,n24,n25,n26,n27,n28,n29,n2A,n2B,n2C,n2D,n2E,n2F,
	n30,n31,n32,n33,n34,n35,n36,n37,n38,n39,n3A,n3B,n3C,n3D,n3E,n3F,
	n40,n41,n42,n43,n44,n45,n46,n47,n48,n49,n4A,n4B,n4C,n4D,n4E,n4F,
	n50,n51,n52,n53,n54,n55,n56,n57,n58,n59,n5A,n5B,n5C,n5D,n5E,n5F,
	n60,n61,n62,n63,n64,n65,n66,n67,n68,n69,n6A,n6B,n6C,n6D,n6E,n6F,
	n70,n71,n72,n73,n74,n75,n76,n77,n78,n79,n7A,n7B,n7C,n7D,n7E,n7F,
	n80,n81,n82,n83,n84,n85,n86,n87,n88,n89,n8A,n8B,n8C,n8D,n8E,n8F,
	n90,n91,n92,n93,n94,n95,n96,n97,n98,n99,n9A,n9B,n9C,n9D,n9E,n9F,
	nA0,nA1,nA2,nA3,nA4,nA5,nA6,nA7,nA8,nA9,nAA,nAB,nAC,nAD,nAE,nAF,
	nB0,nB1,nB2,nB3,nB4,nB5,nB6,nB7,nB8,nB9,nBA,nBB,nBC,nBD,nBE,nBF,
	nC0,nC1,nC2,nC3,nC4,nC5,nC6,nC7,nC8,nC9,nCA,nCB,nCC,nCD,nCE,nCF,
	nD0,nD1,nD2,nD3,nD4,nD5,nD6,nD7,nD8,nD9,nDA,nDB,nDC,nDD,nDE,nDF,
	nE0,nE1,nE2,nE3,nE4,nE5,nE6,nE7,nE8,nE9,nEA,nEB,nEC,nED,nEE,nEF,
	nF0,nF1,nF2,nF3,nF4,nF5,nF6,nF7,nF8,nF9,nFA,nFB,nFC,nFD,nFE,nFF;
struct List mbrtypelist={&n00.ln,0, &nFF.ln, NULL, NULL};
struct PartitionTypeNode n00={MAKENODE(&n01.ln,        (struct Node *)&mbrtypelist, "Empty",                    0, 0), "\000",1};
struct PartitionTypeNode n01={MAKENODE(&n02.ln,                            &n00.ln, "FAT12",                    0, 0), "\001",1};
struct PartitionTypeNode n02={MAKENODE(&n03.ln,                            &n01.ln, "XENIX root",               0, 0), "\002",1};
struct PartitionTypeNode n03={MAKENODE(&n04.ln,                            &n02.ln, "XENIX usr",                0, 0), "\003",1};
struct PartitionTypeNode n04={MAKENODE(&n05.ln,                            &n03.ln, "FAT16  <32M",              0, 0), "\004",1};
struct PartitionTypeNode n05={MAKENODE(&n06.ln,                            &n04.ln, "Extended",                 0, 0), "\005",1};
struct PartitionTypeNode n06={MAKENODE(&n07.ln,                            &n05.ln, "FAT16",                    0, 0), "\006",1};
struct PartitionTypeNode n07={MAKENODE(&n08.ln,                            &n06.ln, "HPFS/NTFS/QNX-2 (16 bit)", 0, 0), "\007",1};
struct PartitionTypeNode n08={MAKENODE(&n09.ln,                            &n07.ln, "AIX",                      0, 0), "\010",1};
struct PartitionTypeNode n09={MAKENODE(&n0A.ln,                            &n08.ln, "AIX bootable",             0, 0), "\011",1};
struct PartitionTypeNode n0A={MAKENODE(&n0B.ln,                            &n09.ln, "OS/2 Boot Manager",        0, 0), "\012",1};
struct PartitionTypeNode n0B={MAKENODE(&n0C.ln,                            &n0A.ln, "Win95 FAT32",              0, 0), "\013",1};
struct PartitionTypeNode n0C={MAKENODE(&n0E.ln,                            &n0B.ln, "Win95 FAT32 (LBA)",        0, 0), "\014",1};
struct PartitionTypeNode n0E={MAKENODE(&n0F.ln,                            &n0C.ln, "Win95 FAT16 (LBA)",        0, 0), "\016",1};
struct PartitionTypeNode n0F={MAKENODE(&n10.ln,                            &n0E.ln, "Win95 Ext'd LBA",          0, 0), "\017",1};
struct PartitionTypeNode n10={MAKENODE(&n11.ln,                            &n0F.ln, "OPUS",                     0, 0), "\020",1};
struct PartitionTypeNode n11={MAKENODE(&n14.ln,                            &n10.ln, "Hidden FAT12",             0, 0), "\021",1};
struct PartitionTypeNode n14={MAKENODE(&n16.ln,                            &n11.ln, "Hidden FAT16 <32",         0, 0), "\024",1};
struct PartitionTypeNode n16={MAKENODE(&n17.ln,                            &n14.ln, "Hidden FAT16",             0, 0), "\026",1};
struct PartitionTypeNode n17={MAKENODE(&n18.ln,                            &n16.ln, "Hidden HPFS/NTFS",         0, 0), "\027",1};
struct PartitionTypeNode n18={MAKENODE(&n1B.ln,                            &n17.ln, "AST Windows swap",         0, 0), "\030",1};
struct PartitionTypeNode n1B={MAKENODE(&n1C.ln,                            &n18.ln, "Hidden Win95 FAT?",        0, 0), "\033",1};
struct PartitionTypeNode n1C={MAKENODE(&n1E.ln,                            &n1B.ln, "Hidden Win95 FAT?",        0, 0), "\034",1};
struct PartitionTypeNode n1E={MAKENODE(&n30.ln,                            &n1C.ln, "Hidden Win95 FAT?",        0, 0), "\036",1};
struct PartitionTypeNode n30={MAKENODE(&n39.ln,                            &n1E.ln, "AROS",                     0, 0), "\060",1};
struct PartitionTypeNode n39={MAKENODE(&n3C.ln,                            &n30.ln, "plan9",                    0, 0), "\071",1};
struct PartitionTypeNode n3C={MAKENODE(&n40.ln,                            &n39.ln, "PartitionMagic",           0, 0), "\074",1};
struct PartitionTypeNode n40={MAKENODE(&n4D.ln,                            &n3C.ln, "VENIX 286",                0, 0), "\100",1};
struct PartitionTypeNode n4D={MAKENODE(&n4E.ln,                            &n40.ln, "QNX4.2 Pri",               0, 0), "\115",1};
struct PartitionTypeNode n4E={MAKENODE(&n4F.ln,                            &n4D.ln, "QNX4.2 Sec",               0, 0), "\116",1};
struct PartitionTypeNode n4F={MAKENODE(&n50.ln,                            &n4E.ln, "QNX4.2 Ter",               0, 0), "\117",1};
struct PartitionTypeNode n50={MAKENODE(&n51.ln,                            &n4F.ln, "DM",                       0, 0), "\120",1};
struct PartitionTypeNode n51={MAKENODE(&n52.ln,                            &n50.ln, "DM",                       0, 0), "\121",1};
struct PartitionTypeNode n52={MAKENODE(&n56.ln,                            &n51.ln, "CP/M, MS SysV/AT",         0, 0), "\122",1};
struct PartitionTypeNode n56={MAKENODE(&n61.ln,                            &n52.ln, "GB",                       0, 0), "\126",1};
struct PartitionTypeNode n61={MAKENODE(&n63.ln,                            &n56.ln, "SpeedStor",                0, 0), "\141",1};
struct PartitionTypeNode n63={MAKENODE(&n64.ln,                            &n61.ln, "ISC UNIX/SysV/GNU HURD",   0, 0), "\143",1};
struct PartitionTypeNode n64={MAKENODE(&n65.ln,                            &n63.ln, "Novell Netware 2.xx",      0, 0), "\144",1};
struct PartitionTypeNode n65={MAKENODE(&n70.ln,                            &n64.ln, "Novell Netware 3.xx",      0, 0), "\145",1};
struct PartitionTypeNode n70={MAKENODE(&n75.ln,                            &n65.ln, "DiskSecure Mult",          0, 0), "\160",1};
struct PartitionTypeNode n75={MAKENODE(&n80.ln,                            &n70.ln, "PC/IX",                    0, 0), "\165",1};
struct PartitionTypeNode n80={MAKENODE(&n81.ln,                            &n75.ln, "Old MINIX",                0, 0), "\200",1};
struct PartitionTypeNode n81={MAKENODE(&n82.ln,                            &n80.ln, "Linux/MINIX",              0, 0), "\201",1};
struct PartitionTypeNode n82={MAKENODE(&n83.ln,                            &n81.ln, "Linux swap",               0, 0), "\202",1};
struct PartitionTypeNode n83={MAKENODE(&n84.ln,                            &n82.ln, "Linux native",             0, 0), "\203",1};
struct PartitionTypeNode n84={MAKENODE(&n85.ln,                            &n83.ln, "OS/2 hidden C:",           0, 0), "\204",1};
struct PartitionTypeNode n85={MAKENODE(&n86.ln,                            &n84.ln, "Linux extended",           0, 0), "\205",1};
struct PartitionTypeNode n86={MAKENODE(&n87.ln,                            &n85.ln, "NTFS volume set",          0, 0), "\206",1};
struct PartitionTypeNode n87={MAKENODE(&n8E.ln,                            &n86.ln, "NTFS volume set",          0, 0), "\207",1};
struct PartitionTypeNode n8E={MAKENODE(&n93.ln,                            &n87.ln, "Linux LVM",                0, 0), "\216",1};
struct PartitionTypeNode n93={MAKENODE(&n94.ln,                            &n8E.ln, "Amoeba",                   0, 0), "\223",1};
struct PartitionTypeNode n94={MAKENODE(&n9F.ln,                            &n93.ln, "Amoeba BBT",               0, 0), "\224",1};
struct PartitionTypeNode n9F={MAKENODE(&nA0.ln,                            &n94.ln, "BSD/OS",                   0, 0), "\237",1};
struct PartitionTypeNode nA0={MAKENODE(&nA5.ln,                            &n9F.ln, "IBM Thinkpad hi?",         0, 0), "\240",1};
struct PartitionTypeNode nA5={MAKENODE(&nA6.ln,                            &nA0.ln, "386BSD/FreeBSD/NetBSD",    0, 0), "\245",1};
struct PartitionTypeNode nA6={MAKENODE(&nA7.ln,                            &nA5.ln, "OpenBSD",                  0, 0), "\246",1};
struct PartitionTypeNode nA7={MAKENODE(&nA9.ln,                            &nA6.ln, "NEXTSTEP",                 0, 0), "\247",1};
struct PartitionTypeNode nA9={MAKENODE(&nB7.ln,                            &nA7.ln, "NetBSD",                   0, 0), "\251",1};
struct PartitionTypeNode nB7={MAKENODE(&nB8.ln,                            &nA9.ln, "BSDI fs",                  0, 0), "\267",1};
struct PartitionTypeNode nB8={MAKENODE(&nC1.ln,                            &nB7.ln, "BSDI swap",                0, 0), "\270",1};
struct PartitionTypeNode nC1={MAKENODE(&nC4.ln,                            &nB8.ln, "DRDOS/sec (FAT-?)",        0, 0), "\301",1};
struct PartitionTypeNode nC4={MAKENODE(&nC6.ln,                            &nC1.ln, "DRDOS/sec (FAT-?)",        0, 0), "\304",1};
struct PartitionTypeNode nC6={MAKENODE(&nC7.ln,                            &nC4.ln, "DRDOS/sec (FAT-?)",        0, 0), "\306",1};
struct PartitionTypeNode nC7={MAKENODE(&nDB.ln,                            &nC6.ln, "Syrinx",                   0, 0), "\307",1};
struct PartitionTypeNode nDB={MAKENODE(&nE1.ln,                            &nC7.ln, "Concurrent CPM",           0, 0), "\333",1};
struct PartitionTypeNode nE1={MAKENODE(&nE3.ln,                            &nDB.ln, "DOS access,SpeedStor",     0, 0), "\341",1};
struct PartitionTypeNode nE3={MAKENODE(&nE4.ln,                            &nE1.ln, "DOS R/O,SpeedStor",        0, 0), "\343",1};
struct PartitionTypeNode nE4={MAKENODE(&nEB.ln,                            &nE3.ln, "Speed",                    0, 0), "\344",1};
struct PartitionTypeNode nEB={MAKENODE(&nF1.ln,                            &nE4.ln, "BeOS fs",                  0, 0), "\353",1};
struct PartitionTypeNode nF1={MAKENODE(&nF2.ln,                            &nEB.ln, "SpeedStor",                0, 0), "\361",1};
struct PartitionTypeNode nF2={MAKENODE(&nF4.ln,                            &nF1.ln, "DOS secondary",            0, 0), "\362",1};
struct PartitionTypeNode nF4={MAKENODE(&nFD.ln,                            &nF2.ln, "SpeedStor",                0, 0), "\364",1};
struct PartitionTypeNode nFD={MAKENODE(&nFE.ln,                            &nF4.ln, "Linux raid auto",          0, 0), "\375",1};
struct PartitionTypeNode nFE={MAKENODE(&nFF.ln,                            &nFD.ln, "LANstep",                  0, 0), "\376",1};
struct PartitionTypeNode nFF={MAKENODE((struct Node *)&mbrtypelist.lh_Tail,&nFE.ln, "BBT",                      0, 0), "\377",1};


extern struct PartitionTypeNode rdb00, rdb01, rdb02, rdb03, rdb04;

struct List rdbtypelist={&rdb00.ln,0, &rdb04.ln, NULL, NULL};
struct PartitionTypeNode rdb00={MAKENODE(&rdb01.ln,        (struct Node *)&rdbtypelist, "OldFileSystem" ,         0, 0), "DOS\0", 4};
struct PartitionTypeNode rdb01={MAKENODE(&rdb02.ln,                          &rdb00.ln, "FastFileSystem",         0, 0), "DOS\1", 4};
struct PartitionTypeNode rdb02={MAKENODE(&rdb03.ln,                          &rdb01.ln, "FastFileSystem Intl",    0, 0), "DOS\2", 4};
struct PartitionTypeNode rdb03={MAKENODE(&rdb04.ln,                          &rdb02.ln, "FastFileSystem Intl-DC", 0, 0), "DOS\3", 4};
struct PartitionTypeNode rdb04={MAKENODE((struct Node *)&rdbtypelist.lh_Tail,&rdb03.ln, "SmartFileSystem",        0, 0), "SFS\0", 4};



struct PartitionTypeNode unknown = {MAKENODE(0,0,"Unknown",0,0),0,0};

struct PartitionTypeNode *getPartitionTypeNode
	(
		struct PartitionTableNode *table,
		struct PartitionType *type
	)
{
struct PartitionTypeNode *ptn;

	if (table->typelist)
	{
		ptn = (struct PartitionTypeNode *)table->typelist->lh_Head;
		while (ptn->ln.ln_Succ)
		{
			if (type->id_len == ptn->id_len)
			{
				if (strncmp(type->id, ptn->id, type->id_len)==0)
					return ptn;
			}
			ptn = (struct PartitionTypeNode *)ptn->ln.ln_Succ;
		}
	}
	return &unknown;
}

void setPartitionType(struct PartitionTableNode *table) {

	switch (table->type)
	{
	case PHPTT_RDB:
		table->typelist = &rdbtypelist;
		CopyMem(table->defaulttype.id, "DOS\002", 4);
		table->defaulttype.id_len = 4;
		break;
	case PHPTT_MBR:
		table->typelist = &mbrtypelist;
		table->defaulttype.id[0] = 0x30;
		table->defaulttype.id_len = 1;
		break;
	}
}

