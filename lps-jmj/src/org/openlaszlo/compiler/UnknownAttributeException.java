/* *****************************************************************************
 * UnknownAttributeException.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import java.io.File;
import org.jdom.Element;
import org.jdom.JDOMException;
import org.xml.sax.SAXParseException;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.sc.parser.ParseException;

/** Represents an exception when an attribute with unknown type is encountered.
 *
 * @author  Henry Minsky
 */
public class UnknownAttributeException extends RuntimeException {
    /** The element which contains this attribute */
    private String elementName;
    private String attrName;

    public UnknownAttributeException (String elementName, String attrName) {
        this.elementName = elementName;
        this.attrName = attrName;
    }

    public UnknownAttributeException () { }

    public String getElementName() { return elementName; }
    public String getName() { return attrName; }

    public void setElementName(String s) { elementName = s; }
    public void setName(String s) { attrName = s; }
    
    public String getMessage () {
        return "Unknown attribute named "+attrName;
    }

}
