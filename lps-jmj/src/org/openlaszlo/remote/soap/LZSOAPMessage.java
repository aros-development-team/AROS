/* *****************************************************************************
 * LZSOAPMessage.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.remote.soap;

import java.util.*;

public class LZSOAPMessage
{
    String mName;
    String mMode;
    String mUse = null;
    Set mPartNames = null;
    List mParts = null;

    /**
     * @param name name of soap message.
     * @param type one of input or output.
     */
    public LZSOAPMessage(String name, String mode) {
        mName = name;
        mMode = mode;
    }

    public String getName() {
        return mName;
    }

    public String getMode() {
        return mMode;
    }

    public void setUse(String use) {
        mUse = use;
    }

    public String getUse() {
        return mUse;
    }

    public Set getPartNames() {
        return mPartNames;
    }

    public void setPartNames(Set partNames) {
        mPartNames = partNames;
    }

    public void setParts(List parts) {
        mParts = parts;
    }

    public List getParts() {
        return mParts;
    }

    String toPartString() {
        if (mParts == null) {
            return "            -- no parts --";
        }

        StringBuffer buf = new StringBuffer(); 
        for (int i=0; i < mParts.size(); i++) {
            LZSOAPPart part = (LZSOAPPart)mParts.get(i);
            buf.append(part).append("\n");
        }
        return buf.toString();
    }

    public String toString() {
        return "        -- SOAPMessage --\n"
            + "        name=" + mName + "\n"
            + "        mode=" + mMode + "\n"
            + "        use=" + mUse + "\n"
            + "        partnames=" + mPartNames + "\n\n"
            + toPartString() + "\n";
    }
}
