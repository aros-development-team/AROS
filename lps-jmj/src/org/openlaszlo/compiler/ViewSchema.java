/* ****************************************************************************
 * ViewSchema.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import java.io.*;
import java.util.*;
import org.apache.oro.text.regex.*;
import org.jdom.Document;
import org.jdom.Attribute;
import org.jdom.Element;
import org.jdom.Namespace;
import org.jdom.output.XMLOutputter;
import org.jdom.input.SAXBuilder;
import org.jdom.JDOMException;
import org.openlaszlo.xml.internal.Schema;
import org.openlaszlo.xml.internal.XMLUtils;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.server.*;

/** A schema that describes a Laszlo XML file. */
public class ViewSchema extends Schema {
    private static final Set sInputTextElements = new HashSet();
    private static final Set sHTMLContentElements = new HashSet();

    /** The location of the base Laszlo RELAXNG schema */
    private final String SCHEMA_PATH = LPS.HOME() + File.separator +
        "WEB-INF" + File.separator + 
        "lps" + File.separator + 
        "schema"  + File.separator + "lzx.rng";

    private Document schemaDOM = null;

    private static Document sCachedSchemaDOM;
    private static long sCachedSchemaLastModified;
    
    /** Default table of attribute name -> typecode */
    private static final Map sAttributeTypes = new HashMap();

    /** Mapping of RNG type names -> LPS Types */
    private static final Map sRNGtoLPSTypeMap = new HashMap();

    /** {String} */
    private static final Set sMouseEventAttributes;
    
    /** Maps a class (name) to its ClassModel. Holds info about
     * attribute/types for each class, as well as pointer to the
     * superclass if any.
     */
    private final Map mClassMap = new HashMap();

    /** Type of script expressions. */
    public static final Type EXPRESSION_TYPE = newType("expression");
    public static final Type REFERENCE_TYPE = newType("reference");
    /** Type of event bodies. */
    public static final Type EVENT_TYPE = newType("script");

    /** Type of attribute setter function */
    public static final Type SETTER_TYPE = newType("setter");

    /** Type of tokens. */
    public static final Type TOKEN_TYPE = newType("token");
    public static final Type COLOR_TYPE = newType("color");
    public static final Type NUMBER_EXPRESSION_TYPE = newType("numberExpression");
    public static final Type SIZE_EXPRESSION_TYPE = newType("sizeExpression");
    public static final Type CSS_TYPE = newType("css");
    public static final Type INHERITABLE_BOOLEAN_TYPE = newType("inheritableBoolean");
    
    static {
        sHTMLContentElements.add("text");
        sInputTextElements.add("inputtext");

        // Define mapping from RNG Schema types to LPS types
        sRNGtoLPSTypeMap.put("ID",               ID_TYPE);
        sRNGtoLPSTypeMap.put("anyURI",           STRING_TYPE);
        sRNGtoLPSTypeMap.put("boolean",          EXPRESSION_TYPE);
        sRNGtoLPSTypeMap.put("booleanLiteral",   EXPRESSION_TYPE);
        sRNGtoLPSTypeMap.put("inheritableBooleanLiteral",   INHERITABLE_BOOLEAN_TYPE);
        sRNGtoLPSTypeMap.put("color",            COLOR_TYPE);
        sRNGtoLPSTypeMap.put("colorLiteral",     COLOR_TYPE);
        sRNGtoLPSTypeMap.put("css",              CSS_TYPE);
        sRNGtoLPSTypeMap.put("double",           NUMBER_TYPE);
        sRNGtoLPSTypeMap.put("enumeration",      STRING_TYPE);
        sRNGtoLPSTypeMap.put("expression",       EXPRESSION_TYPE);
        sRNGtoLPSTypeMap.put("float",            NUMBER_TYPE);
        sRNGtoLPSTypeMap.put("integer",          NUMBER_TYPE);
        sRNGtoLPSTypeMap.put("number",           NUMBER_TYPE);
        sRNGtoLPSTypeMap.put("numberLiteral",    NUMBER_TYPE);
        sRNGtoLPSTypeMap.put("numberExpression", NUMBER_EXPRESSION_TYPE);
        sRNGtoLPSTypeMap.put("propertyPath",     STRING_TYPE);
        sRNGtoLPSTypeMap.put("reference",        REFERENCE_TYPE);
        sRNGtoLPSTypeMap.put("script",           EVENT_TYPE);
        sRNGtoLPSTypeMap.put("size",             SIZE_EXPRESSION_TYPE);
        sRNGtoLPSTypeMap.put("sizeLiteral",      SIZE_EXPRESSION_TYPE);
        sRNGtoLPSTypeMap.put("sizeExpression",   SIZE_EXPRESSION_TYPE);
        sRNGtoLPSTypeMap.put("string",           STRING_TYPE);
        sRNGtoLPSTypeMap.put("token",            TOKEN_TYPE);
        sRNGtoLPSTypeMap.put("opacity",          NUMBER_TYPE);

        // from http://www.w3.org/TR/REC-html40/interact/scripts.html
        String[] mouseEventAttributes = {
            "onclick", "ondblclick", "onmousedown", "onmouseup", "onmouseover",
            "onmousemove", "onmouseout"};
        String[] eventAttributes = {
            "onkeypress", "onstart" , "onstop",
            "onfocus", "onblur",
            "onkeydown", "onkeyup", "onsubmit", "onreset", "onselect",
            "onchange" , "oninit", "onerror", "ondata", "ontimeout", 
            "oncommand" , "onapply" , "onremove"};
        setAttributeTypes(mouseEventAttributes, EVENT_TYPE);
        setAttributeTypes(eventAttributes, EVENT_TYPE);
        sMouseEventAttributes = new HashSet(Arrays.asList(mouseEventAttributes));
    }

    private static final String AUTOINCLUDES_PROPERTY_FILE =
        LPS.getMiscDirectory() + File.separator +
        "lzx-autoincludes.properties";
    public static final Properties sAutoincludes = new Properties();
    
    static {
        try {
            InputStream is = new FileInputStream(AUTOINCLUDES_PROPERTY_FILE);
            try {
                sAutoincludes.load(is);
            } finally {
                is.close();
            }
        } catch (java.io.IOException e) {
            throw new ChainedException(e);
        }
    }
    
    /** Set the attributes to the type.
     * @param attributes a list of attributes
     * @param type a type
     */
    private static void setAttributeTypes(String[] attributes, Type type) {
        for (int i = 0; i < attributes.length; i++) {
            sAttributeTypes.put(attributes[i].intern(), type);
        }
    }

    public AttributeSpec findSimilarAttribute (String className, String attributeName) {
        ClassModel info = getClassModel(className);

        if (info != null) {
            return info.findSimilarAttribute(attributeName);
        } else {
            // Check other classes....
            return null;
        }
    }

    /** Set the attribute to the given type, for a specific element */
    public void setAttributeType (Element elt, String classname, String attrName, AttributeSpec attrspec) {
        ClassModel classModel = getClassModel(classname);
        if (classModel == null) {
            throw new RuntimeException("undefined class: " + classname);
        }
        if (classModel.attributeSpecs.get(attrName) != null) {
            throw new CompilationError("duplicate definition of attribute " + classname + "." + attrName, elt);
        }
        classModel.attributeSpecs.put(attrName, attrspec);

        if (attrName.equals("text")) {
            classModel.supportsTextAttribute = true;
        }
    }

    public void addMethodDeclaration (Element elt, String classname, String methodName, String arglist) {
        ClassModel classModel = getClassModel(classname);
        if (classModel == null) {
            throw new RuntimeException("undefined class: " + classname);
        }
        if (classModel.methods.get(methodName) != null) {
            throw new CompilationError("duplicate definition of method " + classname + "." + methodName, elt);
        }
        classModel.methods.put(methodName, arglist);
    }

    public String getSuperclassName(String className) {
        ClassModel model = getClassModel(className);
        if (model == null)
            return null;
        return model.getSuperclassName();
    }
    
    /**
     * @return the base class of a user defined class
     */
    public String getBaseClassname(String className) {
        String ancestor = getSuperclassName(className);
        String superclass = ancestor;

        while (ancestor != null) {
            if (ancestor.equals(className)) {
                throw new CompilationError(
                    "recursive class definition on "+className);
            }
            superclass = ancestor;
            ancestor = getSuperclassName(ancestor);
        }
        return superclass;
    }

    /** Does this class or its ancestors have this attribute declared for it? */
    AttributeSpec getClassAttribute (String classname, String attrName) {
        // OK, walk up the superclasses, checking for existence of this attribute
        ClassModel info = getClassModel(classname);
        if (info == null) {
            return null;
        } else {
            return info.getAttribute(attrName);
        }
    }

    /**
     * Add in the XML to the Schema class definition MYCLASS,
     * to add this list of attribute declarations.
     *
     * @param sourceElement the user's LZX source file element that holds class LZX definition 
     * @param classname the class we are defining
     * @param classDef the RNG class declaration
     * @param attributeDefs list of AttributeSpec attribute info to add to the Schema
     *
     * This is confusing because we are modifying the RNG XML tree,
     * not the actual lzx source code tree.  So it is confusing to see
     * JDOM Elements floating around and not be able to tell whether
     * they pertain to the LZX source or the RNG tree. Only
     * <i>sourceElement</i> is from the lzx source, we have it here so
     * we can identify the source file and line # in case of an error.
     */
    void addAttributeDefs (Element sourceElement, String classname,
                           Element classDef, List attributeDefs)
    {
        /*
          <element>
            CHILDREN
          </element>

          ==>

          <element>
            NEWATTRS
            CHILDREN
          </element>
        */     
        if (!attributeDefs.isEmpty()) {
            Namespace ns = classDef.getNamespace();
            List attrList = new ArrayList();
            for (Iterator iter = attributeDefs.iterator(); iter.hasNext();) {
                AttributeSpec attr = (AttributeSpec) iter.next();

                // If this attribute does not already occur someplace
                // in an ancestor, then let's add it to the schema, so
                // that the validation phase will know about it.
                //
                // We don't want to splice this attribute into the
                // schema if it already is present in an ancestor,
                // because that causes a fatal error ("duplicate
                // attribute") when parsing the schema.
                
                //System.out.println("getClassAttribute( "+ classname+", "+attr.name+ ") = "  + classInheritsAttribute(classname, attr.name));
                if (getClassAttribute(classname, attr.name) == null) {
                    // Splice some XML into the Schema element
                    Element newAttr = new Element("attribute", ns);
                    newAttr.setAttribute("name", attr.name);
                    /* Datatype needs to be a ref to a laszlo type
                       definition, so we generate this RELAX XML for
                       the attribute:
                       
                       <optional>
                         <attribute name="rotation" a:defaultValue="0">
                           <ref name="numberExpression"/>
                         </attribute>
                       </optional>

                       except for the base RELAX type 'string', for
                       which there is no ref, we generate this:

                       <optional>
                         <attribute name="foo">
                           <data type="string"/>
                         </attribute>
                       </optional>
                    */
                    String attrTypeName = attr.type.toString();
                    Element datatype;
                    if (attrTypeName.equals("string") || attrTypeName.equals("boolean")) {
                        datatype = new Element("data", ns);
                        datatype.setAttribute("type", attrTypeName);
                    } else {
                        datatype = new Element("ref", ns);
                        datatype.setAttribute("name", attrTypeName);
                    }
                    // assemble the <attribute> node
                    newAttr.addContent(datatype);
                    // Wrap it in an <optional>
                    Element optional = new Element("optional", ns);
                    optional.addContent(newAttr);
                    // If it's an <attribute name="text" type="text">, add this
                    // instead:
                    //   <ref name="textAttributes"/>
                    if (attr.contentType != attr.NO_CONTENT) {
                        optional = new Element("ref", ns);
                        if (attr.contentType == attr.TEXT_CONTENT) {
                            optional.setAttribute("name", "textContent");
                        } else if (attr.contentType == attr.HTML_CONTENT) {
                            optional.setAttribute("name", "htmlContent");
                        } else {
                            throw new RuntimeException("unknown content type");
                        }
                    }
                    attrList.add(optional);
                } else {
                    // Check that the overriding type is the same as the superclass' type
                    Type parentType = getAttributeType(classname, attr.name);

                    if (parentType != attr.type) {
                        throw new CompilationError(sourceElement, attr.name, new Throwable("In class '" + classname + "' attribute '"+ attr.name + "' with type '"+attr.type.toString() +
                                                   "' is overriding superclass attribute with same name but different type: "+
                                                   parentType.toString()));
                    }
                }

                // Update the in-memory attribute type table
                setAttributeType(sourceElement, classname, attr.name, attr);
            }

            if (!attrList.isEmpty()) {
                // Now splice the attribute list to the start of the class declaration node
                // Remove the original children of the class node
                List children = new ArrayList();
                for (Iterator iter = classDef.getChildren().iterator(); iter.hasNext();) {
                    Element child = (Element) iter.next();
                    children.add(child);
                    // The only way to detach the child from the
                    // parent is to use the iterator.remove() method
                    // otherwise you get a ConcurrentModificationException
                    iter.remove(); 
                }
                // Add in attributes
                classDef.setContent(attrList);
                // Then add back the old children
                for (Iterator iter = children.iterator(); iter.hasNext();) {
                    Element child = (Element) iter.next();
                    classDef.addContent(child);
                }
            }
        }
    }

    /**
     * Add a new element to the attribute type map.
     *
     * Modifies the in-core schema DOM tree as well, to clone the superclass node.
     *
     * @param element the element to add to the map
     * @param superclass an element to inherit attribute to type info from. May be null.
     * @param attributeDefs list of attribute name/type defs
     */
    public void addElement (Element elt, String className,
                            String superclassName, List attributeDefs)
    {
        ClassModel superclass = getClassModel(superclassName);

        if (superclass == null) {
            throw new CompilationError("undefined superclass " + superclassName + " for class "+className);
        }

        if (mClassMap.get(className) != null) {
            String builtin = "builtin ";
            String also = "";
            Element other = getClassModel(className).definition;
            if (other != null) {
                builtin = "";
                also = "; also defined at " + Parser.getSourceMessagePathname(other) + ":" + Parser.getSourceLocation(other, Parser.LINENO);
            }
            throw new CompilationError("duplicate class definitions for " + builtin + className + also, elt);
        }
        ClassModel info = new ClassModel(className, superclass, this, elt);
        mClassMap.put(className, info);

        if (sInputTextElements.contains(superclassName)) {
            info.isInputText = true;
            info.hasInputText = true;
        } else {
            info.isInputText = superclass.isInputText;
            info.hasInputText = superclass.hasInputText;
        }

        info.supportsTextAttribute = superclass.supportsTextAttribute;


        // Modify the RELAX schema DOM to add this element. We find the superclass and
        // clone it, and modify its tag name.
        
        /*
          Say we are defining a new class "myclass extends somesuperclass":

          We look for some XML in the Schema which defines the
          "somesuperclass" class, which will look like
            <element name="somesuperclass"> .... </element>
          or else
            <element>
              <choice>
                <name>...</name>.
                <name>somesuperclass</name>
              </choice>
            </element>

          We then need to clone the superclass node, change its name
          to "myclass", and replace the original superclass node with
          a CHOICE of
            <choice>
              <..orginal superclass node..>
              <..our new class node..>
            </choice>

          This involves detach()'ing the superclass node, cloning it,
          and then making the new choice node and adding it back into
          the parent.
        */

        // Find the superclass definition
        Element superclassDef = findSchemaClassDef(superclassName, schemaDOM.getRootElement());
        if (superclassDef == null) {
            throw new CompilationError("No definition for superclass "+superclassName+" found in the schema");
        }

        Namespace ns = superclassDef.getNamespace();
        // Clone the superclass's schema definition
        Element myclassDef  = (Element) superclassDef.clone();

        // Update the map of classnames to defining Elements
        classElementTable.put(className, myclassDef);

        // Add in the attribute declarations
        addAttributeDefs(elt, className, myclassDef, attributeDefs);
        // Change "name" attritbute (if there is one) to className.
        // If there's no "name" attribute, we look for the CHOICE
        // child node that has NAME children and remove it.

        // Change the name to our new classname
        Attribute nameAttr = myclassDef.getAttribute("name");
        if (nameAttr == null) {
            // OK, it's not of the form <element name="classname">, so
            // it is of the form <element>
            // <choice><name>classname</name></choice>...</element>.
            //
            // We need to remove the "choice" child and add a "name"
            // attribute

            // +++ We don't necessarily want to remove the first
            // 'choice' node, we want to remove the one which has
            // 'name' children.

            for (Iterator iter = myclassDef.getChildren("choice", ns).iterator(); iter.hasNext();) {
                Element choice = (Element) iter.next();
                List names = choice.getChildren("name", ns);
                if (!names.isEmpty()) {
                    // it contains one or more <name> elements, so
                    // remove this whole 'choice' node from its parent
                    iter.remove();
                    break;
                }
            }
        }

        myclassDef.setAttribute("name", className);

        /* Now detach the superclass node and put in it's place
             <choice>
               <SUPERCLASSNODE>
               <MYNEWCLASSNODE>
             </choice>
        */

        Element parent = superclassDef.getParent();
        if (parent.getName().equals("choice"))
            parent.addContent(myclassDef);
        else {
            superclassDef.detach();
            Element newChoice = new Element("choice", ns);
            newChoice.addContent(superclassDef);
            newChoice.addContent(myclassDef);
            parent.addContent(newChoice);
        }

        /*
          if (definition != null) {
          // Look through the definition for children who are or
          // have text elements, and set info.hasInputText
          // accordingly
          info.hasInputText = info.hasInputText || (new Object() {
          boolean isOrHasChildren(Element elt) {
          if (isInputText(elt.getName()) || hasInputText(elt.getName())) {
          return true;
          }
          for (Iterator iter = elt.getChildren().iterator(); iter.hasNext(); ) {
          Element child = (Element) iter.getNext();
          return isOrHasChildren(child);
          }
          }
          }).isOrHasChildren(definition);

          }*/
        /* <class ...>
           <view>
           <text/>
           </view>
           </class>
        */
    }

    /**
       Walk the Schema DOM looking for an element which defines CLASSNAME.
       <p>
       This
       can actually occur in the schema as either
       <pre>
       &lt;element name="classname"&gt;
       or
       &lt;element&gt; &lt;choice&gt; ... ...  &lt;name&gt;classname&lt;/name&gt; &lt;/choice&gt;
       </pre>

       <p>
       Uses the classElementTable to speed up the lookup.
    */
    
    Element findSchemaClassDef (String classname, Element elt) {

        // Check the index
        Element classdef = (Element) classElementTable.get(classname);
        if (classdef != null) {
            return classdef;
        }

        if (elt.getName().equals("element")) {
            Namespace ns = elt.getNamespace();
            // Is this an element named "classname"?
            String name = elt.getAttributeValue("name");
            if (name != null && name.equals(classname)) {
                // cache this
                classElementTable.put(classname, elt);
                return elt;
            } else if (name == null) {
                // This is a structure of the form
                // <element><choice><name>xxxx</name>...?  Look for
                // immediate "choice" child, and look in its "name"
                // children
                for (Iterator iter = elt.getChildren("choice", ns).iterator(); iter.hasNext();) {
                    Element choice = (Element) iter.next();
                    for (Iterator iter2 = choice.getChildren("name", ns).iterator(); iter2.hasNext();) {
                        Element child = (Element) iter2.next();
                        if (child.getTextTrim().equals(classname)) {
                            classElementTable.put(classname, elt);
                            return elt;
                        }
                    }
                }
            }
        } 

        // class def not found here; descend the tree to search for it
        for (Iterator iter = elt.getChildren().iterator(); iter.hasNext(); ) {
            Element child = (Element) iter.next();
            Element result =  findSchemaClassDef(classname, child);
            if (result != null) {
                return result;
            }
        }

        return (Element) null;
    }

    public Type getTypeForName(String name) {
        if (name.equals("text") ||
            name.equals("html"))
            name = "string";
        if (sRNGtoLPSTypeMap.containsKey(name))
            return (Type) sRNGtoLPSTypeMap.get(name);
        return super.getTypeForName(name);
    }

    /** Maps definition name to list of DOM Elements in the parsed RNG schema DOM */
    private final Map referenceTable = new HashMap();
    /** List of all <ELEMENT> tags in the schema DOM. */
    private final List schemaElementTable = new ArrayList();

    /** Table which maps class names to their definitions in the
     * schema DOM, used by findSchemaClassDef() */
    private final Map classElementTable = new HashMap();


    /** Construct a table of <definition> elements from the RNG schema, to make
     * efficient lookup possible when following <ref> tags.
     */
    private void buildReferenceTable (Element elt) {
        // If it's a <define name="xxx"> add an entry in the referenceTable
        String eltname = elt.getName();
        if (eltname.equals("define")) {        
            String name = elt.getAttributeValue("name");
            List elts = (List) referenceTable.get(name);
            if (elts == null) {
                elts = new ArrayList();
                referenceTable.put(name, elts);
            } 
            elts.add(elt);
            //System.err.println("[adding ref "+name+" " + elt.getAttribute("name")+"]");
        } else if (eltname.equals("element")) {
            schemaElementTable.add(elt);
        }

        for (Iterator iter = elt.getChildren().iterator(); iter.hasNext(); ) {
            Element child = (Element) iter.next();
            buildReferenceTable(child);
        }
    }

    /** Adds a ClassModel entry into the class table for CLASSNAME. */
    private void makeNewStaticClass (String classname) {
        ClassModel info = new ClassModel(classname, this);
        if (sInputTextElements.contains(classname)) {
            info.isInputText = true;
            info.hasInputText = true;
        }
        if (mClassMap.get(classname) == null) {
            mClassMap.put(classname, info);
        } else {
            // <font> seems to come out twice when we parse the
            // schema, so this check doesn't really work.
            
            // throw new CompilationError("makeNewStaticClass: duplicate definition for " + classname);
        }

    }


    /**
       Walks the schema DOM searching for all <element> tags. For each named <element>, parse out the
       attribute declarations for it and add them to the in-core attribute-type table. 
       <p>
       Definitions of elements can actually occur in the schema as either
       <pre>
       &lt;element name="classname"&gt;
       or
       &lt;element&gt; &lt;choice&gt; ... ...  &lt;name&gt;classname&lt;/name&gt; &lt;/choice&gt;
       </pre>
       <p>

       In the second case above, where a set of element names is declared, we
       set the attribute types once for each element (class) name.

       <p>

       The reason we need this method is that when a user defines a new class
       which has one or more attributes, we need to learn if each attribute has
       been declared by a superclass. That info is available for user-defined
       classes, in the mClassMap table, but it's not present for 'primitive'
       system types that are implicitly built into the base schema RNG file. So
       here's where we parse that info from the base schema.

       <p>

       The algorithm is as follows: buildReferenceTable() is called once at
       schema load time to initialize a table of <definition> elements by name,
       for efficient following of <ref> tags.

       <p>

       Make a pass descending from root:
       <ul>
       <li>When you get to an <element>, set that to the current class name, and keep adding attributes
       as you find them, following references. When you hit a "element", change class name.
       <p>
       <li>Mark elements as having been visited to  avoid re-traversing them.
       </ul>
    */
    
    public void parseSchemaAttributes () {
        for (Iterator eltIter = schemaElementTable.iterator(); eltIter.hasNext();) {
            Element elt = (Element) eltIter.next();

            // We are starting on a known element tag; what class names does it define?
            List classnames = new ArrayList();
            Namespace ns = elt.getNamespace();
            String name = elt.getAttributeValue("name");
            // Is this tag of the form <element name="foo"> or
            // <element><choice><name>foo</name><name>bar</name>... ?
            if (name != null) {
                classnames.add(name);
                //System.err.println("parseSchemaAttributes("+name+")");
                makeNewStaticClass(name);
            } else if (name == null) {
                // This is a structure of the form
                // <element><choice><name>xxxx</name>...?  Look for
                // immediate "choice" child, and look in it's "name"
                // children
                for (Iterator iter = elt.getChildren("choice", ns).iterator(); iter.hasNext();) {
                    Element choice = (Element) iter.next();
                    for (Iterator iter2 = choice.getChildren("name", ns).iterator(); iter2.hasNext();) {
                        Element child = (Element) iter2.next();
                        name = child.getTextTrim();
                        // Add this name to the list of classnames that this element defines
                        classnames.add(name);
                        //System.err.println("parseSchemaAttributes("+name+")");
                        makeNewStaticClass(name);
                    }
                }
            }
            // OK we're defining some named element, lets follow the
            // tree down to the next element
            if (!classnames.isEmpty()) {
                //System.out.println("calling parseSchemaAttributes:: "+classnames);
                // Now descend the tree looking for <attribute> tags, and following <refs>
                for (Iterator iter = elt.getChildren().iterator(); iter.hasNext(); ) {
                    Element child = (Element) iter.next();
                    parseSchemaAttributes(child, classnames);
                }
            }
        }
    }

    /**
       This descends the schema DOM collecting <attribute> declaration
       info, but stops the descent wherever another <element> tag is
       found.

       <ref> tags are followed via the referenceTable map.
    */

    public void parseSchemaAttributes (Element elt, List classnames) {
        // Stop if we get to an <element>
        if (elt.getName().equals("element")) {
            return;
        } else if (elt.getName().equals("attribute")) {
            String attrName = elt.getAttributeValue("name");
            if (attrName == null) {
                //throw new RuntimeException("parsing choice attribute names in the schema is unimplemented");
                // There is actually one unnamed attribute, the "anyName" attribute for datacontent.
            } else {
                //System.err.print("parsing attribute "+classnames+"."+attrName);

                String rngTypeName;

                /*
                  We will see one of these three types of attribute elements:

                  <attribute name="width" a:defaultValue="800">
                    <ref name="size"/>
                  </attribute>

                  <attribute name="title" a:defaultValue="Laszlo Presentation Server">
                    <a:documentation>The string that is used in the browser window.</a:documentation>
                    <data type="string" datatypeLibrary=""/>
                  </attribute>

                  <attribute name="fontstyle" a:defaultValue="">
                    <a:documentation>The default font style for views in this application.</a:documentation>
                    <choice>
                      <value>bold</value>
                      <value>italic</value>
                      <value>bold italic</value>
                      <value>plain</value>
                      <value/>
                    </choice>
                  </attribute>

                  // or this degenerate case from menuitem
                  <attribute name="type">
                     <value>separator</value>
                  </attribute>
                */
                Namespace ns = elt.getNamespace();

                Element ref = elt.getChild("ref", ns);
                if (ref != null) {
                    rngTypeName = ref.getAttributeValue("name");
                } else {
                    Element data = elt.getChild("data", ns);
                    if (data != null) {
                        rngTypeName = data.getAttributeValue("type");
                    } else {
                        // If it's a <choice> or <value>, then it's a token
                        rngTypeName = "token";
                    }
                }

                // Set the attribute type 
                Type attrType = (Type) sRNGtoLPSTypeMap.get(rngTypeName);

                if (attrType == null) {
                    throw new RuntimeException("error parsing RNG schema: unknown attribute type name "+ rngTypeName +
                                               "  for attribute "+attrName);
                }

                AttributeSpec attrspec = new AttributeSpec(attrName, attrType, null, null);
                //System.err.println(" ==> "+attrType);

                // Define this attribute on all classnames it applies to
                Iterator citer = classnames.iterator();
                while (citer.hasNext()) {
                    String cname = (String) citer.next();
                    //System.err.println("adding attribute type from schema: elt(class) "+cname+": "+attrName); 

                    // TODO: [2003-02-04 hqm] There is a special case
                    // for <splash> elements; the schema contains a
                    // special <view> declaration which differs from
                    // the normal one, in that "x" and "y" attributes
                    // are numeric constants rather than expressions.

                    // To deal with this, we check if this element has
                    // a parent of <splash> and if so we just discard
                    // the attribute info. SplashCompiler will have a
                    // special case to quote values for x and y
                    // attributes.

                    boolean ignore = false;
                    // search up the parents for a "splash" element
                    if (cname.equals("view")) {
                        Element p = elt;
                        while (p != null) {
                            p = p.getParent();
                            if (p == null) {
                                break;
                            }
                            if ((p.getName().equals("element")) && ("splash".equals(p.getAttributeValue("name")))) {
                                ignore = true;
                                break;
                            }
                        }
                    }

                    // Look up the ClassModel in the class info table
                    ClassModel classModel = getClassModel(cname);
                    if (classModel == null) {
                        throw new RuntimeException("parseSchemaAttributes: undefined class: " + cname);
                    }
                    if (ignore) {
                        //System.err.println("ignoring splash child attribute "+attrName+" "+attrType);
                    } else {
                        classModel.attributeSpecs.put(attrName, attrspec);
                    }
                }
            }
        } else if (elt.getName().equals("ref")) {
            // follow references
            String refName = elt.getAttributeValue("name");
            // The list of all elements that define this reference. A
            // reference can be split over several different elements
            // in the schema. See "viewContentElements" for example.
            List refElements = (List) referenceTable.get(refName);
            if (refElements == null || refElements.isEmpty()) {
                throw new RuntimeException("error parsing schema: could not find definition for reference "+refName+" on elt" + elt);
            }
            Iterator riter = refElements.iterator();
            while (riter.hasNext()) {
                Element ref = (Element) riter.next();
                if (ref == null) {
                    throw new RuntimeException("error parsing schema: could not find definition for reference "+refName+" on elt" + elt);
                }
                parseSchemaAttributes(ref, classnames);
            }
        }

        // Descend the tree
        for (Iterator iter = elt.getChildren().iterator(); iter.hasNext(); ) {
            Element child = (Element) iter.next();
            parseSchemaAttributes(child, classnames);
        }
    }

    static Type getAttributeType(String attrName) {
        return (Type) sAttributeTypes.get(attrName);
    }
    
    /**
     * Returns a value representing the type of an attribute within an
     * XML element. Unknown attributes have Expression type.
     *
     * @param e an Element
     * @param name an attribute name
     * @return a value represting the type of the attribute's
     */
    public Type getAttributeType(Element e, String attrName) {
        return getAttributeType(e.getName(), attrName);
    }

    /**
     * Returns a value representing the type of an attribute within an
     * XML element. Unknown attributes have Expression type.
     *
     * @param e an Element name
     * @param name an attribute name
     * @param throwException if no explicit type is found, throw UnknownAttributeException
     * @return a value represting the type of the attribute's
     */
    public Type getAttributeType(String elt, String attrName)
        throws UnknownAttributeException
    {
        String elementName = elt.intern();
        // +++ This special-case stuff will go away when the RELAX schema
        // is automatically translated to ViewSchema initialization
        // code.

        if (elementName.equals("canvas")) {
            // override NUMBER_EXPRESSION_TYPE, on view
            if (attrName.equals("width") || attrName.equals("height")) {
                return NUMBER_TYPE;
            }
        }

        Type type = null;

        // Look up attribute in type map for this element
        ClassModel classModel = getClassModel(elementName);
        
        if (classModel != null) {
            try {
                type = classModel.getAttributeTypeOrException(attrName);
            } catch (UnknownAttributeException e) {
                e.setName(attrName);
                e.setElementName(elt);
                throw e;
            }
        } else {
            type = getAttributeType(attrName);
            if (type == null) {
                throw new UnknownAttributeException(elt, attrName);
                //type = EXPRESSION_TYPE;
            }
        }
        return type;
    }

    boolean isMouseEventAttribute(String name) {
        return sMouseEventAttributes.contains(name);
    }

    ClassModel getClassModel (String elementName) {
        return (ClassModel) mClassMap.get(elementName);
    }
    
    public void loadSchema() throws JDOMException {
        String schemaPath = SCHEMA_PATH;
        // Load the schema if it hasn't been.
        // Reload it if it's been touched, to make it easier for developers.
        while (sCachedSchemaDOM == null ||
               new File(schemaPath).lastModified() != sCachedSchemaLastModified) {
            // using 'while' and reading the timestamp *first* avoids a
            // race condition --- although since this doesn't happen in
            // production code, this isn't critical
            sCachedSchemaLastModified = new File(schemaPath).lastModified();
            // Without this, parsing fails when LPS is installed
            // in a directory with a space in the name.
            String schemaURI = "file:///" + schemaPath;
            sCachedSchemaDOM = new SAXBuilder().build(schemaURI);
        }
        schemaDOM = (Document) sCachedSchemaDOM.clone();
        buildReferenceTable(schemaDOM.getRootElement());
        // Parse out the attribute types of built-in defs
        parseSchemaAttributes();
    }
    
    public Document getSchemaDOM() throws JDOMException {
        if (schemaDOM == null) {
            loadSchema();
        }
        return schemaDOM;
    }

    /** @return true if this element is an input text field */
    boolean isInputTextElement(Element e) {
        String classname = e.getName();
        ClassModel info = getClassModel(classname);
        if (info != null) {
            return info.isInputText;
        }
        return sInputTextElements.contains(classname);
    }

    boolean supportsTextAttribute(Element e) {
        String classname = e.getName();
        ClassModel info = getClassModel(classname);
        if (info != null) {
            return info.supportsTextAttribute;
        } else {
            return false;
        }
    }


    /** @return true if this element content is interpreted as text */
    boolean hasTextContent(Element e) {
        // input text elements can have text
        return isInputTextElement(e) || supportsTextAttribute(e);
    }

    /** @return true if this element's content is HTML. */
    boolean hasHTMLContent(Element e) {
        String name = e.getName().intern();
        // TBD: return sHTMLContentElements.contains(name). Currently
        // uses a blacklist instead of a whitelist because the parser
        // doesn't tell the schema about user-defined classes.
        // XXX Since any view can have a text body, this implementation
        // is actually correct.  That is, blacklist rather than whitelist
        // is the way to go here.
        return name != "class" && name != "method"
            && name != "property" && name != "script"
            && name != "attribute"
            && !isHTMLElement(e) && !isInputTextElement(e);
    }

    /** @return true if this element is an HTML element, that should
     * be included in the text content of an element that tests true
     * with hasHTMLContent. */
    static boolean isHTMLElement(Element e) {
        String name = e.getName();
        return name.equals("a")
            || name.equals("b")
            || name.equals("br")
            || name.equals("font")
            || name.equals("i")
            || name.equals("p")
            || name.equals("pre")
            || name.equals("u");
    }


    /* Constants for parsing CSS colors. */
    static final PatternMatcher sMatcher = new Perl5Matcher();
    static final Pattern sRGBPattern;
    static final Pattern sHex3Pattern;
    static final Pattern sHex6Pattern;
    static final HashMap sColorValues = new HashMap();
    static {
        try {
            Perl5Compiler compiler = new Perl5Compiler();
            String s = "\\s*(-?\\d+(?:(?:.\\d*)%)?)\\s*"; // component
            String hexDigit = "[0-9a-fA-F]";
            String hexByte = hexDigit + hexDigit;
            sRGBPattern = compiler.compile(
                "\\s*rgb\\("+s+","+s+","+s+"\\)\\s*");
            sHex3Pattern = compiler.compile(
                "\\s*#\\s*(" + hexDigit + hexDigit + hexDigit + ")\\s*");
            sHex6Pattern = compiler.compile(
                "\\s*#\\s*(" + hexByte + hexByte + hexByte + ")\\s*");
        } catch (MalformedPatternException e) {
            throw new ChainedException(e);
        }

        String[] colorNameValues = {
            "black", "000000", "green", "008000",
            "silver", "C0C0C0", "lime", "00FF00",
            "gray", "808080", "olive", "808000",
            "white", "FFFFFF", "yellow", "FFFF00",
            "maroon", "800000", "navy", "000080",
            "red", "FF0000", "blue", "0000FF",
            "purple", "800080", "teal", "008080",
            "fuchsia", "FF00FF", "aqua", "00FFFF"};
        for (int i = 0; i < colorNameValues.length; ) {
            String name = colorNameValues[i++];
            String value = colorNameValues[i++];
            sColorValues.put(name, new Integer(Integer.parseInt(value, 16)));
        }
    }

    static class ColorFormatException extends RuntimeException {
        ColorFormatException(String message) {
            super(message);
        }
    }
     
    /** Parse according to http://www.w3.org/TR/2001/WD-css3-color-20010305,
     * but also allow 0xXXXXXX */
    public static int parseColor(String str) {
        {
            Object value = sColorValues.get(str);
            if (value != null) {
                return ((Integer) value).intValue();
            }
        }
        if (str.startsWith("0x")) {
            try {
                return Integer.parseInt(str.substring(2), 16);
            } catch (java.lang.NumberFormatException e) {
                // fall through
            }
        }
        if (sMatcher.matches(str, sHex3Pattern)) {
            int r1g1b1 = Integer.parseInt(sMatcher.getMatch().group(1), 16);
            int r = (r1g1b1 >> 8) * 17;
            int g = ((r1g1b1 >> 4) & 0xf) * 17;
            int b = (r1g1b1 & 0xf) * 17;
            return (r << 16) + (g << 8) + b;
        }
        if (sMatcher.matches(str, sHex6Pattern)) {
            return Integer.parseInt(sMatcher.getMatch().group(1), 16);
        }
        if (sMatcher.matches(str, sRGBPattern)) {
            int v = 0;
            for (int i = 0; i < 3; i++) {
                String s = sMatcher.getMatch().group(i+1);
                int c;
                if (s.charAt(s.length()-1) == '%') {
                    s = s.substring(0, s.length()-1);
                    float f = Float.parseFloat(s);
                    c = (int) f * 255 / 100;
                } else {
                    c = Integer.parseInt(s);
                }
                if (c < 0) c = 0;
                if (c > 255) c = 255;
                v = (v << 8) | c;
            }
            return v;
        }
        throw new ColorFormatException(str);
    }
}
