struct MiamiBase {
  struct Library Lib;
  struct SocketBase *_SocketBase;
  struct Library *_UserGroupBase;
  UBYTE DynNameServ_Locked;
  UBYTE DynDomain_Locked;
};

