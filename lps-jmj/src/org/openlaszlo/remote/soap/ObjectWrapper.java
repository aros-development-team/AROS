/* *****************************************************************************
 * ObjectWrapper.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.remote.soap;

import org.w3c.dom.Element;

public class ObjectWrapper {
    public Element mElement;
    public ObjectWrapper(Element element) {
        mElement = element;
    }

    public Element getElement() {
        return mElement;
    }
}
