/* *****************************************************************************
 * SwitchCompiler.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/


package org.openlaszlo.compiler;
import java.util.*;
import org.apache.log4j.*;
import org.jdom.Element;

/** Compiler for <code>switch</code> elements.
 */
class SwitchCompiler extends ToplevelCompiler {
    static final String WHEN = "when";
    static final String OTHERWISE = "otherwise";
    
    SwitchCompiler(CompilationEnvironment env) {
        super(env);
    }
    
    static boolean isElement(Element element) {
        return element.getName().equals("switch");
    }
    
    public void compile(Element element) {
        for (Iterator iter = element.getChildren(WHEN, element.getNamespace()).iterator();
             iter.hasNext(); ) {
            Element child = (Element) iter.next();
            if (evaluateConditions(child)) {
                compileChildren(child);
                return;
            }
        }
        for (Iterator iter = element.getChildren(OTHERWISE, element.getNamespace()).iterator();
             iter.hasNext(); ) {
            Element child = (Element) iter.next();
            compileChildren(child);
            return;
        }
    }
    
    protected void compileChildren(Element element) {
        for (Iterator iter = element.getChildren().iterator();
             iter.hasNext(); ) {
            Element child = (Element) iter.next();
            Compiler.compileElement(child, mEnv);
        }
    }
    
    void updateSchema(Element element, ViewSchema schema, Set visited)
    {
        for (Iterator iter = element.getChildren(WHEN, element.getNamespace()).iterator();
             iter.hasNext(); ) {
            Element child = (Element) iter.next();
            if (evaluateConditions(child)) {
                updateChildren(child, schema, visited);
                return;
            }
        }
        for (Iterator iter = element.getChildren(OTHERWISE, element.getNamespace()).iterator();
             iter.hasNext(); ) {
            Element child = (Element) iter.next();
            updateChildren(child, schema, visited);
            return;
        }
    }
    
    protected void updateChildren(Element element, ViewSchema schema, Set visited) {
        for (Iterator iter = element.getChildren().iterator();
             iter.hasNext(); ) {
            Element child = (Element) iter.next();
            Compiler.updateSchema(child, mEnv, schema, visited);
        }
    }
    
    protected boolean evaluateConditions(Element element) {
        if (element.getAttribute("runtime") != null
            && !element.getAttributeValue("runtime").equals(mEnv.getSWFVersion())) {
            return false;
        }
        return true;
    }
}
