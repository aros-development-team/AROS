/* *****************************************************************************
 * CompilationErrorHandler.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import org.jdom.Element;
import org.openlaszlo.utils.ChainedException;
import java.lang.Integer;
import java.io.*;
import java.util.*;
import org.openlaszlo.server.*;
import org.openlaszlo.utils.*;
import org.openlaszlo.xml.internal.*;

/** Hold a list of errors generated during compilation of an lzx file.
 *
 * The list of Errors are all wrapped by CompilationError
 */
public class CompilationErrorHandler {
    /** List of errors, these should all be of type CompilationError  */
    private List errors = new Vector();
    protected String fileBase = "";
    
    public CompilationErrorHandler() {
    }
    
    /** Set the base directory relative to which pathnames are
     * reported. */
    public void setFileBase(String fileBase) {
        this.fileBase = fileBase;
    }
    
    /** Append an error to list of errors.
     * @param error the error which occurred
     */
    void addError(CompilationError e) {
        e.setFileBase(fileBase);
        errors.add(e);
    }

    /** Returns the list of errors
     * @return the list of errors
     */
    public List getErrors()
    {
        return errors;
    }

    /**
     * @return length of error list */
    public int size()
    {
        return errors.size();
    }

    public boolean isEmpty() {
        return errors.isEmpty();
    }

    /**  @return a single consolidated error which holds list of error
        message strings which were collected during a compile.
     */
    public CompilationError toCompilationError() {
        StringBuffer buffer = new StringBuffer();
        for (Iterator iter = errors.iterator(); iter.hasNext(); ) {
            CompilationError error = (CompilationError) iter.next();
            buffer.append(error.getMessage());
            if (iter.hasNext()) {
                buffer.append('\n');
            }
        }
        return new CompilationError(buffer.toString());
    }
    
    public String toXML() {
        StringBuffer buffer = new StringBuffer();
        for (Iterator iter = errors.iterator(); iter.hasNext(); ) {
            CompilationError error = (CompilationError) iter.next();
            buffer.append(error.toXML());
            if (iter.hasNext()) {
                buffer.append("<br/>");
            }
        }
        return buffer.toString();
    }
}
