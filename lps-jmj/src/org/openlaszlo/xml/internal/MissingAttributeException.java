/* *****************************************************************************
 * MissingAttributeException.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.xml.internal;
import org.jdom.Element;

/**
 * MissingAttributeException
 *
 * @author Oliver Steele
 */
public class MissingAttributeException extends RuntimeException {
    public MissingAttributeException(Element element, String aname) {
        super(aname + " is a required attribute of " + element.getName());
    }
}
