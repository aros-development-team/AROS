/******************************************************************************
 * ResourceCompiler.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

import org.openlaszlo.xml.internal.XMLUtils;
import org.openlaszlo.sc.ScriptCompiler;
import org.jdom.Element;
import java.io.File;
import java.io.FileNotFoundException;
import java.util.*;
import org.apache.log4j.*;

/** 
 * Compiler for resource and audio elements.
 *
 * @author Oliver Steele
 */
class ResourceCompiler extends ElementCompiler {

    Logger mLogger = Logger.getLogger(ResourceCompiler.class);

    ResourceCompiler(CompilationEnvironment env) {
        super(env);
    }

    /** Returns true iff this class applies to this element.
     * @param element an element
     * @return see doc
     */
    static boolean isElement(Element element) {
        String name = element.getName();
        return name.equals("resource")
            || name.equals("audio");
    }

    /*
     * @see <code>CompilerNode</code>
     * @param env a compilation environment
     */
    public void compile(Element element) 
        throws CompilationError
    {
        File file = null;

        if (element.hasChildren()) {
            file = mEnv.resolveParentReference(element);
        } else {
            file = mEnv.resolveReference(element);
        }
        
        if (element.getAttribute("src") != null) {
            Element info = new Element("resolve");
            info.setAttribute("src", element.getAttributeValue("src"));
            try {
                info.setAttribute("pathname", file.getCanonicalPath());
            } catch (java.io.IOException ioe) {
                mLogger.warn("Can't canonicalize " + file.toString());
            }
            mEnv.getCanvas().addInfo(info);
        }
        
        String tagName = element.getName();
        try {
            if (tagName.equals("resource") || tagName.equals("preloadresource")) {
                String name = element.getAttributeValue("name");
                String src = element.getAttributeValue("src");

                if (name == null) {
                    throw new CompilationError("You must supply a value for the 'name' attribute, and it must be a valid Javascript identifier.", element);
                }

                if (!ScriptCompiler.isIdentifier(name)) {
                    mEnv.warn("Resource names must be valid Javascript identifiers. The resource name '"+name+"' is not a valid Javascript identifier", element);
                }
                
                // TODO: [1-02-2003 ows]
                // XMLUtils.requireAttributeValue should work here,
                // and the caller should add the source location info
                if (name == null)
                    throw new CompilationError("resource name is required", element);

                Set resourceNames = mEnv.getResourceNames();
                if (resourceNames.contains(name)) {
                    mEnv.warn("The resource name '"+name+"' has already been defined", element);
                }
                resourceNames.add(name);

                // N.B.: Resources are always imported into the main
                // program for the Flash target, hence the use of
                // getResourceGenerator below
                if (src == null) {
                    List sources = new Vector();
                    for (Iterator iter = element.getChildren("frame", element.getNamespace()).iterator();
                         iter.hasNext(); ) {
                        Element child = (Element) iter.next();
                        mEnv.resolveReference(child);
                        
                        // throw error if 'src' attribute not found
                        XMLUtils.requireAttributeValue(child, "src");
                        File pathname = mEnv.resolveReference(child);
                        sources.add(pathname.getAbsolutePath());
                        
                        Element rinfo = new Element("resolve");
                        rinfo.setAttribute("src", child.getAttributeValue("src"));
                        rinfo.setAttribute("pathname", pathname.toString());
                        mEnv.getCanvas().addInfo(rinfo);
                    }
                    if (!sources.isEmpty()) {
                        if (tagName.equals("preloadresource")) {
                            mEnv.getResourceGenerator().importPreloadResource(sources, name, file);
                        } else {
                            mEnv.getResourceGenerator().importResource(sources, name, file);
                        }
                    } else {
                        throw new CompilationError("required src or children", element);
                    }
                } else {
                    if (tagName.equals("preloadresource")) {
                        mEnv.getResourceGenerator().importPreloadResource(file.getAbsolutePath(), 
                                                           name);
                    } else {
                        mEnv.getResourceGenerator().importResource(file.getAbsolutePath(), 
                                                           name);
                    }
                }
                return;
            } else if (tagName.equals("audio")) {
                String name = XMLUtils.requireAttributeValue(element, "name"); 
                mEnv.getResourceGenerator().importResource(file.getAbsolutePath(),
                                                   name);
                return;
            } else {
                // Program error: this shouldn't happen
                throw new RuntimeException("Unknown resource tag: " +
                                           element.getName()); 
            }
        } catch (SWFWriter.ImportResourceError e) {
            mEnv.warn(e, element);
            return;
        }
    }
}
