/* ****************************************************************************
 * ChainedException.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.utils;

import java.io.PrintStream;

/** A checked chained exception. */
public class ChainedException extends RuntimeException
{
    Throwable cause = null;
    
    /** Constructs an instance.
     *
     * @param message a string
     */
    public ChainedException(String message) {
        super(message);
    }

    /** Constructs a new runtime exception with the specified detail
     * message and cause.
     *
     * @param message the detail message (which is saved for later retrieval by
     * the Throwable.getMessage() method).
     * @param cause the cause (which is saved for later retrieval by the
     * Throwable.getCause() method). (A null value is permitted, and indicates
     * that the cause is nonexistent or unknown.)
     */    
    public ChainedException(String message, Throwable cause) {
        super(cause.getClass().getName() + ": " + message);
        this.cause = cause;
    }

    /** Constructs a new runtime exception with the specified cause, which
     * typically contains the class and detail message of cause.
     *
     * @param cause the cause, which is saved for later retrieval by the
     * Throwable.getCause() method. A null value is permitted, and indicates
     * that the cause is nonexistent or unknown.
    */
    public ChainedException(Throwable cause) {
        super(cause.getClass().getName() + ": " + cause.getMessage());
        this.cause = cause;
    }

    /** Returns the cause of this throwable or null if the cause is nonexistent
     * or unknown. (The cause is the throwable that caused this throwable to get
     * thrown.)
     *
     * @return the cause of this throwable or null if the cause is nonexistent
     * or unknown.
     */
    public Throwable getCause() {
        return this.cause;
    }

    /** Prints this throwable and its backtrace to the specified print stream.
     *
     * @param s PrintStream to use for output.
     */
    public void printStackTrace(PrintStream s) {
        super.printStackTrace(s);
        if (cause != null) {
            s.print("Caused by: ");
            cause.printStackTrace(s);
        }
    }

    /** Prints this throwable and its backtrace to the specified print stream.
     *
     * @param s PrintWriter to use for output.
     */
    public void printStackTrace(java.io.PrintWriter pw) {
        super.printStackTrace(pw);
        if (cause != null) {
            pw.println("Caused by:");
            cause.printStackTrace(pw);
        }
    }
}
