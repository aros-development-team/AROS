/* *****************************************************************************
 * ComplexType.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.remote.soap;

import java.util.List;
import java.util.Map;
import java.util.Iterator;
import javax.xml.namespace.QName;

class ComplexType
{
    final static public int TYPE_UNKNOWN = 0;
    final static public int TYPE_SIMPLE = 1;
    final static public int TYPE_STRUCT = 2;
    final static public int TYPE_ARRAY = 3;

    QName mName;
    int mType = TYPE_UNKNOWN;
    QName mTypeQName = null;

    String mExtends = null;
    Map mMembers = null;
    List mMemberSequence = null;
    ComplexType mBase = null;

        
    // For objects
    public ComplexType(QName name, int type, Map members) {
        mName = name;
        mType = type;
        setMembers(members);
    }

    // For arrays
    public ComplexType(QName name, int type, QName typeQName) {
        mName = name;
        mType = type;
        mTypeQName = typeQName;
    }

    public boolean isArray() {
        return mType == TYPE_ARRAY;
    }

    public boolean isComplex() {
        return mType == TYPE_STRUCT;
    }

    public QName getName() {
        return mName;
    }

    public int getType() {
        return mType;
    }

    public String getTypeString() {
        switch (mType) {
        case TYPE_SIMPLE: return "simple";
        case TYPE_STRUCT: return "struct";
        case TYPE_ARRAY: return "array";
        default: 
            return "unknown";
        }
    }

    public QName getTypeQName() {
        return mTypeQName;
    }

    public Map getMembers() {
        return mMembers;
    }

    public List getMemberSequence() {
        return mMemberSequence;
    }

    public boolean sequenceMatters() {
        return mMemberSequence != null;
    }

    public void setBase(ComplexType base) {
        mBase = base;
    }

    public ComplexType getBase() {
        return mBase;
    }

    /**
     * @param members values of members are QName.
     */
    public void setMembers(Map members) {
        mMembers = members;
    }

    /**
     * @param members values of members are QName.
     * @param sequence order of members.
     */
    public void setMembers(Map members, List sequence) {
        mMembers = members;
        mMemberSequence = sequence;
    }


    String toStringMembers() {
        if (mMembers == null) {
            return "";
        }

        StringBuffer buf = new StringBuffer();
        if (mBase != null) {
            buf.append(mBase.toStringMembers());
            buf.append("\n        - extended members -\n");
        }

        Iterator iter = mMembers.entrySet().iterator();
        while (iter.hasNext()) {
            Map.Entry entry = (Map.Entry)iter.next();
            String key = (String)entry.getKey();
            QName qname =(QName)entry.getValue();
            buf.append("        ").append(key)
                .append("=").append(qname).append("\n");
        }
        return buf.toString();
    }


    public String toString() {
        return "    -- ComplexType --\n" +
            "    name=" + mName + "\n" +
            "    type=" + getTypeString() + "\n" +
            "    base=" + (mBase == null ? "" : mBase.mName.toString()) + "\n" +
            (mType == TYPE_ARRAY   ? "    typeQName=" + mTypeQName + "\n" : "") +
            "\n        -- members --\n" +
            toStringMembers();
    }
}
