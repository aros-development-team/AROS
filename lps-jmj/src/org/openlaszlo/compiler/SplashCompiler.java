/* *****************************************************************************
 * SplashCompiler.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

import java.io.*;
import java.util.*;

import org.apache.log4j.*;
import org.jdom.Element;

/** Compiler for <code>preloader</code> elements.
 *
 */
class SplashCompiler extends ElementCompiler {
    /** Logger */
    private static Logger mLogger  = org.apache.log4j.Logger.getLogger(SplashCompiler.class);
    private ViewSchema mSchema;
    private static final String VIEW_INSTANTIATION_FNAME = "_root.lzpreloader.create";

    SplashCompiler(CompilationEnvironment env) {
        super(env);
        mSchema = env.getSchema();
    }

    /** Returns true iff this class applies to this element.
     * @param element an element
     * @return see doc
     */
    static boolean isElement(Element element) {
        return element.getName().equals("splash")
            || element.getName().equals("preloader");
    }

    public void compile(Element element) throws CompilationError {
        ViewCompiler viewCompiler = new ViewCompiler(mEnv);
        ResourceCompiler res = new ResourceCompiler(mEnv);
        StringBuffer script = new StringBuffer();

        boolean persistent = "true".equals(element.getAttributeValue("persistent"));
        element.removeAttribute("persistent");
        String hideafterinit = new Boolean(!persistent).toString();
        element.setAttribute("hideafterinit", hideafterinit);

        if (element.getChild("view", element.getNamespace()) == null) {
            // Add default preloader
            ElementWithLocationInfo child = new ElementWithLocationInfo("view", element.getNamespace());
            child.initSourceLocator(((ElementWithLocationInfo)element).getSourceLocator() );
            child.setAttribute("hideafterinit", hideafterinit);
            child.setAttribute("resource", "defaultpreloader.swf");
            child.setAttribute("center", "true");
            child.setAttribute("ratio", "100%");
            child.setAttribute("name", "lzprelresource");
            child.setAttribute("resourcename", "lzprelresource");
            element.addContent(child);
        }
        
        SWFWriter sw = mEnv.getGenerator();
        sw.addPreloader(hideafterinit, mEnv);
        
        for (Iterator iter = element.getChildren("view", element.getNamespace()).iterator();
             iter.hasNext(); ) {
            Element child = (Element) iter.next();
            // Change the child into the format that the runtime expects
            // TODO: [2003-01-13 ows] change the runtime to expect the
            // format that the schema defines instead
            child.setName("preloadresource");
            if (child.getAttribute("ratio") != null) {
                String str = child.getAttributeValue("ratio").trim();
                double value = 100;
                if (str.endsWith("%")) {
                    str = str.substring(0, str.length() - 1);
                    value = 1;
                }
                try {
                    value *= new Float(str).floatValue();
                } catch (NumberFormatException e) {
                    throw new CompilationError(child, e);
                }
                child.setAttribute("synctoload", "true");
                child.setAttribute("lastframe", "" + value);
                child.setAttribute("lastframe", "100");
                child.removeAttribute("ratio");
            }
            if (child.getAttribute("resource") != null) {
                File file = mEnv.resolveReference(child, "resource");
                String rname = child.getAttributeValue("resourcename");
                if (rname == null) {
                    rname = child.getAttributeValue("name");
                    if (rname == null) {
                        rname = sw.createName();
                    } 
                    child.setAttribute("name", rname);
                }
                try {
                    sw.importPreloadResource(file.toString(), rname);
                } catch (SWFWriter.ImportResourceError e) {
                    mEnv.warn(e, element);
                }
                child.setAttribute("resourcename", rname);
                child.removeAttribute("resource");
            }
            if (child.getAttribute("synchronized") != null) {
                child.setAttribute("synctoload", child.getAttributeValue("synchronized"));
                child.removeAttribute("synchronized");
            }
        }
        NodeModel model = NodeModel.elementAsModel(element, mSchema, mEnv);
        script.append(VIEW_INSTANTIATION_FNAME + "(" +
                          model.asJavascript() +
                          ");" );
        
        String scriptstr = script.toString();
        if (scriptstr != "") {
            try {
                sw.addPreloaderScript(scriptstr);
            } catch (org.openlaszlo.sc.CompilerException e) {
                throw new CompilationError(element, e);
            }
            mLogger.debug("Adding preloader script: " + script);
        }
    }
    
    private String[] getBlogList() {
        return new String[] {
            "http://wetmachine.com/",
            "http://osteele.com/",
            "http://www.davidtemkin.com/",
            "http://www.ultrasaurus.com/",
            "http://pt.withy.org/ptalk/"
        };
    }
}
