/*

Copyright (C) 2017 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/

#ifndef TASK_H
#define TASK_H


#if defined(__mc68000) && !defined(__AROS__)
#define AddUnitTask(task, initial_pc, unit) \
   ({ \
      task->tc_SPReg -= sizeof(APTR); \
      *((APTR *)task->tc_SPReg) = unit; \
      AddTask(task, initial_pc, NULL); \
   })
#endif
#ifdef __amigaos4__
#define AddUnitTask(task, initial_pc, unit) \
   ({ \
      struct TagItem _task_tags[] = \
         {{AT_Param1, (UPINT)unit}, {TAG_END, 0}}; \
      AddTask(task, initial_pc, NULL, _task_tags); \
   })
#endif
#ifdef __MORPHOS__
#define AddUnitTask(task, initial_pc, unit) \
   ({ \
      struct TagItem _task_tags[] = \
      { \
         {TASKTAG_CODETYPE, CODETYPE_PPC}, \
         {TASKTAG_PC, (UPINT)initial_pc}, \
         {TASKTAG_PPC_ARG1, (UPINT)unit}, \
         {TAG_END, 1} \
      }; \
      struct TaskInitExtension _task_init = {0xfff0, 0, _task_tags}; \
      AddTask(task, &_task_init, NULL); \
   })
#endif
#ifdef __AROS__
#define AddUnitTask(task, initial_pc, unit) \
   ({ \
      struct TagItem _task_tags[] = \
         {{TASKTAG_ARG1, (UPINT)unit}, {TAG_END, 0}}; \
      NewAddTask(task, initial_pc, NULL, _task_tags); \
   })
#endif


#endif


