/******************************************************************************
 * Option.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.server;

import java.io.*;
import java.util.*;

import org.apache.log4j.Logger;
import org.jdom.*;
import org.apache.regexp.RE;
import org.apache.regexp.RESyntaxException;

/**
 * An Option contains a set of deny and allow patterns.
 * 
 * @author Eric Bloch
 * @version 1.0 
 */
public class Option implements Serializable {

    private static Logger mLogger  = Logger.getLogger(Option.class);
    
    // No need to sync; access is read only after construction. These lists hold
    // regexp objects.
    private ArrayList allowList = null;
    private ArrayList deniesList = null;

    // These lists hold the pattern for the regexp objects.
    private ArrayList allowSerializableList = null;
    private ArrayList deniesSerializableList = null;

    void init() {
        if (allowList == null)
            allowList  = new ArrayList();
        if (deniesList == null)
            deniesList = new ArrayList();        
        if (allowSerializableList == null) 
            allowSerializableList = new ArrayList();
        if (deniesSerializableList == null)
            deniesSerializableList = new ArrayList();
    }

    Option() {
        init();
    }

    /**
     * Constructs a new option
     * @param elt
     */
    Option(Element el) {
        this();
        addElement(el);
    }
    
    /**
     * Add new patterns to option
     */
    public void addElement(Element el) {
        List list = el.getChildren();
        ListIterator iter = list.listIterator();
        
        Element a = el.getChild("allow", el.getNamespace());
        if (a != null)
            addPatterns(allowList, allowSerializableList, a);
        Element d = el.getChild("deny", el.getNamespace());
        if (d != null)
            addPatterns(deniesList, deniesSerializableList, d);
    }

    /**
     * @param l list to add REs to
     * @param element that has pattern element children
     */
    private void addPatterns(ArrayList list, ArrayList serializableList, Element elt) {
        ListIterator iter = 
            elt.getChildren("pattern", elt.getNamespace()).listIterator();

        while(iter.hasNext()) {
            String p = ((Element)iter.next()).getTextNormalize();
            mLogger.debug(elt.getName() + ": " + p);
            try {
                RE re = new RE(p);
                list.add(re);
                serializableList.add(p);
            } catch (RESyntaxException e) {
                mLogger.error("ignoring bad regexp syntax: " + p);
                continue;
            }
        }
    }

    /**
     * @param val value to check against
     * @param allow if true, an undefined option means that it is allowed, else
     * it is denied.
     * @return true if this option allows the given value
     */
    boolean allows(String val, boolean allow) {

        mLogger.debug("checking: " + val);

        // If we don't specify what's allowed, allow all.
        if (!allowList.isEmpty())  {
            allow = false;
            ListIterator iter = allowList.listIterator();
            while (iter.hasNext()) {
                RE re = (RE)iter.next();
                if (re.match(val)) {
                    allow = true;
                    break;
                }
            } 
        }

        if (allow) {
            ListIterator iter = deniesList.listIterator();
            while (iter.hasNext()) {
                RE re = (RE)iter.next();
                if (re.match(val)) {
                    allow = false;
                    break;
                }
            } 
        }
        return allow;
    }


    /**
     * Handle object serialization.
     */
    private void writeObject(ObjectOutputStream out)
        throws IOException {

        ListIterator iter;

        // write out allow
        out.writeInt(allowSerializableList.size());
        iter = allowSerializableList.listIterator();
        while (iter.hasNext()) {
            out.writeObject(iter.next());
        }

        // write out denies
        out.writeInt(deniesSerializableList.size());
        iter = deniesSerializableList.listIterator();
        while (iter.hasNext()) {
            out.writeObject(iter.next());
        }
    }

    /**
     * Handle object deserialization.
     */
    private void readObject(ObjectInputStream in)
        throws IOException, ClassNotFoundException {
        int size, i;

        init();

        size = in.readInt();
        for (i=0; i < size; i++) {
            String p = (String)in.readObject();
            try {
                allowList.add(new RE(p));
                allowSerializableList.add(p);
            } catch (RESyntaxException e) {
                mLogger.error("ignoring bad regexp syntax: " + p);
                continue;
            }
        }

        size = in.readInt();
        for (i=0; i < size; i++) {
            String p = (String)in.readObject();
            try {
                deniesList.add(new RE(p));
                deniesSerializableList.add(p);
            } catch (RESyntaxException e) {
                mLogger.error("ignoring bad regexp syntax: " + p);
                continue;
            }
        }
    }
}
