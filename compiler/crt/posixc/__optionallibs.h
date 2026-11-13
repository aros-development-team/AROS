/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
*/

/* Functions in this include should be used in rest of posixc.library to check
   if functions of a certain library may be used. If not, alternative should
   be provided.
*/

int __usergroup_available(struct PosixCIntBase *PosixCBase);
int __entropy_available(struct PosixCIntBase *PosixCBase);
int __dos64_available(struct PosixCIntBase *PosixCBase);
int __optionallibs_close(struct PosixCIntBase *PosixCBase);
