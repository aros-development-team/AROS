/* ****************************************************************************
 * SimpleNode.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.sc.parser;
import java.io.Serializable;

public abstract class SimpleNode implements Node, Serializable {
    protected final static Node[] noChildren = {};
    protected Node parent;
    protected Node[] children = noChildren;
    protected final int id;
    protected Parser parser;
    public String filename;     // added
    public int beginLine, beginColumn; // added
    public String comment;
  
    public SimpleNode(int i) {
        id = i;
    }

    public SimpleNode(Parser p, int i) {
        this(i);
        parser = p;
    }

    public void jjtOpen() {
    }

    public void jjtClose() {
    }
  
    public void jjtSetParent(Node n) { parent = n; }
    public Node jjtGetParent() { return parent; }

    public void jjtAddChild(Node n, int i) {
        if (i >= children.length) {
            Node c[] = new Node[i + 1];
            System.arraycopy(children, 0, c, 0, children.length);
            children = c;
        }
        children[i] = n;
    }

    public Node jjtGetChild(int i) {
        return children[i];
    }

    public int jjtGetNumChildren() {
        return (children == null) ? 0 : children.length;
    }

    /* You can override these two methods in subclasses of SimpleNode to
       customize the way the node appears when the tree is dumped.  If
       your output uses more than one line you should override
       toString(String), otherwise overriding toString() is probably all
       you need to do. */

    public String toString(String prefix) { return prefix + toString(); }

    /* Override this method if you want to customize how the node dumps
       out its children. */

    public void dump(String prefix) {
        System.out.println(toString(prefix));
        if (children != null) {
            for (int i = 0; i < children.length; ++i) {
                SimpleNode n = (SimpleNode)children[i];
                if (n != null) {
                    n.dump(prefix + " ");
                }
            }
        }
    }
  
    public void dump() {
        dump("");
    }

    // Added

    public SimpleNode() {
        this(0);
    }

    public String toString() {
        String name = this.getClass().getName();
        if (name.lastIndexOf('.') >= 0) {
            name = name.substring(name.lastIndexOf('.') + 1);
        }
        return name;
    }

    public Node[] getChildren() {
        return children;
    }

    public void setChildren(Node[] children) {
        this.children = children;
    }
    
    public Node __getitem__(int n) {
        return jjtGetChild(n);
    }
    
    public int __len__() {
        return jjtGetNumChildren();
    }

    public boolean __nonzero__() {
        return true;
    }

    //public void setLineNumber(int line) {
    //    this.beginLine = line;
    //}
  
    public int getLineNumber() {
        return beginLine;
    }
    
    public int getColumnNumber() {
        return beginColumn;
    }

    public String getFilename() {
        return filename;
    }

    public void setBeginLocation(String filename, int line, int column) {
        this.filename = filename;
        this.beginLine = line;
        this.beginColumn = column;
    }

    public void setComment(String comment) {
        this.comment = comment;
    }

    public String getComment() {
        return this.comment;
    }
}
