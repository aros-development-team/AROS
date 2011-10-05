/*
    Copyright 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef P_AROS_VERSION_H
#define P_AROS_VERSION_H

/*
    This is AROS specific file used for versioning the Gallium3D interface.
    Since the gallium interface (see DEVELOPMENT:include/gallium) on AROS is
    exposed from gallium hidds to client modules (at this time mesa.library)
    there is a need to make sure that the interface that hidd was compiled with
    is the same as the interface the client was compiled with.
    
    IMPORTANT: With each change of the gallium header files, this value needs to
    be increased. The client will only use the gallium hidd if the versions
    exactly match. This safety is implemented because the Gallium3D interface
    is originally treated as internal and does not need to be backward 
    compatible. In fact, in most of the cases, changes made are not backward 
    compatile.
    
    If this value is not increased with each headers change, it will lead to
    random crashes if client was compiled with different version of interface
    than the hidd.
    
    This values is meant to be compiled in into each gallium hidd driver and
    each gallium hidd driver client.
*/

#define GALLIUM_INTERFACE_VERSION   5

#endif
