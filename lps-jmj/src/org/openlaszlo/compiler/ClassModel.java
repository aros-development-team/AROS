/*****************************************************************************
 * ClassModel.java
 *****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import java.util.*;
import org.jdom.Element;

class ClassModel {
    protected final ViewSchema schema;
    protected final String className;
    // This is null for builtin classes
    protected final ClassModel superclass;
    // This is null for builtin classes
    protected final Element definition;
    protected NodeModel nodeModel;
    
    /* If superclass is a predefined system class, just store its name. */
    protected String superclassName = null;
    protected boolean hasInputText = false;
    protected boolean isInputText = false;
    
    /* Class or superclass has an <attribute type="text"/>  */
    protected boolean supportsTextAttribute = false;
    /** Map attribute name to type */
    protected final Map attributeSpecs = new HashMap();
    /** Map of method names to arglist */
    protected final Map methods = new HashMap();
    protected boolean inline = false;
    
    public String toString() {
        return "ClassModel: className="+className + ", " + 
            "superclass=" + superclass + ", " + 
            "superclassName=" + superclassName + ", " + 
            "hasInputText=" + hasInputText + ", " + 
            "isInputText=" + isInputText + ", " + 
            "definition=" + definition;
    }

    // Construct a user-defined class
    ClassModel(String className, ClassModel superclass,
               ViewSchema schema, Element definition) {
        this.className = className;
        this.superclass = superclass;
        this.definition = definition;
        this.schema = schema;
    }
    
    // Construct a builtin class
    ClassModel(String className, ViewSchema schema) {
        this(className, null, schema, null);
    }
    
    /** Returns true if this is equal to or a subclass of
     * superclass. */
    boolean isSubclassOf(ClassModel superclass) {
        if (this == superclass) return true;
        if (this.superclass == null) return false;
        return this.superclass.isSubclassOf(superclass);
    }
    
    boolean isBuiltin() {
        return superclass == null;
    }
    
    ClassModel getSuperclassModel() {
        return superclass;
    }
    
    String getSuperclassName() {
        if (superclassName != null) {
            return superclassName; 
        } else if (superclass == null) {
            return null;
        }  else {
            return superclass.className;
        }
    }
    
    /** Return the AttributeSpec for the attribute named attrName.  If
     * the attribute is not defined on this class, look up the
     * superclass chain.
     */
    AttributeSpec getAttribute(String attrName) {
        AttributeSpec attr = (AttributeSpec) attributeSpecs.get(attrName);
        if (attr != null) {
            return attr;
        } else if (superclass != null) {
            return(superclass.getAttribute(attrName));
        } else {
            return null;
        }
    }

    /** Find an attribute name which is similar to attrName, or return
     * null.  Used in compiler warnings. */
    AttributeSpec findSimilarAttribute(String attrName) {
        for (Iterator iter = attributeSpecs.values().iterator(); iter.hasNext();) {
            AttributeSpec attr = (AttributeSpec) iter.next();
            if ((attrName.toLowerCase().equals(attr.name.toLowerCase())) ||
                (attrName.toLowerCase().startsWith(attr.name.toLowerCase())) ||
                (attrName.toLowerCase().endsWith(attr.name.toLowerCase())) ||
                (attr.name.toLowerCase().startsWith(attrName.toLowerCase())) ||
                (attr.name.toLowerCase().endsWith(attrName.toLowerCase()))) {
                return attr;
            }
        }
        // if that didn't work, try the supeclass
        if (superclass == null) {
            return null;
        } else {
            return superclass.findSimilarAttribute(attrName);
        }
    }

    ViewSchema.Type getAttributeTypeOrException(String attrName)
        throws UnknownAttributeException
    {
        AttributeSpec attr = getAttribute(attrName);
        if (attr != null) {
            return attr.type;
        }  
        // If there is no superclass attribute, use the default static
        // attribute map
        ViewSchema.Type type = ViewSchema.getAttributeType(attrName);
        // Last resort, use default of 'expression' type
        if (type == null) {
            throw new UnknownAttributeException();
        }
        return type;
    }
    
    ViewSchema.Type getAttributeType(String attrName) {
        AttributeSpec attr = getAttribute(attrName);
        if (attr != null) {
            return attr.type;
        }  
        // If there is no superclass attribute, use the default static
        // attribute map
        ViewSchema.Type type = ViewSchema.getAttributeType(attrName);
        // Last resort, use default of 'expression' type
        if (type == null) {
            type = ViewSchema.EXPRESSION_TYPE;
        }
        return type;
    }
    
    void setNodeModel(NodeModel model) {
        this.nodeModel = model;
    }

    boolean getInline() {
        return inline && nodeModel != null;
    }
    
    void setInline(boolean inline) {
        this.inline = inline;
    }
    
    public static class InlineClassError extends CompilationError {
        public InlineClassError(ClassModel cm, NodeModel im, String message) {
            super(
                "The class " + cm.className + " has been declared " +
                "inline-only but cannot be inlined.  " + message + ". " +
                "Remove " + cm.className + " from the <?lzc class=\"" +
                cm.className + "\"> or " + "<?lzc classes=\"" + cm.className
                + "\"> processing instruction to remove this error.",
                im.element);
        }
    }
    
    protected boolean descendantDefinesAttribute(NodeModel model, String name) {
        for (Iterator iter = model.getChildren().iterator(); iter.hasNext(); ) {
            NodeModel child = (NodeModel) iter.next();
            if (child.hasAttribute(name) || descendantDefinesAttribute(child, name))
                return true;
        }
        return false;
    }
    
    NodeModel applyClass(NodeModel instance) {
        final String DEFAULTPLACEMENT_ATTR_NAME = "defaultPlacement";
        final String PLACEMENT_ATTR_NAME = "placement";
        if (nodeModel == null) throw new RuntimeException("no nodeModel for " + className);
        if (nodeModel.hasAttribute(DEFAULTPLACEMENT_ATTR_NAME))
            throw new InlineClassError(this, instance, "The class has a " + DEFAULTPLACEMENT_ATTR_NAME + " attribute");
        if (instance.hasAttribute(DEFAULTPLACEMENT_ATTR_NAME))
            throw new InlineClassError(this, instance, "The instance has a " + DEFAULTPLACEMENT_ATTR_NAME + " attribute");
        if (descendantDefinesAttribute(instance, PLACEMENT_ATTR_NAME))
            throw new InlineClassError(this, instance, "An element within the instance has a " + PLACEMENT_ATTR_NAME + " attribute");
        
        try {
            // Replace this node by the class model.
            NodeModel model = (NodeModel) nodeModel.clone();
            // Set $classrootdepth on children of the class (but not the
            // instance that it's applied to)
            setChildrenClassRootDepth(model, 1);
            model.updateMembers(instance);
            model.setClassName(getSuperclassName());
            return model;
        } catch (CompilationError e) {
            throw new InlineClassError(this, instance, e.getMessage());
        }
    }
    
    protected void setChildrenClassRootDepth(NodeModel model, int depth) {
        final String CLASSROOTDEPTH_ATTRIBUTE_NAME = "$classrootdepth";
        for (Iterator iter = model.getChildren().iterator(); iter.hasNext(); ) {
            NodeModel child = (NodeModel) iter.next();
            // If it has already been set, this child is the result of
            // a previous inline class expansion with a different
            // classroot.
            if (child.hasAttribute(CLASSROOTDEPTH_ATTRIBUTE_NAME))
                continue;
            child.setAttribute(CLASSROOTDEPTH_ATTRIBUTE_NAME,
                               new Integer(depth));
            int childDepth = depth;
            ClassModel childModel = child.getClassModel();
            // If this is an undefined class, childModel will be null.
            // This is an error, and other code signals a compiler
            // warning. This test keeps it from resulting in a stack
            // trace too.
            if (childModel != null && childModel.isSubclassOf(schema.getClassModel("state")))
                childDepth++;
            setChildrenClassRootDepth(child, childDepth);
        }
    }
}
