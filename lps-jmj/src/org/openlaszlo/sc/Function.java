/* *****************************************************************************
 * Function.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.sc;

/** 
 *
 * @author Oliver Steele
 */
public class Function {
    private final String name;
    private final String args;
    private final String body;
    
    public Function(String body) {
        this("", body);
    }
    
    public Function(String args, String body) {
        this("", args, body);
    }
    
    public Function(String name, String args, String body) {
        this.name = name;
        this.args = args;
        this.body = body;
    }

    public String toString() {
        return "function " + name + "\n(" + args + "\n) {" + body + "\n}";
    }
}
