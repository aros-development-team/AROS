/* *****************************************************************************
 * ScriptElementCompiler.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import org.openlaszlo.utils.FileUtils;
import java.io.File;
import java.io.IOException;
import org.jdom.Element;

/** Compiler for <code>script</code> elements.
 *
 * @author  Oliver Steele
 */
class ScriptElementCompiler extends ElementCompiler {
    private static final String SRC_ATTR_NAME = "src";

    ScriptElementCompiler(CompilationEnvironment env) {
        super(env);
    }

    /** Returns true iff this class applies to this element.
     * @param element an element
     * @return see doc
     */
    static boolean isElement(Element element) {
        return element.getName().intern() == "script";
    }

    public void compile(Element element) {
        if (element.hasChildren()) {
            throw new CompilationError("<script> elements can't have children",
                                       element);
        }
        String pathname = null;
        String script = element.getText();
        if (element.getAttribute(SRC_ATTR_NAME) != null) {
            pathname = element.getAttributeValue(SRC_ATTR_NAME);
            File file = mEnv.resolveReference(element, SRC_ATTR_NAME);
            try {
                script = "#file " + element.getAttributeValue(SRC_ATTR_NAME)
                    + "\n" +
                    "#line 1\n" + FileUtils.readFileString(file);
            } catch (IOException e) {
                throw new CompilationError(e);
            }
        }
        
        // Compile scripts to run at construction time in the view
        // instantiation queue.
        try {
            mEnv.compileScript(
                // Provide file info for anonymous function name
                CompilerUtils.sourceLocationDirective(element, true) +
                VIEW_INSTANTIATION_FNAME + 
                "({name: 'script', attrs: " +
                "{script: function () {\n" +
                "#beginContent\n" +
                "#pragma 'scriptElement'\n" +
                CompilerUtils.sourceLocationDirective(element, true) +
                script +
                "\n#endContent\n" +
                // Scripts have no children
                "}}}, 1)",
                element);
        } catch (CompilationError e) {
            // TODO: [2003-01-16] Instead of this, put the filename in ParseException,
            // and modify CompilationError.initElement to copy it from there.
            if (pathname != null) {
                e.setPathname(pathname);
            }
            throw e;
        }
    }
}






