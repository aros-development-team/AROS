/*  vsc portable scheduler (c) in 1997 by Volker Barthelmann */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define BSET(array,bit) (array)[(bit)/CHAR_BIT]|=1<<((bit)%CHAR_BIT)
#define BCLR(array,bit) (array)[(bit)/CHAR_BIT]&=~(1<<((bit)%CHAR_BIT))
#define BTST(array,bit) ((array)[(bit)/CHAR_BIT]&(1<<((bit)%CHAR_BIT)))

/* An instruction with LABEL set is preceded with a numbered label. */
/* The number is in <label>.                                        */
#define LABEL 1

/* An instrution with COND_BRANCH set may be followed by the next */
/* instruction or by the instruction with the label in <label>.   */
#define COND_BRANCH 2

/* An instruction with UNCOND_BRANCH set is followed by the */
/* instruction with the label in <label>.                   */
#define UNCOND_BRANCH 4

/* The side-effects of an instruction with BARRIER set cannot entirely */
/* be specified by a struct sinfo (e.g. function-calls, pseudo-ops or  */
/* other complicated things). vsc will not move any code across this   */
/* instruction.                                                        */
#define BARRIER 8

/* An instruction with neither COND_BRANCH, UNCOND_BRANCH nor BARRIER  */
/* set _must_ _not_ change control-flow.                               */

/* Used by vsched.c. */
#define OUT 16
#define READY 32

/* schedule.h has to #define REGS (the maximum number of registers */
/* (numbered from 0 to REGS-1) and the maximum number of pipelines */
/* PIPES (numbered from 0 to PIPES-1). If multiple CPUs with       */
/* different numbers of pipelines or registers are supported the   */
/* largest number must be specified. The CPU-differences must be   */
/* represented by the information provided by sched_info().        */
#include "schedule.h"

/* MEM is a pseudo-register used to indicate that memory is        */
/* accessed.                                                       */ 
#define MEM REGS

#define PIPES_SIZE ((PIPES+CHAR_BIT-1)/CHAR_BIT*CHAR_BIT)
#define REGS_SIZE ((MEM+1+CHAR_BIT-1)/CHAR_BIT*CHAR_BIT)

/* The struct to hold scheduling-information on an instruction. */
struct sinfo {
  /* The instruction in assembly-language. */
  char *txt;

  /* A combination of the flags mentioned above. */
  unsigned int flags;

  /* A numbered label used with LABEL, COND_BRANCH and UNCOND_BRANCH. */
  unsigned int label;

  /* Number of cycles until all side-effects of the operation are */
  /* completed. */
  unsigned int latency;

  /* Bit-vector which contains 1s for every pipeline which can execute */
  /* this instruction. */
  unsigned char pipes[PIPES_SIZE];

  /* Bit-vector which contains 1s for every register that is used by */
  /* this instruction. Use pseudo-register MEM if it reads memory.   */
  unsigned char uses[REGS_SIZE];

  /* Bit-vector which contains 1s for every register that is modified  */
  /* by this instruction. Use pseudo-register MEM if it writes memory. */
  unsigned char modifies[REGS_SIZE];

  /* An ID which identifies the memory-object which is read. 0 means    */
  /* no further information available. Two accesses with different IDs  */
  /* are guaranteed not to access the same memory.                      */
  unsigned long memread_id;

  /* An ID which identifies the memory-object which is written. 0 means */
  /* no further information available. Two accesses with different IDs  */
  /* are guaranteed not to access the same memory.                      */
  unsigned long memwrite_id;

  /* If this flag is set to 1 then the write to the object with the     */
  /* ID specified by memwrite_id completely overwrites the object.      */
  unsigned int memwrite_completely;
};


/* Target-specific data which must be provided by schedule.c. */

/* Copyright notice which is printed if -quiet is not specified. */
extern char tg_copyright[];


/* Target-specific functions which must be provided by schedule.c */

/* This function allows some initializations to be done. If       */
/* zero is returned vsc assumes an error has happened and aborts. */
extern int sched_init(void);

/* This function allows some cleanup before vsc exits. */
extern void sched_cleanup(void);

/* This is the main target-specific function. It will be called with  */
/* a pointer to a struct sinfo that is set to binary zeroes except    */
/* for the member <txt> which points to the assembly representation   */
/* of the instruction.                                                */
/* sched_info() has to fill in all the other members with information */
/* that correctly represents the instruction.                         */ 
/* If the BARRIER-flag is set all other informations are basically    */
/* irrelevant.                                                        */
extern int sched_info(struct sinfo *);



