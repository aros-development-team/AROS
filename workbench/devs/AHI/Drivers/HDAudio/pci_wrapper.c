/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

(C) Copyright xxxx-2009 Davy Wentzler.
(C) Copyright 2009-2010 Stephen Jones.

The Initial Developer of the Original Code is Davy Wentzler.

All Rights Reserved.
*/

#include <exec/types.h>

#if defined(__AROS__)
#include "pci_aros.c"
#elif defined(__AMIGAOS4__)
#include "pci_aos4.c"
#else
#include "pci_openpci.c"
#endif

