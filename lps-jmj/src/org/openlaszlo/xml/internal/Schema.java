/* ****************************************************************************
 * Schema.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.xml.internal;
import org.jdom.Element;
import java.util.*;

/**
 * Describes the content model of an XML document.
 *
 * @author Oliver Steele
 * @version 1.0
 */
public abstract class Schema {
    
    /** Hold mapping from Javascript type names to Types */
    static Map typeNames = new HashMap();

    /** Represents the type of an attribute whose type isn't known. */
    public static final Type UNKNOWN_TYPE = newType("unknown");
    /** Represents a String. */
    public static final Type STRING_TYPE = newType("string");
    /** Represents a number. */
    public static final Type NUMBER_TYPE = newType("number");
    /** Represents an XML ID. */
    public static final Type ID_TYPE = newType("ID");

    /** The type of an attribute. */
    public static class Type {
        private String mName;

        public Type(String name) {
            mName = name;
        }
        
        public String toString() {
            return mName;
        }
    }
    
    /** Returns a unique type.
     * @return a unique type
     */
    public static Type newType(String typeName) {
        Type newtype = new Type(typeName);
        typeNames.put(typeName, newtype);
        return newtype;
    }
    
    /** Look up the Type object from a Javascript type name */
    public Type getTypeForName (String typeName) {
        return (Type) typeNames.get(typeName);
    }

    /** An empty Schema, all of whose attribute types are unknown. */
    public static final Schema DEFAULT_SCHEMA =
        new Schema() {
            /** @see Schema */
            public Type getAttributeType(Element element, String name) {
                return UNKNOWN_TYPE;
            }
        };
    
    /**
     * Returns a value representing the type of an attribute within an
     * XML element.
     * 
     * @param element an Element
     * @param attributeName an attribute name
     * @return a value represting the type of the attribute's
     */
    public abstract Type getAttributeType(Element element,
                                         String attributeName);
}
