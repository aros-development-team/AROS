/* Copyright (c) 2005 by Pavel Fedin.
 * Copyright (C) 2005 - 2026 The AROS Dev Team
 */
#ifndef MIAMI_API_H
#define MIAMI_API_H

struct MiamiBase {
  struct Library Lib;
  struct SocketBase *_SocketBase;
  struct Library *_UserGroupBase;
  UBYTE DynNameServ_Locked;
  UBYTE DynDomain_Locked;
};

#endif /* !MIAMI_API_H */
