/* *****************************************************************************
 * DataCompiler.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import org.openlaszlo.iv.flash.api.action.*;
import java.io.*;
import org.jdom.Element;
import org.jdom.JDOMException;
import org.openlaszlo.xml.internal.XMLUtils;

/** Compiler for local data elements.
 *
 * @author  Henry Minsky
 * @author  Oliver Steele
 */
class DataCompiler extends ElementCompiler {
    static final String LOCAL_DATA_FNAME = "lzAddLocalData";

    DataCompiler(CompilationEnvironment env) {
        super(env);
    }

    static boolean isElement(Element element) {
        if (element.getName().equals("dataset")) {
            // return type in ('soap', 'http') or src is url
            String src = element.getAttributeValue("src");
            String type = element.getAttributeValue("type");
            if (type != null && (type.equals("soap") || type.equals("http"))) {
                return false;
            }
            if (src != null && src.indexOf("http:") == 0) {
                return false;
            }
            return src == null || !XMLUtils.isURL(src);
        }
        return false;
    }
    
    public void compile(Element element) {
        Element data = element;
        if (element.getAttribute("src") != null) {
            File file = resolveSrcReference(element);
            try {
                Element newdata = new org.jdom.input.SAXBuilder(false)
                    .build(file)
                    .getRootElement();

                data.addContent(newdata);
            } catch (org.jdom.JDOMException e) {
                throw new CompilationError(e);
            } catch (java.lang.OutOfMemoryError e) {
                // The runtime gc is necessary in order for subsequent
                // compiles to succeed.  The System gc is for good
                // luck.
                throw new CompilationError("out of memory", element);
            }
        }

        boolean trimwhitespace = true;
        String trimAttr = element.getAttributeValue("trimwhitespace");
        if (trimAttr != null && trimAttr.equals("false")) {
            trimwhitespace = false;
        }
            
        int flashVersion = mEnv.getSWFVersionInt();

        Program program = org.openlaszlo.xml.internal.DataCompiler.makeProgram(data, flashVersion, trimwhitespace, true);
        // this leaves the value in a variable named "__lzdataroot"
        
        program.push(LOCAL_DATA_FNAME);
        program.push("_level0");
        program.getVar();
        program.push(LOCAL_DATA_FNAME);
        program.getMember();
        program.setVar();

        program.push("__lzdataroot");
        program.getVar();
        program.push(XMLUtils.requireAttributeValue(element, "name"));
        program.push(2);
        program.push(LOCAL_DATA_FNAME);
        program.callFunction();
        program.pop();
        mEnv.getGenerator().addProgram(program);
    }
}
