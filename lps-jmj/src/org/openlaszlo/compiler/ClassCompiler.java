/* *****************************************************************************
 * ClassNode.java
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
import org.openlaszlo.xml.internal.XMLUtils;
import org.openlaszlo.xml.internal.MissingAttributeException;
import org.openlaszlo.xml.internal.Schema;
import org.openlaszlo.xml.internal.Schema.Type;
import org.openlaszlo.sc.ScriptCompiler;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.css.CSSParser;

/** Compiler for <code>class</code> class elements.
 */
class ClassCompiler extends ViewCompiler {
    /**
       For a declaration of a class named "foobar"
       
       <pre>&lt;class name="foobar" extends="view"&gt;</pre>

       We are going to call

       <pre>
       LzInstantiateView(
      {
        name: 'userclass', 
        attrs: {
                parent: "view", 
                initobj: {
                             name: "foobar",
                             attrs: {name: "foobar"},

       </pre>
     */
    static final String DEFAULT_SUPERCLASS_NAME = "view";
    
    ClassCompiler(CompilationEnvironment env) {
        super(env);
    }
    
    /** Returns true iff this class applies to this element.
     * @param element an element
     * @return see doc
     */
    static boolean isElement(Element element) {
        return element.getName().equals("class");
    }
    
    /** Parse out an XML class definition, add the superclass and
     * attribute types to the schema.
     *
     * <p>
     * For each CLASS element, find child ATTRIBUTE tags, and add them
     * to the schema. */
    void updateSchema(Element element, ViewSchema schema, Set visited) {
        Element elt = element;
        
        String classname = elt.getAttributeValue("name");
        String superclass = elt.getAttributeValue("extends");
        
        if (classname == null || classname.equals("")) {
            CompilationError cerr = new CompilationError("The classname attribute, \"name\" must be supplied for a class definition", elt);
            throw(cerr);
        }
        
        if (superclass != null && superclass.equals("")) {
            mEnv.warn("An empty string value is not allowed for the 'extends' attribute on a class definition", elt);
            superclass = null;
        }
        if (superclass == null) {
            // Default to LzView
            superclass = ClassCompiler.DEFAULT_SUPERCLASS_NAME;
        }
        
        ClassModel superclassinfo = schema.getClassModel(superclass);
        if (superclassinfo == null) {
            throw new CompilationError("undefined superclass " + superclass + " for class "+classname, elt);
        }
        
        // Collect up the attribute defs, if any, of this class
        List attributeDefs = new ArrayList();
        Iterator iterator = element.getContent().iterator();
        
        while (iterator.hasNext()) {
            Object o = iterator.next();
            if (o instanceof Element) {
                Element child = (Element) o;
                // Is this an element named ATTRIBUTE which is a
                // direct child of a CLASS tag?
                if (child.getName().equals("attribute")) {
                    String attrName = "";
                    try {
                        attrName = XMLUtils.requireAttributeValue(child, "name");
                    } catch (MissingAttributeException e) {
                        throw new CompilationError(
                            "'name' is a required attribute of <" + child.getName() + ">", child);
                    }
                    
                    String attrTypeName = child.getAttributeValue("type");
                    String attrDefault = child.getAttributeValue("default");
                    String attrSetter = child.getAttributeValue("setter");
                    String attrRequired = child.getAttributeValue("required");
                    
                    ViewSchema.Type attrType;
                    if (attrTypeName == null) {
                        // Check if this attribute exists in ancestor classes,
                        // and if so, default to that type.
                        attrType = superclassinfo.getAttributeType(attrName);
                        if (attrType == null) {
                            // The default attribute type
                            attrType = ViewSchema.EXPRESSION_TYPE;
                        }
                    } else {
                        attrType = schema.getTypeForName(attrTypeName);
                    }
                    
                    if (attrType == null) {
                        throw new CompilationError("In class "+classname+ " type '"+attrTypeName +"', declared for attribute '"+
                                                   attrName + "' is not a known data type.", element);
                    }
                    
                    AttributeSpec attrSpec = 
                        new AttributeSpec(attrName, attrType, attrDefault,
                                          attrSetter, child);
                    if (attrName.equals("text") && attrTypeName != null) {
                        if ("text".equals(attrTypeName))
                            attrSpec.contentType = attrSpec.TEXT_CONTENT;
                        else if ("html".equals(attrTypeName))
                            attrSpec.contentType = attrSpec.HTML_CONTENT;
                    }
                    attributeDefs.add(attrSpec);
                }
            }
        }
        
        // Add this class to the schema
        schema.addElement(element, classname, superclass, attributeDefs);
    }
    
    public void compile(Element elt) {
        String className = XMLUtils.requireAttributeValue(elt, "name");
        String extendsName = XMLUtils.getAttributeValue(
            elt, "extends", DEFAULT_SUPERCLASS_NAME);
        
        ViewSchema schema = mEnv.getSchema();
        ClassModel classModel = schema.getClassModel(className);
        
        String linedir = CompilerUtils.sourceLocationDirective(elt, true);
        ViewCompiler.preprocess(elt, mEnv);
        
        FontInfo fontInfo = new FontInfo(mEnv.getCanvas().getFontInfo());
        if (mEnv.getSWFVersion().equals("swf5")) {
            ViewCompiler.collectInputFonts(elt, mEnv, fontInfo, new HashSet());
        }
        
        // We compile a class declaration just like a view, and then
        // add attribute declarations and perhaps some other stuff that
        // the runtime wants.
        NodeModel model = NodeModel.elementAsModel(elt, schema, mEnv);
        model = model.expandClassDefinitions();
        model.removeAttribute("name");
        classModel.setNodeModel(model);
        Map viewMap = model.asMap();
        
        // Put in the class name, not "class" 
        viewMap.put("name", ScriptCompiler.quote(className));
        
        // Construct a Javascript statement from the initobj map
        String initobj;
        try {
            java.io.Writer writer = new java.io.StringWriter();
            ScriptCompiler.writeObject(viewMap, writer);
            initobj = writer.toString();
        } catch (java.io.IOException e) {
            throw new ChainedException(e);
        }
        // Generate a call to queue instantiation
        StringBuffer buffer = new StringBuffer();
        buffer.append(VIEW_INSTANTIATION_FNAME + 
                      "({name: 'userclass', attrs: " +
                      "{parent: " +
                      ScriptCompiler.quote(extendsName) +
                      ", initobj: " + initobj +
                      "}}" +
                      ", " + ((ElementWithLocationInfo)elt).model.totalSubnodes() +
                      ");\n");
        if (!classModel.getInline()) {
            ClassModel superclassModel = classModel.getSuperclassModel();
            mEnv.compileScript(buffer.toString(), elt);
        }
        
        // TODO: [12-27-2002 ows] use the log4j API instead of this property
        boolean tracexml =
            mEnv.getProperties().getProperty("trace.xml", "false") == "true";
        if (tracexml) {
            Logger mXMLLogger = Logger.getLogger("trace.xml");
            mXMLLogger.info("compiling class definition:");
            org.jdom.output.XMLOutputter outputter =
                new org.jdom.output.XMLOutputter();
            outputter.setTextNormalize(true);
            mXMLLogger.info(outputter.outputString(elt));
            mXMLLogger.info("compiled to:\n" + buffer.toString() + "\n");
        }
    }
}
