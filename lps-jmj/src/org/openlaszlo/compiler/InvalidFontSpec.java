/* *****************************************************************************
 * InvalidFontSpec.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import java.io.File;
import org.jdom.Element;


/** Represents an exception when an attribute value for font/size/style cannot be understood.
 *
 * @author  Henry Minsky
 */
public class InvalidFontSpec extends CompilationError {

    public InvalidFontSpec() {
        super("INVALID FONTSPEC");
        System.out.println("new InvalidFontSpec ()");
 }

    public InvalidFontSpec(String message, Element element) {
        super(message, element);
        System.out.println("new InvalidFontSpec ("+message+", "+element.getName()+")");
    }

}
