/* ***************************************************************************
 * NodeModel.java
 * ***************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

import java.io.*;
import java.text.ChoiceFormat;
import java.util.*;

import org.openlaszlo.compiler.ViewSchema.ColorFormatException;
import org.openlaszlo.css.CSSParser;
import org.openlaszlo.sc.Function;
import org.openlaszlo.sc.ScriptCompiler;
import org.openlaszlo.server.*;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.utils.ListFormat;
import org.openlaszlo.utils.ComparisonMap;
import org.openlaszlo.xml.internal.MissingAttributeException;
import org.openlaszlo.xml.internal.Schema;
import org.openlaszlo.xml.internal.XMLUtils;
import org.apache.oro.text.regex.*;
import org.apache.commons.collections.CollectionUtils;
import org.jdom.Attribute;
import org.jdom.Element;
import org.jdom.Namespace;

/** Models a runtime LzNode. */
public class NodeModel implements Cloneable {
    public static final String FONTSTYLE_ATTRIBUTE = "fontstyle";
    public static final String WHEN_IMMEDIATELY = "immediately";
    public static final String WHEN_ONCE = "once";
    public static final String WHEN_ALWAYS = "always";
    public static final String WHEN_PATH = "path";

    protected final ViewSchema schema;
    protected final Element element;
    protected String className;
    protected String id = null;
    protected ComparisonMap attrs = new ComparisonMap();
    protected List children = new Vector();
    /** A set {eventName: String -> True) of names of event handlers
     * declared with <method event="xxx"/>. */
    protected ComparisonMap delegates = new ComparisonMap();
    protected ComparisonMap events = new ComparisonMap();
    protected ComparisonMap references = new ComparisonMap();
    protected ComparisonMap paths = new ComparisonMap();
    protected ComparisonMap setters = new ComparisonMap();
    /** [eventName: String, methodName: String, Function] */
    protected List delegateList = new Vector();
    protected ClassModel parentClassModel;
    protected String initstage = null;
    protected int totalSubnodes = 1;
    protected final CompilationEnvironment env;

    public Object clone() {
        NodeModel copy;
        try {
            copy = (NodeModel) super.clone();
        } catch (CloneNotSupportedException e) {
            throw new RuntimeException(e);
        }
        copy.attrs = new ComparisonMap(copy.attrs);
        copy.delegates = new ComparisonMap(copy.delegates);
        copy.events = new ComparisonMap(copy.events);
        copy.references = new ComparisonMap(copy.references);
        copy.paths = new ComparisonMap(copy.paths);
        copy.setters = new ComparisonMap(copy.setters);
        copy.delegateList = new Vector(copy.delegateList);
        copy.children = new Vector();
        for (Iterator iter = children.iterator(); iter.hasNext(); ) {
            copy.children.add(((NodeModel) iter.next()).clone());
        }
        return copy;
    }

    NodeModel(Element element, ViewSchema schema, CompilationEnvironment env) {
        this.element = element;
        this.schema = schema;
        this.env = env;

        this.className = element.getName();
        // Cache ClassModel for parent
        this.parentClassModel = this.getParentClassModel();
        this.initstage =
            this.element.getAttributeValue("initstage");
        if (this.initstage != null) {
            this.initstage = this.initstage.intern();
        }

        // Get initial node count from superclass
        // TODO: [2003-05-04] Extend this mechanism to cache/model
        // all relevant superclass info
        // TODO: [2003-05-04 ptw] How can we get this info for
        // instances of built-in classes?
        if (this.parentClassModel != null) {
            ElementWithLocationInfo parentElement =
                (ElementWithLocationInfo) this.parentClassModel.definition;
            // TODO: [2003-05-04 ptw] As above, but a class
            // that extends a built-in class
            if (parentElement != null) {
                // TODO: [2003-05-04 ptw] An instantiation of
                // a class that has not been modelled yet --
                // do enough modelling to get what is needed.
                if (parentElement.model != null) {
                    this.totalSubnodes =
                        parentElement.model.classSubnodes();
                    if (this.initstage == null) {
                        this.initstage =
                            parentElement.model.initstage;
                    }
                }
            }
        }
    }

    private static final String DEPRECATED_METHODS_PROPERTY_FILE = (
        LPS.getMiscDirectory() + File.separator + "lzx-deprecated-methods.properties"
        );
    private static final Properties sDeprecatedMethods = new Properties();

    static {
        try {
            InputStream is = new FileInputStream(DEPRECATED_METHODS_PROPERTY_FILE);
            try {
                sDeprecatedMethods.load(is);
            } finally {
                is.close();
            }
        } catch (java.io.IOException e) {
            throw new ChainedException(e);
        }
    }

    static class CompiledAttribute {
        static final int ATTRIBUTE = 0;
        static final int EVENT = 1;
        static final int REFERENCE = 2;
        static final int PATH = 3;

        final int type;
        final Object value;

        CompiledAttribute(int type, Object value) {
            this.type = type;
            this.value = value;
        }
        CompiledAttribute(Object value) {
            this(ATTRIBUTE, value);
        }
    }

    public String toString() {
        StringBuffer buffer = new StringBuffer();
        buffer.append("{NodeModel class=" + className);
        if (!attrs.isEmpty())
            buffer.append(" attrs=" + attrs.keySet());
        if (!delegates.isEmpty())
            buffer.append(" delegates=" + delegates.keySet());
        if (!events.isEmpty())
            buffer.append(" events=" + events.keySet());
        if (!references.isEmpty())
            buffer.append(" references=" + references.keySet());
        if (!paths.isEmpty())
            buffer.append(" paths=" + paths.keySet());
        if (!setters.isEmpty())
            buffer.append(" setters=" + setters.keySet());
        if (!delegateList.isEmpty())
            buffer.append(" delegateList=" + delegateList);
        if (!children.isEmpty())
            buffer.append(" children=" + children);
        buffer.append("}");
        return buffer.toString();
    }

    List getChildren() {
        return children;
    }

    public static boolean isPropertyElement(Element elt) {
        String name = elt.getName();
        return name.equals("attribute") || name.equals("method");
    }

    /** Returns a name that is used to report this element in warning
     * messages. */
    String getMessageName() {
        return "element " + element.getName();
    }

    /**
     * Returns a script that creates a runtime representation of a
     * model.  The format of this representation is specified <a
     * href="../../../../doc/compiler/views.html">here</a>.
     *
     * @param elt an element
     * @param schema a schema, used to encode attribute values
     * @param env the CompilationEnvironment
     * @return see doc
     */
    public String asJavascript() {
        try {
            java.io.Writer writer = new java.io.StringWriter();
            ScriptCompiler.writeObject(this.asMap(), writer);
            return writer.toString();
        } catch (java.io.IOException e) {
            throw new ChainedException(e);
        }
    }

    /** Returns true iff clickable should default to true. */
    private static boolean computeDefaultClickable(ViewSchema schema,
                                                   Map attrs,
                                                   Map events,
                                                   Map delegates) {
        if ("true".equals(attrs.get("cursor"))) {
            return true;
        }
        for (Iterator iter = events.keySet().iterator(); iter.hasNext();) {
            String eventName = (String) iter.next();
            if (schema.isMouseEventAttribute(eventName)) {
                return true;
            }
        }
        for (Iterator iter = delegates.keySet().iterator(); iter.hasNext();) {
            String eventName = (String) iter.next();
            if (schema.isMouseEventAttribute(eventName)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Returns a NodeModel that represents an Element, including the
     * element's children
     *
     * @param elt an element
     * @param schema a schema, used to encode attribute values
     * @param env the CompilationEnvironment
     */
    public static NodeModel elementAsModel(Element elt, ViewSchema schema,
                                           CompilationEnvironment env) {
        return elementAsModelInternal(elt, schema, true, env);
    }

    /**
     * Returns a NodeModel that represents an Element, excluding the
     * element's children
     *
     * @param elt an element
     * @param schema a schema, used to encode attribute values
     * @param env the CompilationEnvironment
     */
    public static NodeModel elementOnlyAsModel(Element elt, ViewSchema schema,
                                               CompilationEnvironment env) {
        return elementAsModelInternal(elt, schema, false, env);
    }

    /** Returns a NodeModel that represents an Element
     *
     * @param elt an element
     * @param schema a schema, used to encode attribute values
     * @param includeChildren whether or not to include children
     * @param env the CompilationEnvironment
     */
    private static NodeModel elementAsModelInternal(
        Element elt, ViewSchema schema, boolean includeChildren,
        CompilationEnvironment env)
    {
            NodeModel model = new NodeModel(elt, schema, env);
            ComparisonMap attrs = model.attrs;
            Map events = model.events;
            Map delegates = model.delegates;
            model.addAttributes(env);
            if (includeChildren) {
                model.addChildren(env);
                model.addText();
                if (!attrs.containsKey("clickable")
                    && computeDefaultClickable(schema, attrs, events, delegates)) {
                    attrs.put("clickable", "true");
                }
            }
            // Record the model in the element for classes
            ((ElementWithLocationInfo) elt).model = model;
            return model;
        }

    // Calculate how many nodes this object will put on the
    // instantiation queue.
    int totalSubnodes() {
        // A class does not instantiate its subnodes.
        // States override LzNode.thaw to delay subnodes.
        // FIXME [2004-06-3 ows]: This won't work for subclasses
        // of state.
        if (ClassCompiler.isElement(element) ||
            className.equals("state")) {
            return 1;
        }
        // initstage late, defer delay subnodes
        if (this.initstage != null &&
            (this.initstage == "late" ||
             this.initstage == "defer")) {
            return 0;
        }
        return this.totalSubnodes;
    }

    // How many nodes will be inherited from this class
    int classSubnodes() {
        if (ClassCompiler.isElement(element)) {
            return this.totalSubnodes;
        }
        return 0;
    }

    ClassModel getClassModel() {
        return schema.getClassModel(this.className);
    }

    /** Gets the ClassModel for this element's parent class.  If this
     * element is a <class> definition, the superclass; otherwise the
     * class of the tag of this element. */
    ClassModel getParentClassModel() {
        String parentName = this.className;
        return
            parentName.equals("class")?
            schema.getClassModel(element.getAttributeValue("extends")):
            schema.getClassModel(parentName);
    }

    void setClassName(String name) {
        this.className = name;
        this.parentClassModel = getParentClassModel();
    }

    // Should only be called on a <class> definition element.
    ViewSchema.Type getAttributeTypeInfoFromSuperclass(
        Element classDefElement, String attrname)
        throws UnknownAttributeException
    {
        String superclassname = classDefElement.getAttributeValue("extends", ClassCompiler.DEFAULT_SUPERCLASS_NAME);
        ClassModel superclassModel = schema.getClassModel(superclassname);

        if (superclassModel == null) {
            throw new CompilationError("Could not find superclass info for class " + superclassname, classDefElement);
        }

        // Check if this attribute is defined on the parent class, if
        // so, return that type
        AttributeSpec attr = superclassModel.getAttribute(attrname);
        if (attr != null) {
            return attr.type;
        }
        // Otherwise, check if it's defined on the "class" element
        // (e.g., 'name' or 'extends')
        superclassModel = schema.getClassModel("class");
        return superclassModel.getAttributeTypeOrException(attrname);
    }

    // Get an attribute value, defaulting to the
    // inherited value, or ultimately the supplied default
    String getAttributeValueDefault(String attribute,
                                    String name,
                                    String defaultValue) {
        // Look for an inherited value
        if (this.parentClassModel != null) {
            AttributeSpec attrSpec =
                this.parentClassModel.getAttribute(attribute);
            if (attrSpec != null) {
                Element source = attrSpec.source;
                if (source != null) {
                    return XMLUtils.getAttributeValue(source, name, defaultValue);
                }
            }
        }

        return defaultValue;
    }

    /** Is this element a direct child of the canvas? */
    // FIXME [2004-06-03 ows]: Use CompilerUtils.isTopLevel instead.
    // This implementation misses children of <library> and <switch>.
    // Since it's only used for compiler warnings about duplicate
    // names this doesn't break program compilation.
    boolean topLevelDeclaration() {
        Element parent = element.getParent();
        if (parent == null) {
            return false;
        }
        return ("canvas".equals(parent.getName()));
    }

    void addAttributes(CompilationEnvironment env) {
        // Encode the attributes
        for (Iterator iter = element.getAttributes().iterator(); iter.hasNext(); ) {
            Attribute attr = (Attribute) iter.next();
            Namespace ns = attr.getNamespace();
            String name = attr.getName();
            String value = element.getAttributeValue(name, ns);

            if (name.equals(FONTSTYLE_ATTRIBUTE)) {
                // "bold italic", "italic bold" -> "bolditalic"
                value = FontInfo.normalizeStyleString(value, false);
            }

            if (name.equals("defaultPlacement")) {
                if (value != null && value.matches("\\s*['\"]\\S*['\"]\\s*")) {
                    String oldValue = value;
                    // strip off start and ending quotes;
                    value = value.trim();
                    value = value.substring(1, value.length()-1);
                    env.warn(
                        "Replacing defaultPlacement=\"" + oldValue +
                        "\" by \"" + value + "\".  For future compatability" +
                        ", you should make this change to your source code.",
                        element);
                }
            }

            // Catch duplicated id/name attributes which may shadow
            // each other or overwrite each other.  An id/name will be
            // global there is "id='foo'" or if "name='foo'" at the
            // top level (immediate child of the canvas).
            //
            // NB: since this finds class names via a lookup from
            // elements in the schema, it will give some false
            // positives on class-name collisions, such as tag names
            // like "audio" which do not actually correspond to a LFC
            // class at runtime.
            if ((name.equals("id")) ||
                (name.equals("name") &&
                 topLevelDeclaration() && !className.equals("class"))) {
                ClassModel superclassModel = schema.getClassModel(value);
                if (superclassModel != null && !superclassModel.isBuiltin()) {
                    env.warn(
                        "You have given the "+getMessageName()+
                        " an attribute "+name+"=\""+value+"\", "+
                        "which may overwrite the class \""+value+"\".",
                        element);
                } else {
                    ElementWithLocationInfo dup =
                        (ElementWithLocationInfo) env.getId(value);
                    // we don't want to give a warning in the case
                    // where the id and name are on the same element,
                    // i.e., <view id="foo" name="foo"/>
                    if (dup != null && dup != element) {
                        String locstring =
                            CompilerUtils.sourceLocationPrettyString(dup);
                        env.warn(
                            "Duplicate id attribute \""+value+"\" at "+locstring,
                            element);
                    } else {
                        // TODO: [07-18-03 hqm] We will canonicalize
                        // all id's to lowercase, because actionscript
                        // is not case sensitive.  but in the future,
                        // we should preserve case.
                        env.addId(value, element);
                    }
                }
            }

            // Special case, if we are compiling a "class" tag,
            // then get the type of attributes from the
            // superclass.
            Schema.Type type;
            try {
                if (className.equals("class")) {
                    type = getAttributeTypeInfoFromSuperclass(element, name);
                }  else {
                    type = schema.getAttributeType(element, name);
                }

            } catch (UnknownAttributeException e) {
                String solution;
                AttributeSpec alt = schema.findSimilarAttribute(className, name);
                if (alt != null) {
                    String classmessage = "";
                    if (alt.source != null) {
                        classmessage = " on class "+alt.source.getName()+"\"";
                    } else {
                        classmessage = " on class "+getMessageName();
                    }
                    solution = "found an unknown attribute named \""+name
                        +"\" on "+getMessageName()+", however there is an attribute named \""
                        +alt.name+"\""+ classmessage+ ", did you mean to use that?";
                } else {
                    solution = "found an unknown attribute named \""+name+"\" on "+getMessageName()+
                        ", check the spelling of this attribute name";
                }
                env.warn(solution, element);
                type = ViewSchema.EXPRESSION_TYPE;
            }

            if (type == schema.ID_TYPE) {
                this.id = value;
            } else {
                String when = this.getAttributeValueDefault(
                    name, "when", WHEN_IMMEDIATELY);
                try {
                    CompiledAttribute cattr = compileAttribute(
                        element, name, value, type, when);
                    addAttribute(cattr, name, attrs, events,
                                 references, paths);
                    // Check if we are aliasing another 'name'
                    // attribute of a sibling
                    if (name.equals("name")) {
                        Element parent = element.getParent();
                        if (parent != null) {
                            for (Iterator iter2 = parent.getChildren().iterator(); iter2.hasNext();
                                 ) {
                                Element e = (Element) iter2.next();
                                if (!e.getName().equals("resource") && !e.getName().equals("font")
                                    && e != element && value.equals(e.getAttributeValue("name"))) {
                                    String dup_location =
                                        CompilerUtils.sourceLocationPrettyString(e);
                                    env.warn(
                                        getMessageName() + " has the same name=\""+
                                        value+"\" attribute as a sibling element at "+dup_location,
                                        element);
                                }
                            }
                        }
                    }
                } catch (CompilationError e) {
                    env.warn(e);
                }
            }
        }
    }

    void addAttribute(CompiledAttribute cattr, String name,
                      ComparisonMap attrs, ComparisonMap events,
                      ComparisonMap references, ComparisonMap paths) {
        if (cattr.type == cattr.ATTRIBUTE) {
            if (attrs.containsKey(name)) {
                env.warn(
                    "an attribute or method named '"+name+
                    "' already is defined on "+getMessageName(),
                    element);
            }
            attrs.put(name, cattr.value);
        } else if (cattr.type == cattr.EVENT) {
            if (events.containsKey(name)) {
                env.warn(
                    "redefining event '"+name+
                    "' which has already been defined on "+getMessageName(),
                    element);
            }
            events.put(name, cattr.value);
        } else if (cattr.type == cattr.REFERENCE) {
            if (references.containsKey(name)) {
                env.warn(
                    "redefining reference '"+name+
                    "' which has already been defined on "+getMessageName(),
                    element);
            }
            references.put(name, cattr.value);
        } else if (cattr.type == cattr.PATH) {
            references.put(name, cattr.value);
        }
    }

    void addChildren(CompilationEnvironment env) {
        // Encode the children
        for (Iterator iter = element.getChildren().iterator(); iter.hasNext(); ) {
            ElementWithLocationInfo child = (ElementWithLocationInfo) iter.next();
            try {
                if (isPropertyElement(child)) {
                    addPropertyElement(child);
                } else if (schema.isHTMLElement(child)) {
                    ; // ignore; the text compiler wiil handle this
                } else {
                    NodeModel childModel = elementAsModel(child, schema, env);
                    children.add(childModel);
                    totalSubnodes += childModel.totalSubnodes();
                }
            } catch (CompilationError e) {
                env.warn(e);
            }
        }
    }

    void addPropertyElement(Element element) {
        String tagName = element.getName();
        if (tagName.equals("method")) {
            addMethodElement(element);
        } else if (tagName.equals("attribute")) {
            addAttributeElement(element);
        }
    }

    void addMethodElement(Element element) {
        String srcloc =
            CompilerUtils.sourceLocationDirective(element, true);
        String name = element.getAttributeValue("name");
        String event = element.getAttributeValue("event");
        String args = CompilerUtils.attributeLocationDirective(element, "args") +
            XMLUtils.getAttributeValue(element, "args", "");
        if ((name == null || "".equals(name)) && (event == null || "".equals(event))) {
            env.warn("method needs a non-null name or event attribute");
            return;
        }
        if (name != null && sDeprecatedMethods.containsKey(name)) {
            String oldName = name;
            String newName = (String) sDeprecatedMethods.get(name);
            name = newName;
            env.warn(
                oldName + " is deprecated.  " +
                "This method will be compiled as <method name='" + newName + "' instead.  " +
                "Please update your sources.",
                element);
        }

        String parent_name =
            element.getParent().getAttributeValue("id");
        String name_loc =
            (name == null ?
             CompilerUtils.attributeLocationDirective(element, "event") :
             CompilerUtils.attributeLocationDirective(element, "name"));
        if (parent_name == null) {
            parent_name =
                (name == null ?
                 CompilerUtils.attributeUniqueName(element, "event") :
                 CompilerUtils.attributeUniqueName(element, "name"));
        }
        if (event != null) {
            if (name == null) {
                // Note, this could be optimized again to try to
                // reuse names where they don't conflict, but if
                // we create a new symbol generator for each view,
                // and restart it at $m1, we have this screw case
                // of an unnamed method in a class getting the
                // same gensym'ed name as an unnamed method in an
                // instance.
                // Could use $cn with a global generator for classes,
                // and a restarted $m1 generator for each instance.
                name = env.methodNameGenerator.next();
            }
            String reference = element.getAttributeValue("reference");
            Object referencefn = "null";
            if (reference != null) {
                String ref_loc =
                    CompilerUtils.attributeLocationDirective(element, "reference");
                referencefn = new Function(
                    ref_loc +
                    parent_name + "_" + name + "_reference",
                    args,
                    "\n#pragma 'withThis'\n" +
                    "return (" +
                    "#beginAttribute\n" + ref_loc +
                    reference + "\n#endAttribute\n)");
            }
            // delegates is only used to determine whether to
            // default clickable to true.  Clickable should only
            // default to true if the event handler is attached to
            // this view.
            if (reference == null)
                delegates.put(event, Boolean.TRUE);
            delegateList.add(ScriptCompiler.quote(event));
            delegateList.add(ScriptCompiler.quote(name));
            delegateList.add(referencefn);
        }
        String body = element.getText();

        String childcontentloc =
            CompilerUtils.sourceLocationDirective(element, true);
        Function fndef = new
            // Use "mangled" name, so it will be unique
            Function(name_loc +
                     parent_name + "_" + name,
                     //"#beginAttribute\n" +
                     CompilerUtils.attributeLocationDirective(element, "args") + args,
                     "\n#beginContent\n" +
                     "\n#pragma 'methodName=" + name + "'\n" +
                     "\n#pragma 'withThis'\n" +
                     childcontentloc +
                     body + "\n#endContent");

        if (attrs.containsKey(name)) {
            env.warn(
                "an attribute or method named '"+name+
                "' already is defined on "+getMessageName(),
                element);
        }

        attrs.put(name, fndef);
    }

    CompiledAttribute compileAttribute(
        Element source, String name,
        String value, Schema.Type type,
        String when)
        {
            String srcloc = CompilerUtils.sourceLocationDirective(source, true);
            String parent_name = source.getAttributeValue("id");
            if (parent_name == null) {
                parent_name =  CompilerUtils.attributeUniqueName(source, name);
            }
            // Some values are not canonicalized to String
            Object canonicalValue = null;
            boolean warnOnDeprecatedConstraints = true;

            if (value == null) {
                throw new RuntimeException("value is null in " + source);
            }

            if (value.startsWith("$(")) {
                env.warn(
                    "The syntax '$(...)' is not valid, "
                    + "you probably meant to use curly-braces instead '${...}'",
                    source);
            } else if (value.startsWith("$") && value.endsWith("}")) {
                // Canonicalize values
                int brace = value.indexOf('{');
                // Expressions override any when value, default is
                // constraint
                if (brace >= 1) {
                    when = value.substring(1, brace);
                    value = value.substring(brace + 1, value.length() - 1);
                    if (when.equals(""))
                        when = WHEN_ALWAYS;
                }
            } else if (type == ViewSchema.COLOR_TYPE) {
                if (when.equals(WHEN_IMMEDIATELY)) {
                    try {
                        value = "0x" +
                            Integer.toHexString(ViewSchema.parseColor(value));
                    } catch (ColorFormatException e) {
                        // Or just set when to WHEN_ONCE and fall
                        // through to TODO?
                        throw new CompilationError(source, name, e);
                    }
                }
                // TODO: [2003-05-02 ptw] Wrap non-constant colors in
                // runtime parser
            } else if (type == ViewSchema.CSS_TYPE) {
                if (when.equals(WHEN_IMMEDIATELY)) {
                    try {
                        Map cssProperties = new CSSParser
                            (new AttributeStream(source, name, value)).Parse();
                        for (Iterator i2 = cssProperties.entrySet().iterator(); i2.hasNext(); ) {
                            Map.Entry entry = (Map.Entry) i2.next();
                            Object mv = entry.getValue();
                            if (mv instanceof String) {
                                entry.setValue(ScriptCompiler.quote((String) mv));
                            }
                        }
                        canonicalValue = cssProperties;
                    } catch (org.openlaszlo.css.ParseException e) {
                        // Or just set when to WHEN_ONCE and fall
                        // through to TODO?
                        throw new CompilationError(e);
                    } catch (org.openlaszlo.css.TokenMgrError e) {
                        // Or just set when to WHEN_ONCE and fall
                        // through to TODO?
                        throw new CompilationError(e);
                    }
                }
                // TODO: [2003-05-02 ptw] Wrap non-constant styles in
                // runtime parser
            } else if (type == ViewSchema.STRING_TYPE
                       || type == ViewSchema.TOKEN_TYPE
                       ) {
                // Immediate string attributes are auto-quoted
                if (when.equals(WHEN_IMMEDIATELY)) {
                    value = ScriptCompiler.quote(value);
                }
            } else if (type == ViewSchema.EXPRESSION_TYPE) {
                // No change currently, possibly analyze expressions
                // and default non-constant to when="once" in the
                // future
            } else if (type == ViewSchema.INHERITABLE_BOOLEAN_TYPE) {
                // change "inherit" to null and pass true/false through as expression
                if ("inherit".equals(value)) {
                    value = "null";
                }
            } else if (type == ViewSchema.NUMBER_TYPE) {
                // No change currently, possibly analyze expressions
                // and default non-constant to when="once" in the
                // future
            } else if (type == ViewSchema.NUMBER_EXPRESSION_TYPE ||
                       type == ViewSchema.SIZE_EXPRESSION_TYPE) {
                // if it's a number that ends in percent:
                if (value.trim().endsWith("%")) {
                    String numstr = value.trim();
                    numstr = numstr.substring(0, numstr.length() - 1);
                    try {
                        double scale = new Float(numstr).floatValue() / 100.0;
                        warnOnDeprecatedConstraints = false;
                        String referenceAttribute = name;
                        if (name.equals("x")) {
                            referenceAttribute = "width";
                        } else if (name.equals("y")) {
                            referenceAttribute = "height";
                        }
                        value = "immediateparent." + referenceAttribute;
                        if (scale != 1.0) {
                            // This special case doesn't change the
                            // semantics, but it generates shorter (since
                            // the sc doesn't fold constants) and more
                            // debuggable code
                            value += "\n * " + scale;
                        }
                        // fall through to the reference case
                    } catch (NumberFormatException e) {
                        // fall through
                    }
                }
                // if it's a literal, treat it the same as a number
                try {
                    new Float(value); // for effect, to generate the exception
                    when = WHEN_IMMEDIATELY;
                } catch (NumberFormatException e) {
                    // It's not a constant, unless when has been
                    // specified, default to a constraint
                    if (when.equals(WHEN_IMMEDIATELY)) {
                        if (warnOnDeprecatedConstraints) {
                            env.warn(
                                "The value of the '" + name + "' attribute uses a deprecated syntax. " +
                                "Use " + name + "=\"${" + value + "}\" instead.",
                                element);
                        }
                        when = WHEN_ALWAYS;
                    }
                }
            } else if (type == ViewSchema.EVENT_TYPE) {
                // Oddball case -- short-circuit when altogether
                return new CompiledAttribute(
                    CompiledAttribute.EVENT,
                    "function " +
                    parent_name + "_" + name + "_event" +
                    " () {" +
                    "\n#pragma 'withThis'\n" +
                    "{\n#beginAttributeStatements\n" +
                    srcloc + value + "\n#endAttributeStatements\n}}");
            } else if (type == ViewSchema.REFERENCE_TYPE) {
                // type="reference" is defined to imply when="once"
                // since reference expressions are unlikely to
                // evaluate correctly at when="immediate" time
                if (when.equals(WHEN_IMMEDIATELY)) {
                    when = WHEN_ONCE;
                }
            } else {
                throw new RuntimeException("unknown schema datatype " + type);
            }

            if (canonicalValue == null)
                canonicalValue = value;

            // Handle when cases
            if (when.equals(WHEN_PATH)) {
                return new CompiledAttribute(
                    CompiledAttribute.PATH,
                    srcloc + ScriptCompiler.quote(value) + "\n");
            } else if (when.equals(WHEN_ONCE)) {
                return new CompiledAttribute(
                    CompiledAttribute.REFERENCE,
                    "function " +
                    parent_name + "_" + name + "_once" +
                    " () {" +
                    "\n#pragma 'withThis'\n" +
                    // Use this.setAttribute so that the compiler
                    // will recognize it for inlining.
                    "this.setAttribute(" +
                    ScriptCompiler.quote(name) + " , " +
                    "\n#beginAttribute\n" + srcloc + canonicalValue + "\n#endAttribute\n)}");
            } else if (when.equals(WHEN_ALWAYS)) {
                return new CompiledAttribute(
                    CompiledAttribute.REFERENCE,
                    "function " +
                    parent_name + "_" + name + "_always" +
                    " () {" +
                    "\n#pragma 'constraintFunction'\n" +
                    "\n#pragma 'withThis'\n" +
                    // Use this.setAttribute so that the compiler
                    // will recognize it for inlining.
                    "this.setAttribute(" +
                    ScriptCompiler.quote(name) + ", " +
                    "\n#beginAttribute\n" + srcloc + canonicalValue +
                    "\n#endAttribute\n)}");
            } else if (when.equals(WHEN_IMMEDIATELY)) {
                if (type == ViewSchema.EXPRESSION_TYPE) {
                    return new CompiledAttribute("\n#beginAttribute\n" + srcloc + canonicalValue + "\n#endAttribute");
                } else {
                    // it's already an object, compiled from a CSS list
                    return new CompiledAttribute(canonicalValue);
                }
            } else {
                throw new CompilationError("invalid when value '" +
                                           when + "'", source);
            }
        }

    void addAttributeElement(Element element) {
        String name = "";
        try {
            name = XMLUtils.requireAttributeValue(element, "name");
        } catch (MissingAttributeException e) {
            throw new CompilationError("'name' is a required attribute of <" + element.getName() + ">", element);
        }

        String value = element.getAttributeValue("value");
        String when = element.getAttributeValue("when");
        String typestr = element.getAttributeValue("type");
        Element parent = element.getParent();
        String parent_name = parent.getAttributeValue("id");

        if (parent_name == null) {
            parent_name = CompilerUtils.attributeUniqueName(element, name);
        }

        // Default when according to parent
        if (when == null) {
            when = this.getAttributeValueDefault(
                name, "when", WHEN_IMMEDIATELY);
        }

        Schema.Type type = null;
        Schema.Type parenttype = null;

        try {
            if (parent.getName().equals("class")) {
                parenttype = getAttributeTypeInfoFromSuperclass(parent, name);
            }  else {
                parenttype = schema.getAttributeType(parent, name);
            }
        } catch (UnknownAttributeException e) {
            // If attribute type is not defined on parent, leave
            // parenttype null.  The user can define it however they
            // like.
        }

        if (typestr == null) {
            // Did user supply an explicit attribute type?
            //  No.  Default to parent type if there is one, else
            // EXPRESSION type.
            if (parenttype == null) {
                type = ViewSchema.EXPRESSION_TYPE;
            } else {
                type = parenttype;
            }
        } else {
            // parse attribute type and compare to parent type
            type = schema.getTypeForName(typestr);
            if (type == null) {
                throw new CompilationError("unknown attribute type: " + typestr, element);
            }
            // If we are trying to declare the attribute with a
            // conflicting type to the parent, throw an error
            if (parenttype != null && type != parenttype) {
                env.warn(
                    new CompilationError(
                        element,
                        name,
                        new Throwable(
                            "In element '" + parent.getName() +
                            "' attribute '"+ name + "' with type '"+type.toString() +
                            "' is overriding parent class attribute with same name but different type: "+
                            parenttype.toString())
                        )
                    );
            }
        }

        // Don't initialize an attribute that is only declared.
        if (value != null) {
            CompiledAttribute cattr = compileAttribute(element, name,
                                                       value, type,
                                                       when);
            addAttribute(cattr, name, attrs, events, references, paths);
        }

        // Add entry for attribute setter function
        String setter = element.getAttributeValue("setter");
        // Backward compatibility
        if (setter == null) {
            setter = element.getAttributeValue("onset");
        }
        if (setter != null) {
            String srcloc =
                CompilerUtils.sourceLocationDirective(element, true);
            // Maybe we need a new type for "function"?
            String setterfn =
                srcloc + "function " +
                parent_name + "_" + name + "_onset" +
                " (" + name + ") {" +
                "\n#pragma 'withThis'\n" +
                srcloc + setter + "\n}";

            if (setters.get(name) != null) {
                env.warn(
                    "a setter for attribute named '"+name+
                    "' is already defined on "+getMessageName(),
                    element);
            }

            setters.put(name, setterfn);
        }
    }

    boolean hasAttribute(String name) {
        return attrs.containsKey(name);
    }

    void removeAttribute(String name) {
        attrs.remove(name);
    }

    void setAttribute(String name, Object value) {
        attrs.put(name, value);
    }

    void addText() {
        if (schema.hasHTMLContent(element)) {
            String text = TextCompiler.getHTMLContent(element);
            if (text.length() != 0) {
                if (!attrs.containsKey("text")) {
                    attrs.put("text", ScriptCompiler.quote(text));
                }
            }
        } else if (schema.hasTextContent(element)) {
            String text;
            // The current inputtext component doesn't understand
            // HTML, but we'd like to have some way to enter
            // linebreaks in the source.
            text = TextCompiler.getInputText(element);
            if (text.length() != 0) {
                if (!attrs.containsKey("text")) {
                    attrs.put("text", ScriptCompiler.quote(text));
                }
            }
        }
    }

    void updateAttrs() {
        if (!setters.isEmpty()) {
            attrs.put("$setters", setters);
        }
        if (!delegateList.isEmpty()) {
            attrs.put("$delegates", delegateList);
        }
        if (!events.isEmpty()) {
            attrs.put("$events", events);
        }
        if (!references.isEmpty()) {
            attrs.put("$refs", references);
        }
        if (!paths.isEmpty()) {
            attrs.put("$paths", paths);
        }
    }

    Map asMap() {
        Map map = new HashMap();
        updateAttrs();
        map.put("name", ScriptCompiler.quote(className));
        map.put("attrs", attrs);
        if (id != null) {
            map.put("id", ScriptCompiler.quote(id));
        }
        if (!children.isEmpty()) {
            List childMaps = new Vector(children.size());
            for (Iterator iter = children.iterator(); iter.hasNext(); )
                childMaps.add(((NodeModel) iter.next()).asMap());
            map.put("children", childMaps);
        }
        return map;
    }

    /** Expand eligible instances by replacing the instance by the
     * merge of its class definition with the instance content
     * (attributes and children).  An eligible instance is an instance
     * of a compile-time class, that doesn't contain any merge
     * stoppers.  If the class and the instance contain a member with
     * the same name, this is a merge stopper.  In the future, this
     * restriction may be relaxed, but will probably always include
     * the case where a class and instance have a member with the same
     * name and the instance name calls a superclass method. */
    NodeModel expandClassDefinitions() {
        NodeModel model = this;
        while (true) {
            ClassModel classModel = schema.getClassModel(model.className);
            if (classModel == null)
                break;
            if (classModel.getSuperclassName() == null)
                break;
            if (!classModel.getInline())
                break;
            model = classModel.applyClass(model);
            // Iterate to allow for the original classes superclass to
            // be expanded as well.
        }
        // Recurse. Make a copy so we can replace the child list.
        // TODO [2004-0604]: As an optimization, only do this if one
        // of the children changed.
        model = (NodeModel) model.clone();
        for (ListIterator iter = model.children.listIterator();
             iter.hasNext(); ) {
            NodeModel child = (NodeModel) iter.next();
            iter.set(child.expandClassDefinitions());
        }
        return model;
    }

    /** Replace members of this with like-named members of source. */
    void updateMembers(NodeModel source) {
        final String OPTIONS_ATTR_NAME = "options";

        // FIXME [2004-06-04]: only compare events with the same reference
        if (CollectionUtils.containsAny(
                events.normalizedKeySet(), source.events.normalizedKeySet())) {
            Collection sharedEvents = CollectionUtils.intersection(
                events.normalizedKeySet(), source.events.normalizedKeySet());
            throw new CompilationError(
                "Both the class and the instance or subclass define the " +
                new ChoiceFormat("1#event |1<events ").format(sharedEvents.size()) +
                new ListFormat("and").format(sharedEvents));
        }

        // Check for duplicate methods.  Collect all the keys that name
        // a Function in both the source and target.
        List sharedMethods = new Vector();
        for (Iterator iter = attrs.normalizedKeySet().iterator();
             iter.hasNext(); ) {
            String key = (String) iter.next();
            if (attrs.get(key) instanceof Function &&
                source.attrs.get(key) instanceof Function)
                sharedMethods.add(key);
        }
        if (!sharedMethods.isEmpty())
            throw new CompilationError(
                "Both the class and the instance or subclass define the method" +
                new ChoiceFormat("1# |1<s ").format(sharedMethods.size()) +
                new ListFormat("and").format(sharedMethods));

        // Check for attributes that have a value in this and
        // a setter in the source.  These can't be merged.
        Collection overriddenAttributes = CollectionUtils.intersection(
            attrs.normalizedKeySet(), source.setters.normalizedKeySet());
        if (!overriddenAttributes.isEmpty())
            throw new CompilationError(
                "A class that defines a value can't be inlined against a " +
                "subclass or instance that defines a setter.  The following " +
                new ChoiceFormat("1#attribute violates|1<attributes violate")
                .format(overriddenAttributes.size()) +
                " this condition: " +
                new ListFormat("and").format(overriddenAttributes));

        // Do the actual merge.
        id = source.id;
        if (source.initstage != null)
            initstage = source.initstage;
        Object options = attrs.get(OPTIONS_ATTR_NAME);
        Object sourceOptions = source.attrs.get(OPTIONS_ATTR_NAME);
        attrs.putAll(source.attrs);
        if (options instanceof Map && sourceOptions instanceof Map) {
            System.err.println(options);
            System.err.println(sourceOptions);
            Map newOptions = new HashMap((Map) options);
            newOptions.putAll((Map) sourceOptions);
            attrs.put(OPTIONS_ATTR_NAME, newOptions);
        }
        delegates.putAll(source.delegates);
        events.putAll(source.events);
        references.putAll(source.references);
        paths.putAll(source.paths);
        setters.putAll(source.setters);
        delegateList.addAll(source.delegateList);
        // TBD: warn on children that share a name?
        // TBD: update the node count
        children.addAll(source.children);
    }
}
