/* *****************************************************************************
 * LibraryCompiler.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import java.io.*;
import java.util.*;
import org.jdom.Document;
import org.jdom.Element;
import org.openlaszlo.utils.ChainedException;

/** Compiler for <code>library</code> elements.
 *
 * @author  Oliver Steele
 */
class LibraryCompiler extends ToplevelCompiler {
    final static String HREF_ANAME = "href";
    
    LibraryCompiler(CompilationEnvironment env) {
        super(env);
    }

    static boolean isElement(Element element) {
        return element.getName().equals("library");
    }

    // TODO: [ows] this duplicates functionality in Parser
    static File resolveLibraryName(File file)
    {
        if (file.isDirectory()) {
            file = new File(file, "library.lzx");
        }
        return file;
    }

    /** Return the library element and add the library to visited.  If
     * the library has already been visited, return null instead.
     */
    static Element resolveLibraryElement(File file,
                                         CompilationEnvironment env,
                                         Set visited,
                                         boolean validate)
    {
        try {
            file = resolveLibraryName(file);
            File key = file.getCanonicalFile();
            if (!visited.contains(key)) {
                visited.add(key);
                Document doc = env.getParser().parse(file);
                if (validate)
                    Parser.validate(doc, file.getPath(), env);
                Element root = doc.getRootElement();
                return root;
            } else {
                return null;
            }
        } catch (IOException e) {
            throw new CompilationError(e);
        }
    }
    
    /** Return the resolved library element and add the library to visited.
     * If the library has already been visited, return null instead.
     */
    static Element resolveLibraryElement(Element element,
                                         CompilationEnvironment env,
                                         Set visited,
                                         boolean validate)
    {
        String href = element.getAttributeValue(HREF_ANAME);
        if (href == null) {
            return element;
        }
        File file = env.resolveReference(element, HREF_ANAME);
        return resolveLibraryElement(file, env, visited, validate);
    }
    
    public void compile(Element element) throws CompilationError
    {
        element = resolveLibraryElement(
            element, mEnv, mEnv.getImportedLibraryFiles(),
            mEnv.getBooleanProperty(mEnv.VALIDATE_PROPERTY));
        if (element != null && !mEnv.mRecursiveCompilation) {
            super.compile(element);
        }
    }

    void updateSchema(Element element, ViewSchema schema, Set visited) {
        element = resolveLibraryElement(element, mEnv, visited, false);
        if (element != null) {
            super.updateSchema(element, schema, visited);
            // TODO [hqm 2005-02-09] can we compare any 'proxied' attribute here
            // with the parent element (canvas) to warn if it conflicts.
        }
    }
}
