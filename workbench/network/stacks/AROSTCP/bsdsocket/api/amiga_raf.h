/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

#ifndef AMIGA_RAF_H
#define AMIGA_RAF_H

/*
 * RAF1...RAF7 gives consistent way to make function take arguments in
 * registers. currently __GNUC__ and __SASC supported.
 */

#if __SASC

#define RAF1(funname, type1, arg1, reg1)	\
 __asm funname(register __##reg1 type1 arg1)	\
{

#define RAF2(funname,type1, arg1, reg1,type2, arg2, reg2) \
 __asm funname(register __##reg1 type1 arg1,	\
	       register __##reg2 type2 arg2)	\
{

#define RAF3(funname, type1, arg1, reg1, type2, arg2, reg2, type3, arg3, reg3)\
 __asm funname(register __##reg1 type1 arg1,	\
	       register __##reg2 type2 arg2,	\
	       register __##reg3 type3 arg3)	\
{

#define RAF4(funname, type1, arg1, reg1, type2, arg2, reg2, type3, arg3, reg3, type4, arg4, reg4) \
 __asm funname(register __##reg1 type1 arg1,	\
	       register __##reg2 type2 arg2,	\
	       register __##reg3 type3 arg3,	\
	       register __##reg4 type4 arg4)	\
{

#define RAF5(funname, type1, arg1, reg1, type2, arg2, reg2, type3, arg3, reg3, type4, arg4, reg4, type5, arg5, reg5) \
  __asm funname(register __##reg1 type1 arg1,	\
	       register __##reg2 type2 arg2,	\
	       register __##reg3 type3 arg3,	\
	       register __##reg4 type4 arg4,	\
	       register __##reg5 type5 arg5)	\
{

#define RAF6(funname, type1, arg1, reg1, type2, arg2, reg2, type3, arg3, reg3, type4, arg4, reg4, type5, arg5, reg5, type6, arg6, reg6) \
 __asm funname(register __##reg1 type1 arg1,	\
	       register __##reg2 type2 arg2,	\
	       register __##reg3 type3 arg3,	\
	       register __##reg4 type4 arg4,	\
	       register __##reg5 type5 arg5,	\
	       register __##reg6 type6 arg6)	\
{

#define RAF7(funname, type1, arg1, reg1, type2, arg2, reg2, type3, arg3, reg3, type4, arg4, reg4, type5, arg5, reg5, type6, arg6, reg6, type7, arg7, reg7) \
 __asm funname(register __##reg1 type1 arg1,	\
	       register __##reg2 type2 arg2,	\
	       register __##reg3 type3 arg3,	\
	       register __##reg4 type4 arg4,	\
	       register __##reg5 type5 arg5,	\
	       register __##reg6 type6 arg6,	\
	       register __##reg7 type7 arg7)	\
{

#elif __GNUC__

#define RAF1(funname,type1, arg1, reg1) \
  funname(VOID)			    \
{				    \
  register type1 reg1 __asm(#reg1); \
  type1 arg1 = reg1;

#define RAF2(funname, type1, arg1, reg1, type2, arg2, reg2) \
  funname(VOID)			    \
{				    \
  register type1 reg1 __asm(#reg1); \
  type1 arg1 = reg1; 		    \
  register type2 reg2 __asm(#reg2); \
  type2 arg2 = reg2;

#if 0
#define RAF2(funname, type1, arg1, reg1, type2, arg2, reg2) \
  funname(type1 arg1 __asm(#reg1), type2 arg2 __asm(#reg2)) \
{
#endif

#define RAF3(funname, type1, arg1, reg1, type2, arg2, reg2, type3, arg3, reg3)\
  funname(VOID)			    \
{				    \
  register type1 reg1 __asm(#reg1); \
  type1 arg1 = reg1; 		    \
  register type2 reg2 __asm(#reg2); \
  type2 arg2 = reg2; 		    \
  register type3 reg3 __asm(#reg3); \
  type3 arg3 = reg3;                     	

#define RAF4(funname, type1, arg1, reg1, type2, arg2, reg2, type3, arg3, reg3, type4, arg4, reg4)		   \
  funname(VOID)			    \
{				    \
  register type1 reg1 __asm(#reg1); \
  type1 arg1 = reg1; 		    \
  register type2 reg2 __asm(#reg2); \
  type2 arg2 = reg2; 		    \
  register type3 reg3 __asm(#reg3); \
  type3 arg3 = reg3; 		    \
  register type4 reg4 __asm(#reg4); \
  type4 arg4 = reg4;                     	

#define RAF5(funname, type1, arg1, reg1, type2, arg2, reg2, type3, arg3, reg3, type4, arg4, reg4, type5, arg5, reg5) \
  funname(VOID)			    \
{				    \
  register type1 reg1 __asm(#reg1); \
  type1 arg1 = reg1; 		    \
  register type2 reg2 __asm(#reg2); \
  type2 arg2 = reg2; 		    \
  register type3 reg3 __asm(#reg3); \
  type3 arg3 = reg3; 		    \
  register type4 reg4 __asm(#reg4); \
  type4 arg4 = reg4; 		    \
  register type5 reg5 __asm(#reg5); \
  type5 arg5 = reg5;                     	


#define RAF6(funname, type1, arg1, reg1, type2, arg2, reg2, type3, arg3, reg3, type4, arg4, reg4, type5, arg5, reg5, type6, arg6, reg6) \
  funname(VOID)			    \
{				    \
  register type1 reg1 __asm(#reg1); \
  type1 arg1 = reg1; 		    \
  register type2 reg2 __asm(#reg2); \
  type2 arg2 = reg2; 		    \
  register type3 reg3 __asm(#reg3); \
  type3 arg3 = reg3; 		    \
  register type4 reg4 __asm(#reg4); \
  type4 arg4 = reg4; 		    \
  register type5 reg5 __asm(#reg5); \
  type5 arg5 = reg5; 		    \
  register type6 reg6 __asm(#reg6); \
  type6 arg6 = reg6;                     	

#define RAF7(funname, type1, arg1, reg1, type2, arg2, reg2, type3, arg3, reg3, type4, arg4, reg4, type5, arg5, reg5, type6, arg6, reg6, type7, arg7, reg7) \
  funname(VOID)			    \
{				    \
  register type1 reg1 __asm(#reg1); \
  type1 arg1 = reg1; 		    \
  register type2 reg2 __asm(#reg2); \
  type2 arg2 = reg2; 		    \
  register type3 reg3 __asm(#reg3); \
  type3 arg3 = reg3; 		    \
  register type4 reg4 __asm(#reg4); \
  type4 arg4 = reg4; 		    \
  register type5 reg5 __asm(#reg5); \
  type5 arg5 = reg5; 		    \
  register type6 reg6 __asm(#reg6); \
  type6 arg6 = reg6; 		    \
  register type7 reg7 __asm(#reg7); \
  type7 arg7 = reg7;                     	

#endif /* __SASC | __GNUC__ */

#endif /* !defined(AMIGA_RAF_H) */
