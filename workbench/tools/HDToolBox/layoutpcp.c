/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/lists.h>
#include <exec/nodes.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>

#include "gadgets.h"

//const char *unknowntxt="unknown";
#define unknowntxt "unknown"
extern struct Node n00,n01,n02,n03,n04,n05,n06,n07,n08,n09,n0A,n0B,n0C,n0D,n0E,n0F,
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
struct List typelist={&n00,0, &nFF, NULL, NULL};
struct Node n00={&n01,(struct Node *)&typelist, "Empty",0,0};
struct Node n01={&n02,&n00, "FAT12", 0, 0};
struct Node n02={&n03,&n01, "XENIX root", 0, 0};
struct Node n03={&n04,&n02, "XENIX usr", 0, 0};
struct Node n04={&n05,&n03, "FAT16  <32M", 0, 0};
struct Node n05={&n06,&n04, "Extended", 0, 0};
struct Node n06={&n07,&n05, "FAT16", 0, 0};
struct Node n07={&n08,&n06, "HPFS/NTFS/QNX-2 (16 bit)", 0, 0};
struct Node n08={&n09,&n07, "AIX", 0, 0};
struct Node n09={&n0A,&n08, "AIX bootable", 0, 0};
struct Node n0A={&n0B,&n09, "OS/2 Boot Manager", 0, 0};
struct Node n0B={&n0C,&n0A, "Win95 FAT32", 0, 0};
struct Node n0C={&n0D,&n0B, "Win95 FAT32 (LBA)", 0, 0};
struct Node n0D={&n0E,&n0C, unknowntxt, 0, 0};
struct Node n0E={&n0F,&n0D, "Win95 FAT16 (LBA)", 0, 0};
struct Node n0F={&n10,&n0E, "Win95 Ext'd LBA", 0, 0};
struct Node n10={&n11,&n0F, "OPUS", 0, 0};
struct Node n11={&n12,&n10, "Hidden FAT12", 0, 0};
struct Node n12={&n13,&n11, unknowntxt, 0, 0};
struct Node n13={&n14,&n12, unknowntxt, 0, 0};
struct Node n14={&n15,&n13, "Hidden FAT16 <32", 0, 0};
struct Node n15={&n16,&n14, unknowntxt, 0, 0};
struct Node n16={&n17,&n15, "Hidden FAT16", 0, 0};
struct Node n17={&n18,&n16, "Hidden HPFS/NTFS", 0, 0};
struct Node n18={&n19,&n17, "AST Windows swap", 0, 0};
struct Node n19={&n1A,&n18, unknowntxt, 0, 0};
struct Node n1A={&n1B,&n19, unknowntxt, 0, 0};
struct Node n1B={&n1C,&n1A, "Hidden Win95 FAT?", 0, 0};
struct Node n1C={&n1D,&n1B, "Hidden Win95 FAT?", 0, 0};
struct Node n1D={&n1E,&n1C, unknowntxt, 0, 0};
struct Node n1E={&n1F,&n1D, "Hidden Win95 FAT?", 0, 0};
struct Node n1F={&n20,&n1E, unknowntxt, 0, 0};
struct Node n20={&n21,&n1F, unknowntxt, 0, 0};
struct Node n21={&n22,&n20, unknowntxt, 0, 0};
struct Node n22={&n23,&n21, unknowntxt, 0, 0};
struct Node n23={&n24,&n22, unknowntxt, 0, 0};
struct Node n24={&n25,&n23, unknowntxt, 0, 0};
struct Node n25={&n26,&n24, unknowntxt, 0, 0};
struct Node n26={&n27,&n25, unknowntxt, 0, 0};
struct Node n27={&n28,&n26, unknowntxt, 0, 0};
struct Node n28={&n29,&n27, unknowntxt, 0, 0};
struct Node n29={&n2A,&n28, unknowntxt, 0, 0};
struct Node n2A={&n2B,&n29, unknowntxt, 0, 0};
struct Node n2B={&n2C,&n2A, unknowntxt, 0, 0};
struct Node n2C={&n2D,&n2B, unknowntxt, 0, 0};
struct Node n2D={&n2E,&n2C, unknowntxt, 0, 0};
struct Node n2E={&n2F,&n2D, unknowntxt, 0, 0};
struct Node n2F={&n30,&n2E, unknowntxt, 0, 0};
struct Node n30={&n31,&n2F, "AROS", 0, 0};
struct Node n31={&n32,&n30, unknowntxt, 0, 0};
struct Node n32={&n33,&n31, unknowntxt, 0, 0};
struct Node n33={&n34,&n32, unknowntxt, 0, 0};
struct Node n34={&n35,&n33, unknowntxt, 0, 0};
struct Node n35={&n36,&n34, unknowntxt, 0, 0};
struct Node n36={&n37,&n35, unknowntxt, 0, 0};
struct Node n37={&n38,&n36, unknowntxt, 0, 0};
struct Node n38={&n39,&n37, unknowntxt, 0, 0};
struct Node n39={&n3A,&n38, "plan9", 0, 0};
struct Node n3A={&n3B,&n39, unknowntxt, 0, 0};
struct Node n3B={&n3C,&n3A, unknowntxt, 0, 0};
struct Node n3C={&n3D,&n3B, "PartitionMagic", 0, 0};
struct Node n3D={&n3E,&n3C, unknowntxt, 0, 0};
struct Node n3E={&n3F,&n3D, unknowntxt, 0, 0};
struct Node n3F={&n40,&n3E, unknowntxt, 0, 0};
struct Node n40={&n41,&n3F, "VENIX 286", 0, 0};
struct Node n41={&n42,&n40, unknowntxt, 0, 0};
struct Node n42={&n43,&n41, unknowntxt, 0, 0};
struct Node n43={&n44,&n42, unknowntxt, 0, 0};
struct Node n44={&n45,&n43, unknowntxt, 0, 0};
struct Node n45={&n46,&n44, unknowntxt, 0, 0};
struct Node n46={&n47,&n45, unknowntxt, 0, 0};
struct Node n47={&n48,&n46, unknowntxt, 0, 0};
struct Node n48={&n49,&n47, unknowntxt, 0, 0};
struct Node n49={&n4A,&n48, unknowntxt, 0, 0};
struct Node n4A={&n4B,&n49, unknowntxt, 0, 0};
struct Node n4B={&n4C,&n4A, unknowntxt, 0, 0};
struct Node n4C={&n4D,&n4B, unknowntxt, 0, 0};
struct Node n4D={&n4E,&n4C, "QNX4.2 Pri", 0, 0};
struct Node n4E={&n4F,&n4D, "QNX4.2 Sec", 0, 0};
struct Node n4F={&n50,&n4E, "QNX4.2 Ter", 0, 0};
struct Node n50={&n51,&n4F, "DM", 0, 0};
struct Node n51={&n52,&n50, "DM", 0, 0};
struct Node n52={&n53,&n51, "CP/M, MS SysV/AT", 0, 0};
struct Node n53={&n54,&n52, unknowntxt, 0, 0};
struct Node n54={&n55,&n53, unknowntxt, 0, 0};
struct Node n55={&n56,&n54, unknowntxt, 0, 0};
struct Node n56={&n57,&n55, "GB", 0, 0};
struct Node n57={&n58,&n56, unknowntxt, 0, 0};
struct Node n58={&n59,&n57, unknowntxt, 0, 0};
struct Node n59={&n5A,&n58, unknowntxt, 0, 0};
struct Node n5A={&n5B,&n59, unknowntxt, 0, 0};
struct Node n5B={&n5C,&n5A, unknowntxt, 0, 0};
struct Node n5C={&n5D,&n5B, unknowntxt, 0, 0};
struct Node n5D={&n5E,&n5C, unknowntxt, 0, 0};
struct Node n5E={&n5F,&n5D, unknowntxt, 0, 0};
struct Node n5F={&n60,&n5E, unknowntxt, 0, 0};
struct Node n60={&n61,&n5F, unknowntxt, 0, 0};
struct Node n61={&n62,&n60, "SpeedStor", 0, 0};
struct Node n62={&n63,&n61, unknowntxt, 0, 0};
struct Node n63={&n64,&n62, "ISC UNIX/SysV/GNU HURD", 0, 0};
struct Node n64={&n65,&n63, "Novell Netware 2.xx", 0, 0};
struct Node n65={&n66,&n64, "Novell Netware 3.xx", 0, 0};
struct Node n66={&n67,&n65, unknowntxt, 0, 0};
struct Node n67={&n68,&n66, unknowntxt, 0, 0};
struct Node n68={&n69,&n67, unknowntxt, 0, 0};
struct Node n69={&n6A,&n68, unknowntxt, 0, 0};
struct Node n6A={&n6B,&n69, unknowntxt, 0, 0};
struct Node n6B={&n6C,&n6A, unknowntxt, 0, 0};
struct Node n6C={&n6D,&n6B, unknowntxt, 0, 0};
struct Node n6D={&n6E,&n6C, unknowntxt, 0, 0};
struct Node n6E={&n6F,&n6D, unknowntxt, 0, 0};
struct Node n6F={&n70,&n6E, unknowntxt, 0, 0};
struct Node n70={&n71,&n6F, "DiskSecure Mult", 0, 0};
struct Node n71={&n72,&n70, unknowntxt, 0, 0};
struct Node n72={&n73,&n71, unknowntxt, 0, 0};
struct Node n73={&n74,&n72, unknowntxt, 0, 0};
struct Node n74={&n75,&n73, unknowntxt, 0, 0};
struct Node n75={&n76,&n74, "PC/IX", 0, 0};
struct Node n76={&n77,&n75, unknowntxt, 0, 0};
struct Node n77={&n78,&n76, unknowntxt, 0, 0};
struct Node n78={&n79,&n77, unknowntxt, 0, 0};
struct Node n79={&n7A,&n78, unknowntxt, 0, 0};
struct Node n7A={&n7B,&n79, unknowntxt, 0, 0};
struct Node n7B={&n7C,&n7A, unknowntxt, 0, 0};
struct Node n7C={&n7D,&n7B, unknowntxt, 0, 0};
struct Node n7D={&n7E,&n7C, unknowntxt, 0, 0};
struct Node n7E={&n7F,&n7D, unknowntxt, 0, 0};
struct Node n7F={&n80,&n7E, unknowntxt, 0, 0};
struct Node n80={&n81,&n7F, "Old MINIX", 0, 0};
struct Node n81={&n82,&n80, "Linux/MINIX", 0, 0};
struct Node n82={&n83,&n81, "Linux swap", 0, 0};
struct Node n83={&n84,&n82, "Linux native", 0, 0};
struct Node n84={&n85,&n83, "OS/2 hidden C:", 0, 0};
struct Node n85={&n86,&n84, "Linux extended", 0, 0};
struct Node n86={&n87,&n85, "NTFS volume set", 0, 0};
struct Node n87={&n88,&n86, "NTFS volume set", 0, 0};
struct Node n88={&n89,&n87, unknowntxt, 0, 0};
struct Node n89={&n8A,&n88, unknowntxt, 0, 0};
struct Node n8A={&n8B,&n89, unknowntxt, 0, 0};
struct Node n8B={&n8C,&n8A, unknowntxt, 0, 0};
struct Node n8C={&n8D,&n8B, unknowntxt, 0, 0};
struct Node n8D={&n8E,&n8C, unknowntxt, 0, 0};
struct Node n8E={&n8F,&n8D, "Linux LVM", 0, 0};
struct Node n8F={&n90,&n8E, unknowntxt, 0, 0};
struct Node n90={&n91,&n8F, unknowntxt, 0, 0};
struct Node n91={&n92,&n90, unknowntxt, 0, 0};
struct Node n92={&n93,&n91, unknowntxt, 0, 0};
struct Node n93={&n94,&n92, "Amoeba", 0, 0};
struct Node n94={&n95,&n93, "Amoeba BBT", 0, 0};
struct Node n95={&n96,&n94, unknowntxt, 0, 0};
struct Node n96={&n97,&n95, unknowntxt, 0, 0};
struct Node n97={&n98,&n96, unknowntxt, 0, 0};
struct Node n98={&n99,&n97, unknowntxt, 0, 0};
struct Node n99={&n9A,&n98, unknowntxt, 0, 0};
struct Node n9A={&n9B,&n99, unknowntxt, 0, 0};
struct Node n9B={&n9C,&n9A, unknowntxt, 0, 0};
struct Node n9C={&n9D,&n9B, unknowntxt, 0, 0};
struct Node n9D={&n9E,&n9C, unknowntxt, 0, 0};
struct Node n9E={&n9F,&n9D, unknowntxt, 0, 0};
struct Node n9F={&nA0,&n9E, "BSD/OS", 0, 0};
struct Node nA0={&nA1,&n9F, "IBM Thinkpad hi?", 0, 0};
struct Node nA1={&nA2,&nA0, unknowntxt, 0, 0};
struct Node nA2={&nA3,&nA1, unknowntxt, 0, 0};
struct Node nA3={&nA4,&nA2, unknowntxt, 0, 0};
struct Node nA4={&nA5,&nA3, unknowntxt, 0, 0};
struct Node nA5={&nA6,&nA4, "386BSD/FreeBSD/NetBSD", 0, 0};
struct Node nA6={&nA7,&nA5, "OpenBSD", 0, 0};
struct Node nA7={&nA8,&nA6, "NEXTSTEP", 0, 0};
struct Node nA8={&nA9,&nA7, unknowntxt, 0, 0};
struct Node nA9={&nAA,&nA8, "NetBSD", 0, 0};
struct Node nAA={&nAB,&nA9, unknowntxt, 0, 0};
struct Node nAB={&nAC,&nAA, unknowntxt, 0, 0};
struct Node nAC={&nAD,&nAB, unknowntxt, 0, 0};
struct Node nAD={&nAE,&nAC, unknowntxt, 0, 0};
struct Node nAE={&nAF,&nAD, unknowntxt, 0, 0};
struct Node nAF={&nB0,&nAE, unknowntxt, 0, 0};
struct Node nB0={&nB1,&nAF, unknowntxt, 0, 0};
struct Node nB1={&nB2,&nB0, unknowntxt, 0, 0};
struct Node nB2={&nB3,&nB1, unknowntxt, 0, 0};
struct Node nB3={&nB4,&nB2, unknowntxt, 0, 0};
struct Node nB4={&nB5,&nB3, unknowntxt, 0, 0};
struct Node nB5={&nB6,&nB4, unknowntxt, 0, 0};
struct Node nB6={&nB7,&nB5, unknowntxt, 0, 0};
struct Node nB7={&nB8,&nB6, "BSDI fs", 0, 0};
struct Node nB8={&nB9,&nB7, "BSDI swap", 0, 0};
struct Node nB9={&nBA,&nB8, unknowntxt, 0, 0};
struct Node nBA={&nBB,&nB9, unknowntxt, 0, 0};
struct Node nBB={&nBC,&nBA, unknowntxt, 0, 0};
struct Node nBC={&nBD,&nBB, unknowntxt, 0, 0};
struct Node nBD={&nBE,&nBC, unknowntxt, 0, 0};
struct Node nBE={&nBF,&nBD, unknowntxt, 0, 0};
struct Node nBF={&nC0,&nBE, unknowntxt, 0, 0};
struct Node nC0={&nC1,&nBF, unknowntxt, 0, 0};
struct Node nC1={&nC2,&nC0, "DRDOS/sec (FAT-?)", 0, 0};
struct Node nC2={&nC3,&nC1, unknowntxt, 0, 0};
struct Node nC3={&nC4,&nC2, unknowntxt, 0, 0};
struct Node nC4={&nC5,&nC3, "DRDOS/sec (FAT-?)", 0, 0};
struct Node nC5={&nC6,&nC4, unknowntxt, 0, 0};
struct Node nC6={&nC7,&nC5, "DRDOS/sec (FAT-?)", 0, 0};
struct Node nC7={&nC8,&nC6, "Syrinx", 0, 0};
struct Node nC8={&nC9,&nC7, unknowntxt, 0, 0};
struct Node nC9={&nCA,&nC8, unknowntxt, 0, 0};
struct Node nCA={&nCB,&nC9, unknowntxt, 0, 0};
struct Node nCB={&nCC,&nCA, unknowntxt, 0, 0};
struct Node nCC={&nCD,&nCB, unknowntxt, 0, 0};
struct Node nCD={&nCE,&nCC, unknowntxt, 0, 0};
struct Node nCE={&nCF,&nCD, unknowntxt, 0, 0};
struct Node nCF={&nD0,&nCE, unknowntxt, 0, 0};
struct Node nD0={&nD1,&nCF, unknowntxt, 0, 0};
struct Node nD1={&nD2,&nD0, unknowntxt, 0, 0};
struct Node nD2={&nD3,&nD1, unknowntxt, 0, 0};
struct Node nD3={&nD4,&nD2, unknowntxt, 0, 0};
struct Node nD4={&nD5,&nD3, unknowntxt, 0, 0};
struct Node nD5={&nD6,&nD4, unknowntxt, 0, 0};
struct Node nD6={&nD7,&nD5, unknowntxt, 0, 0};
struct Node nD7={&nD8,&nD6, unknowntxt, 0, 0};
struct Node nD8={&nD9,&nD7, unknowntxt, 0, 0};
struct Node nD9={&nDA,&nD8, unknowntxt, 0, 0};
struct Node nDA={&nDB,&nD9, unknowntxt, 0, 0};
struct Node nDB={&nDC,&nDA, "Concurrent CPM", 0, 0};
struct Node nDC={&nDD,&nDB, unknowntxt, 0, 0};
struct Node nDD={&nDE,&nDC, unknowntxt, 0, 0};
struct Node nDE={&nDF,&nDD, unknowntxt, 0, 0};
struct Node nDF={&nE0,&nDE, unknowntxt, 0, 0};
struct Node nE0={&nE1,&nDF, unknowntxt, 0, 0};
struct Node nE1={&nE2,&nE0, "DOS access,SpeedStor", 0, 0};
struct Node nE2={&nE3,&nE1, unknowntxt, 0, 0};
struct Node nE3={&nE4,&nE2, "DOS R/O,SpeedStor", 0, 0};
struct Node nE4={&nE5,&nE3, "Speed", 0, 0};
struct Node nE5={&nE6,&nE4, unknowntxt, 0, 0};
struct Node nE6={&nE7,&nE5, unknowntxt, 0, 0};
struct Node nE7={&nE8,&nE6, unknowntxt, 0, 0};
struct Node nE8={&nE9,&nE7, unknowntxt, 0, 0};
struct Node nE9={&nEA,&nE8, unknowntxt, 0, 0};
struct Node nEA={&nEB,&nE9, unknowntxt, 0, 0};
struct Node nEB={&nEC,&nEA, "BeOS fs", 0, 0};
struct Node nEC={&nED,&nEB, unknowntxt, 0, 0};
struct Node nED={&nEE,&nEC, unknowntxt, 0, 0};
struct Node nEE={&nEF,&nED, unknowntxt, 0, 0};
struct Node nEF={&nF0,&nEE, unknowntxt, 0, 0};
struct Node nF0={&nF1,&nEF, unknowntxt, 0, 0};
struct Node nF1={&nF2,&nF0, "SpeedStor", 0, 0};
struct Node nF2={&nF3,&nF1, "DOS secondary", 0, 0};
struct Node nF3={&nF4,&nF2, unknowntxt, 0, 0};
struct Node nF4={&nF5,&nF3, "SpeedStor", 0, 0};
struct Node nF5={&nF6,&nF4, unknowntxt, 0, 0};
struct Node nF6={&nF7,&nF5, unknowntxt, 0, 0};
struct Node nF7={&nF8,&nF6, unknowntxt, 0, 0};
struct Node nF8={&nF9,&nF7, unknowntxt, 0, 0};
struct Node nF9={&nFA,&nF8, unknowntxt, 0, 0};
struct Node nFA={&nFB,&nF9, unknowntxt, 0, 0};
struct Node nFB={&nFC,&nFA, unknowntxt, 0, 0};
struct Node nFC={&nFD,&nFB, unknowntxt, 0, 0};
struct Node nFD={&nFE,&nFC, "Linux raid auto", 0, 0};
struct Node nFE={&nFF,&nFD, "LANstep", 0, 0};
struct Node nFF={(struct Node *)&typelist.lh_Tail,&nFE, "BBT", 0, 0};

struct List partition_list;
struct TagItem pcpartitiontags[] =
{
	{GTLV_Labels, 0},
	{GTLV_ShowSelected, 0},
	{TAG_DONE, NULL}
};
struct TagItem pcpaddpartitiontags[] =
{
	{TAG_DONE,NULL}
};
struct TagItem pcpdeletepartitiontags[] =
{
	{GA_Disabled, TRUE},
	{TAG_DONE,NULL}
};
struct TagItem pcpstartcyltags[] =
{
	{GA_Disabled, TRUE},
	{GTIN_Number, 0},
	{TAG_DONE,NULL}
};
struct TagItem pcpendcyltags[] =
{
	{GA_Disabled, TRUE},
	{GTIN_Number, 0},
	{TAG_DONE,NULL}
};
struct TagItem pcptotalcyltags[] =
{
	{GA_Disabled, TRUE},
	{GTIN_Number, 0},
	{TAG_DONE,NULL}
};
struct TagItem pcpeditarospartitiontags[] =
{
	{GA_Disabled, TRUE},
	{TAG_DONE,NULL}
};
struct TagItem pcptypelvtags[] =
{
	{GA_Disabled, TRUE},
	{GTLV_Labels,(ULONG)&typelist},
	{GTLV_MakeVisible, 0},
	{GTLV_Selected, ~0},
	{GTLV_ShowSelected, 0},
	{TAG_DONE,NULL}
};
struct TagItem pcptypeintegertags[] =
{
	{GA_Disabled, TRUE},
	{GTIN_Number, 0},
	{TAG_DONE,NULL}
};
struct TagItem pcpoktags[] =
{
	{TAG_DONE,NULL}
};
struct TagItem pcpcanceltags[] =
{
	{TAG_DONE,NULL}
};



struct creategadget pcpgadgets[] =
{
	{
		LISTVIEW_KIND,
		{
			20,90,280,60,
			NULL, NULL,
			ID_PCP_PARTITION, NULL, NULL, NULL
		},
		pcpartitiontags
	},
	{
		BUTTON_KIND,
		{
			20,150,93,20,
			"Add", NULL,
			ID_PCP_ADD_PARTITION, PLACETEXT_IN, NULL, NULL
		},
		pcpaddpartitiontags
	},
	{
		BUTTON_KIND,
		{
			113,150,94,20,
			"Delete", NULL,
			ID_PCP_DELETE_PARTITION, PLACETEXT_IN, NULL, NULL
		},
		pcpdeletepartitiontags
	},
	{
		BUTTON_KIND,
		{
			207,150,93,20,
			"Edit", NULL,
			ID_PCP_EDIT_PARTITION, PLACETEXT_IN, NULL, NULL
		},
		pcpeditarospartitiontags
	},
	{
		INTEGER_KIND,
		{
			155,180,80,15,
			"Start Cyl", NULL,
			ID_PCP_STARTCYL, PLACETEXT_LEFT, NULL, NULL
		},
		pcpstartcyltags
	},
	{
		INTEGER_KIND,
		{
			155,200,80,15,
			"End Cyl", NULL,
			ID_PCP_ENDCYL, PLACETEXT_LEFT, NULL, NULL
		},
		pcpendcyltags
	},
	{
		INTEGER_KIND,
		{
			155,220,80,15,
			"Total Cyl", NULL,
			ID_PCP_TOTALCYL, PLACETEXT_LEFT, NULL, NULL
		},
		pcptotalcyltags
	},
	{
		LISTVIEW_KIND,
		{
			330,90,290,120,
			NULL, NULL,
			ID_PCP_TYPELV, PLACETEXT_LEFT, NULL, NULL
		},
		pcptypelvtags
	},
	{
		INTEGER_KIND,
		{
			330,210,100,15,
			NULL, NULL,
			ID_PCP_TYPEINTEGER, NULL, NULL, NULL
		},
		pcptypeintegertags
	},
	{
		BUTTON_KIND,
		{
			20,190,50,20,
			"Ok", NULL,
			ID_PCP_OK, PLACETEXT_IN, NULL, NULL
		},
		pcpoktags
	},
	{
		BUTTON_KIND,
		{
			20,215,50,20,
			"Cancel", NULL,
			ID_PCP_CANCEL, PLACETEXT_IN, NULL, NULL
		},
		pcpcanceltags
	}
};
