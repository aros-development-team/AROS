/* *****************************************************************************
 * ElementWithLocationInfo.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

import org.jdom.Namespace;

// TODO [2003-05-03 ptw] Rename this ElementAnnotated
// [ows] or AnnotatedElement

/**  A subclass of JDOM Element which can store info about the
 * location in the XML source file that this it parsed from. 
 */
public class ElementWithLocationInfo extends org.jdom.Element {
    SourceLocator locator = new SourceLocator();
    String mHTMLContent = null;
    // The compiler model object that represents this element
    NodeModel model = null;

    public static SourceLocator getSourceLocator(org.jdom.Element elt) {
        try {
            return ((ElementWithLocationInfo) elt).getSourceLocator();
        } catch (ClassCastException e) {
            return null;
        }
    }

    public ElementWithLocationInfo (String name) {
        super(name);
    }

    public ElementWithLocationInfo (String name, Namespace ns) {
        super(name, ns);
    }

    public ElementWithLocationInfo (String name, String uri) {
        super(name, uri);
    }

    public ElementWithLocationInfo (String name, String prefix, String uri) {
        super(name, prefix, uri);
    }
    
    public void initSourceLocator(SourceLocator locator) {
        this.locator = locator;
    }

    public SourceLocator getSourceLocator() {
        return this.locator;
    }

    public void setHTMLContent(String s) {
        mHTMLContent = s;
    }

    public String getHTMLContent() {
        return mHTMLContent;
    }

}
