/* *****************************************************************************
 * CanvasCompiler.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

import java.io.*;
import java.util.*;

import org.openlaszlo.compiler.ViewCompiler.*;
import org.openlaszlo.compiler.ViewSchema.ColorFormatException;
import org.openlaszlo.sc.*;
import org.openlaszlo.server.*;
import org.openlaszlo.utils.*;
import org.jdom.*;
import org.apache.log4j.*;

/** Compiler for the <code>canvas</code> element. */
class CanvasCompiler extends ToplevelCompiler {
    /** Logger */
    private static Logger mLogger = Logger.getLogger(CanvasCompiler.class);

    CanvasCompiler(CompilationEnvironment env) {
        super(env);
    }
    
    static boolean isElement(Element element) {
        return element.getName().equals("canvas");
    }
    
    // Apps are proxied by default.
    public static boolean APP_PROXIED_DEFAULT = true;

    public void compile(Element element) throws CompilationError
    {
        Canvas canvas = new Canvas();
        // query arg
        String lzproxied = mEnv.getProperty(mEnv.PROXIED_PROPERTY);
        // canvas attribute
        String cproxied = element.getAttributeValue("proxied");

        // Set the "proxied" flag for this app.
        // canvas attribute overrides passed in arg, warn for conflict
        if (cproxied != null && !cproxied.equals("inherit")) {
            if (lzproxied != null && !lzproxied.equals(cproxied)) {
                mEnv.warn("The canvas attribute 'proxied="+cproxied +
                          "' conflicts with the 'lzproxied="+lzproxied +
                          "' query arg" , element);
            }
            canvas.setProxied(cproxied.equals("true"));
        } else {
            // inherit from lzproxied arg, or default to APP_PROXIED_PROPERTY
            if (lzproxied != null) {
                canvas.setProxied(lzproxied.equals("true"));
            } else {
                canvas.setProxied(APP_PROXIED_DEFAULT);
            }
        }

        String versionNumber = element.getAttributeValue("version");
        if (versionNumber != null) {
            String msg = "The canvas is declared with version=\"" + versionNumber + "\".  This version of the LPS compiles version 1.1 files.  This applicaton may not behave as intended when compiled with this version of the product.";
            if (versionNumber.equals("1.1"))
                ;
            else if (versionNumber.equals("1.0"))
                mEnv.warn(msg + "  It is recommended that you run it in debug mode and fix all compiler and debugger warnings, and that you read the migration guide in the developer documentation.", element);
            else
                mEnv.warn(msg, element);
        }
        
        String baseLibraryName = getBaseLibraryName(mEnv);
        String baseLibraryBecause = "Required for all applications";

        // TODO [2004-06-02]: explanation for debug attribute and request
        // parameter
        
        mEnv.getGenerator().importBaseLibrary(baseLibraryName, mEnv);
        
        canvas.setSWFVersion(mEnv.getSWFVersion());
        initializeFromElement(canvas, element);
        
        // Default to true, embed fonts in swf file
        boolean embedFonts = true;
        String embed = element.getAttributeValue(CompilationEnvironment.EMBEDFONTS_PROPERTY);
        if ("false".equals(embed)) {
            embedFonts = false;
        }
        mEnv.setEmbedFonts(embedFonts);
        
        Map map = createCanvasObject(element, canvas);
        String script;
        try {
            java.io.Writer writer = new java.io.StringWriter();
            writer.write("canvas = new LzCanvas(");
            ScriptCompiler.writeObject(map, writer);
            writer.write(");"); 
            script = writer.toString();
        } catch (java.io.IOException e) {
            throw new ChainedException(e);
        }
        if (mEnv.getCanvas() != null) {
            throw new CompilationError("An application may only have one canvas tag.  Check included files for a duplicate canvas tag", element);
        }
        
        mEnv.setCanvas(canvas, script);
        
        // Compile (import) canvas's fonts first
        for (Iterator iter = 
                 element.getChildren("font", element.getNamespace()).iterator(); 
             iter.hasNext(); ) {
            Compiler.compileElement((Element)iter.next(), mEnv);
        }
        
        handleAutoincludes(mEnv, element);
        
        // Compile child elements
        for (Iterator iter = element.getChildren().iterator();
             iter.hasNext(); ) {
            Element child = (Element) iter.next();
            // fonts were compiled above
            // createCanvasObject, so ignore them here
            if (!NodeModel.isPropertyElement(child) && !FontCompiler.isElement(child)) {
                Compiler.compileElement(child, mEnv);
            }
        }
    }
        
    void updateSchema(Element element, ViewSchema schema, Set visited) {
        for (Iterator iter = getLibraries(element).iterator();
             iter.hasNext(); ) {
            File file = (File) iter.next();
            Compiler.updateSchemaFromLibrary(file, mEnv, schema, visited);
        }
        // Visit the children:
        super.updateSchema(element, schema, visited);
    }
    
    private Map createCanvasObject(Element element, Canvas canvas) {
        NodeModel model =
            NodeModel.elementOnlyAsModel(element, mEnv.getSchema(), mEnv);
        Set visited = new HashSet();
        for (Iterator iter = getLibraries(element).iterator();
             iter.hasNext(); ) {
            File file = (File) iter.next();
            Element library = LibraryCompiler.resolveLibraryElement(
                file, mEnv, visited, false);
            if (library != null) {
                collectObjectProperties(library, model, visited);
            }
        }
        collectObjectProperties(element, model, visited);
        model.updateAttrs();
        Map attrs = model.attrs;
        setDimension(attrs, "width", canvas.getWidth());
        setDimension(attrs, "height", canvas.getHeight());
        
        attrs.put("build", ScriptCompiler.quote(LPS.getBuild()));
        attrs.put("lpsversion", ScriptCompiler.quote(LPS.getVersion()));
        attrs.put("lpsrelease", ScriptCompiler.quote(LPS.getRelease()));
        attrs.put("runtime", ScriptCompiler.quote(canvas.getSWFVersion()));
        attrs.put("__LZproxied",
                  ScriptCompiler.quote(
                      mEnv.getProperty(mEnv.PROXIED_PROPERTY, APP_PROXIED_DEFAULT ? "true" : "false")));
        
        attrs.put("embedfonts", Boolean.toString(mEnv.getEmbedFonts()));
        attrs.put("bgcolor", new Integer(canvas.getBGColor()));
        FontInfo fontInfo = canvas.getFontInfo();
        attrs.put("fontname", ScriptCompiler.quote(fontInfo.getName()));
        attrs.put("fontsize", new Integer(fontInfo.getSize()));
        attrs.put("fontstyle", ScriptCompiler.quote(fontInfo.getStyle()));
        if (element.getAttribute("id") != null)
            attrs.put("id", ScriptCompiler.quote(element.getAttributeValue("id")));
        // Remove this so that Debug.write works in canvas methods.
        attrs.remove("debug");
        // Remove this since it isn't a JavaScript expression.
        attrs.remove("libraries");
        return attrs;
    }
    
    protected void setDimension(Map attrs, String name, int value) {
        String strval = (String) attrs.get(name);
        if (strval != null && isPercentageDimension(strval))
            attrs.put(name, ScriptCompiler.quote(strval));
        else
            attrs.put(name, new Integer(value));
    }
    
    protected boolean isPercentageDimension(String str) {
        return str.matches("\\s*(?:\\d+[.\\d*]|.\\d+)%\\s*");
    }
    
    /** 
     * Initializes the canvas from the Element and
     * removes any "special" children elements that
     * should not be compiled.
     *
     * @param elt element that contains the canvas
     */
    public void initializeFromElement(Canvas canvas, Element elt) {
        final String BGCOLOR_ATTR_NAME = "bgcolor";
        boolean resizable = false;
        
        String width = elt.getAttributeValue("width");
        String height = elt.getAttributeValue("height");
        String bgcolor = elt.getAttributeValue(BGCOLOR_ATTR_NAME);
        String title = elt.getAttributeValue("title");
        
        if (width != null) {
            if (isPercentageDimension(width)) {
                resizable = true;
                canvas.setWidthString(width);
            } else {
                try {
                    canvas.setWidth(Integer.parseInt(width));
                } catch (NumberFormatException e) {
                    throw new CompilationError(elt, "width", e);
                }
            }
        }
        if (height != null) {
            if (isPercentageDimension(height)) {
                resizable = true;
                canvas.setHeightString(height);
            } else {
                try {
                    canvas.setHeight(Integer.parseInt(height));
                } catch (NumberFormatException e) {
                    throw new CompilationError(elt, "height", e);
                }
            }
        }
        if (resizable && canvas.getSWFVersion().equals("swf5")) {
            mEnv.warn("Percentage values for canvas width and height are not compatible with the swf5 runtime.");
        }
        if (bgcolor != null) {
            try {
                canvas.setBGColor(ViewSchema.parseColor(bgcolor));
            } catch (ColorFormatException e) {
                throw new CompilationError(elt, BGCOLOR_ATTR_NAME, e);
            }
        }
        if (title != null) {
            canvas.setTitle(title);
        }
        
        // Persistent connection parameters
        canvas.initializeConnection(elt);
        
        String version = elt.getAttributeValue("version");
        if (version != null && !version.equals(canvas.DEFAULT_VERSION)) {
            if (version.equals("1.0")) {
                // TODO: [2003-10-25 bloch] these should come from a
                // properties file
                canvas.defaultFont = "lztahoe8";
                canvas.defaultFontFilename = "lztahoe8.ttf";
                canvas.defaultBoldFontFilename = "lztahoe8b.ttf";
                canvas.defaultItalicFontFilename = "lztahoe8i.ttf";
                // NOTE: [2003-10-30 bloch] we don't have lztahoe8bi yet
                // But 1.0 didn't either so this is prolly ok.
                canvas.defaultBoldItalicFontFilename = "lztahoe8bi.ttf"; 
            }
        }
        
        String font      = elt.getAttributeValue("font");
        String fontstyle = elt.getAttributeValue("fontstyle");
        String fontsize  = elt.getAttributeValue("fontsize");
        if (font == null || font.equals("")) {
            font = canvas.defaultFont();
        }
        if (fontstyle == null || fontstyle.equals("")) {
            fontstyle = canvas.DEFAULT_FONTSTYLE;
        }
        if (fontsize == null || fontsize.equals("") ) {
            fontsize = canvas.defaultFontsize();
        }
        
        canvas.setFontInfo(new FontInfo(font, fontsize, fontstyle));
        
        // The width of the "root" Flash output text resource. Since
        // it's fixed at compile time, this gives the developer a way
        // to specify it. Defaults to canvas width if not specified.
        String maxtextwidth = elt.getAttributeValue("maxtextwidth");
        if (maxtextwidth != null) {
            try {
                canvas.setMaxTextWidth(Integer.parseInt(maxtextwidth));
            } catch (NumberFormatException e) {
                throw new CompilationError(elt, "maxtextwidth", e);
            }
        }
        
        // The height of the "root" Flash output text resource. Since
        // it's fixed at compile time, this gives the developer a way
        // to specify it. Defaults to canvas height if not specified.
        String maxtextheight = elt.getAttributeValue("maxtextheight");
        if (maxtextheight != null) {
            try {
                canvas.setMaxTextHeight(Integer.parseInt(maxtextheight));
            } catch (NumberFormatException e) {
                throw new CompilationError(elt, "maxtextheight", e);
            }
        }
    }
    
    private void collectObjectProperties(Element element, NodeModel model,
                                         Set visited) {
        for (Iterator iter = element.getChildren().iterator();
             iter.hasNext(); ) {
            Element child = (Element) iter.next();
            if (NodeModel.isPropertyElement(child)) {
                model.addPropertyElement(child);
            } else if (LibraryCompiler.isElement(child)) {
                Element libraryElement = LibraryCompiler.resolveLibraryElement(
                    child, mEnv, visited, false);
                if (libraryElement != null) {
                    collectObjectProperties(libraryElement, model, visited);
                }
            }
        }
    }
}
