/* *****************************************************************************
 * CompilationError.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;
import java.io.File;
import java.util.*;
import org.jdom.Element;
import org.jdom.JDOMException;
import org.xml.sax.SAXParseException;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.xml.internal.XMLUtils;
import org.openlaszlo.sc.parser.ParseException;

/** Represents an error that occurred during the invocation of the
 * interface compiler.
 */
public class CompilationError extends RuntimeException {
    /** The name of the file within which the error occurred. */
    private String mPathname = null;
    /** The line number of the error, or null. */
    private Integer mLineNumber = null;
    /** The line number of the error, or null. */
    private Integer mColumnNumber = null;
    /** The element at which the error occurred, or null. */
    private Element mElement = null;
    private String mAttribute = null;
    /** Prefix to remove from pathnames. */
    private String mFileBase = "";
    private List errors = new Vector();

    /** A suggested solution for an error */
    private String solutionMessage = "";

    /** Set this to true to throw compilation errors that have a
     * cause, instead of wrapping them in instances of
     * CompilationError.  This is useful for debugging the
     * compiler. */
    public static boolean ThrowCompilationErrors = false;

    /** Constructs an instance.
     * @param message a string
     */
    public CompilationError(String message) {
        super(message);
    }

    /** Constructs an instance.
     * @param message a string
     * @param element the element within which the error occurred
     */
    public CompilationError(String message, Element element) {
        super(message);
        initElement(element, null);
    }

    /** Constructs an instance.
     * @param element the element within which the error occurred
     * @param cause the chained cause of the error
     */
    public CompilationError(Element element, Throwable cause) {
        super(getCauseMessage(cause));
        initElement(element, cause);
    }

    public CompilationError(Element element, String attribute,
                            Throwable cause) {
        super(getCauseMessage(cause));
        initElement(element, attribute, cause);
    }

    
    public CompilationError(Throwable cause, String solution) {
        this(cause);
        this.solutionMessage = solution;
    }

    /** Constructs an instance.
     * @param cause the chained cause of the error
     */
    public CompilationError(Throwable cause) {
        super(getCauseMessage(cause));
        SAXParseException se = null; // is there a SAXParseException with more location info?
        if (cause instanceof JDOMException) {
            JDOMException je = (JDOMException) cause;
            if (je.getCause() instanceof SAXParseException) {
                 se = (SAXParseException) je.getCause();
            }
        } else if (cause instanceof SAXParseException) {
                se = (SAXParseException) cause;
        }
        if (se != null) {
            initPathname(se.getPublicId());
            setLineNumber(se.getLineNumber());
            setColumnNumber(se.getColumnNumber());
        }
    }

    /** The constructors use this. */
    private static String getCauseMessage(Throwable cause) {
        if (ThrowCompilationErrors) {
            cause.printStackTrace();
            throw new ChainedException(cause);
        }
        if (cause instanceof JDOMException &&
            ((JDOMException) cause).getCause() != null)
            cause = ((JDOMException) cause).getCause();
        String message = cause.getMessage();
        if (cause instanceof java.io.FileNotFoundException) {
            return "file not found: " + message;
        } else if (cause instanceof java.lang.NumberFormatException) {
            return "invalid number: " + message;
        } else if (cause instanceof org.openlaszlo.compiler.ViewSchema.ColorFormatException) {
            return "invalid color: " + message;
        } else if (cause instanceof ParseException) {
            return ((ParseException) cause).getMessage(false);
        } else {
            /*String name = cause.getClass().getName();
            if (name.lastIndexOf('.') > 0)
                name = name.substring(name.lastIndexOf('.')+1);
            if (name.indexOf('$') < 0)
            message = name + ": " + message;*/
            return message;
        }
    }

    public void attachErrors(List errors) {
        this.errors.addAll(errors);
    }
    
    /** Set the 'solution message', a hint as to what might have
        caused this error.
        @param solution a helpful message to be appended to the error message
     */
    public void setSolution(String sol) {
        this.solutionMessage = sol;
    }

    /** Initializes this instance's element to the specified
     * parameter.  May be called at most once, and only if the element
     * wasn't initialized in the constructor.
     * @param element the element at which the error occurred
     */

    void initElement(Element element) {
        initElement(element, null);
    }

    void initElement(Element element, Throwable cause) {
        if (this.mElement != null && mElement != element) {
            throw new IllegalStateException("initElement called twice, on " +
                                            mElement + " and " + element);
        }
        this.mElement = element;
        this.initPathname(Parser.getSourceMessagePathname(element));

        // Prefer the script compiler's suggestion of what line number
        // the error occurred on
        if (cause instanceof ParseException) {
            this.setLineNumber(((ParseException) cause).getBeginLine());
            this.setColumnNumber(((ParseException) cause).getBeginColumn());
        } else {
            this.setLineNumber(Parser.getSourceLocation(element, Parser.LINENO).intValue());
            this.setColumnNumber(Parser.getSourceLocation(element, Parser.COLNO).intValue());
        }
    }

    void initElement(Element element, String attribute, Throwable cause) {
        initElement(element, cause);
        this.mAttribute = attribute;
    }

    /** Returns the element at which the error occurred.
     * @return the element at which the error occurred
     */
    public Element getElement()
    {
        return this.mElement;
    }

    public String getSolutionMessage() {
        return solutionMessage;
    }

    public void setFileBase(String fileBase) {
        this.mFileBase = fileBase;
    }
    
    /** Initializes this instance's pathname to the specified
     * parameter.  May be called at most once, and only if the pathname
     * wasn't initialized in the constructor.
     * @param pathname the name of the file at which the error occurred
     */
    public void initPathname(String pathname) {
        if (mPathname != null && mPathname.intern() != pathname.intern()) {
            throw new IllegalStateException("initPathname called twice, on " +
                                            mPathname + " and " + pathname);
        }
        this.mPathname = pathname;
    }

    /** Returns the name of the file within which this error
     * occurred.
     * @return the name of the file within which the error occurred
     */
    public String getPathname() {
        return mPathname;
    }

    // TODO: [2003-01-16] Once ScriptElementCompiler is fixed not to
    // use this method, it can be removed.
    public void setPathname(String pathname) {
        mPathname = pathname;
    }

    /** Returns the column number at which this error occurred, or null
     * if this can't be determined.
     * @return the line number at which the error occurred
    */
    public Integer getColumnNumber() {
        return mColumnNumber;
    }

    public void setColumnNumber(int columnNumber) {
        mColumnNumber = new Integer(columnNumber);
    }

    public void initColumnNumber(int columnNumber) {
        if (mColumnNumber != null && mColumnNumber.intValue() != columnNumber)
            throw new IllegalStateException("initColumnNumber called twice, on " +
                                            mColumnNumber + " and " + columnNumber);
        setColumnNumber(columnNumber);
    }

    /** Returns the line number at which this error occurred, or null
     * if this can't be determined.
     * @return the line number at which the error occurred
    */
    public Integer getLineNumber() {
        return mLineNumber;
    }

    public void setLineNumber(int lineNumber) {
        mLineNumber = new Integer(lineNumber);
    }

    public void initLineNumber(int lineNumber) {
        if (mLineNumber != null && mLineNumber.intValue() != lineNumber)
            throw new IllegalStateException("initLineNumber called twice, on " +
                                            mLineNumber + " and " + lineNumber);
        setLineNumber(lineNumber);
    }

    /** Return a message describing the error.
     * @param base pathname prefix to strip out from error messages
     * @return a message describing the error
     */
    public String getMessage (File base) {
        return getMessage(base.getAbsolutePath() + File.separator);
    }

    /** Return a message describing the error.
     * @param base pathname prefix to strip out from error messages
     * @return a message describing the error
     */
    public String getMessage (String base) {
        String errmsg = _getMessage();

        if (base != null && errmsg.startsWith(base)) {
            // remove base string prefix
            return errmsg.substring(base.length());
        } else {
            return errmsg;
        }
    }


    /** Return a message describing the error.
     * @return a message describing the error
     */
    private String _getMessage() {
        String message = "";
        if (mPathname != null && !super.getMessage().startsWith(mPathname)) {
            message += mPathname + ":";
            if (mLineNumber != null) {
                message += mLineNumber + ":";
                if (mColumnNumber != null) {
                    message += mColumnNumber + ":";
                }
            }
            message += " ";
        }

        String errorText = super.getMessage();
        // super.getMessage() seems to be returning null for Tomcat
        // xerces/JDOM errors [hqm 11-2003]
        if (errorText == null) {
            errorText = "";
        }
        if (!solutionMessage.equals("") &&
            !errorText.equals("") &&
            !errorText.endsWith(".") && !errorText.endsWith(". ")) {
            errorText += ". ";
        }
        if (!solutionMessage.equals("") &&
            !errorText.endsWith(" ")) {
            errorText += " ";
        }
        return message + errorText + getSolutionMessage();
    }

    public String getMessage() {
        return getMessage(mFileBase + File.separator);
    }
    
    public String toHTML() {
        // todo: properly quote the filename
        String sourceFile = getPathname();
        String message = getMessage();
        String solution = "<font color=\"green\">" + getSolutionMessage() + "</font>";
        if (sourceFile != null && message.startsWith(sourceFile)) {
            message = "<A HREF=\"" + sourceFile + "\">" + sourceFile + "</A>" +
                message.substring(sourceFile.length());
        }
        return "<HTML><HEAD>" +
            "<TITLE>" + "Compilation Error" + "</TITLE>" +
            "</HEAD><BODY>" + message + "<p>" + solution + 
            "</BODY></HTML>";
    }
    
    public String toXML() {
        StringBuffer buffer = new StringBuffer();
        buffer.append("<error>");
        buffer.append(XMLUtils.escapeXml(getMessage()));
        for (Iterator iter = errors.iterator(); iter.hasNext(); )
            buffer.append(((CompilationError) iter.next()).toXML());
        buffer.append("</error>");
        return buffer.toString();
    }

    /**
     * @return the error message w/out the filename and line, col numbers
     * XXX This doesn't seem to work.
     */
    public String getErrorMessage() {
        return super.getMessage();
    }
}
