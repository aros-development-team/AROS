/*
 * $Id: IVException.java,v 1.4 2002/03/13 01:46:16 skavish Exp $
 *
 * ===========================================================================
 *
 * The JGenerator Software License, Version 1.0
 *
 * Copyright (c) 2000 Dmitry Skavish (skavish@usa.net). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution, if
 *    any, must include the following acknowlegement:
 *    "This product includes software developed by Dmitry Skavish
 *     (skavish@usa.net, http://www.flashgap.com/)."
 *    Alternately, this acknowlegement may appear in the software itself,
 *    if and wherever such third-party acknowlegements normally appear.
 *
 * 4. The name "The JGenerator" must not be used to endorse or promote
 *    products derived from this software without prior written permission.
 *    For written permission, please contact skavish@usa.net.
 *
 * 5. Products derived from this software may not be called "The JGenerator"
 *    nor may "The JGenerator" appear in their names without prior written
 *    permission of Dmitry Skavish.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL DMITRY SKAVISH OR THE OTHER
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

package org.openlaszlo.iv.flash.util;

import java.util.*;
import java.io.*;

/**
 * This exception is thrown if there any errors during generation process.
 * <P>
 * The exception can be created with message from resource bundles
 * All messages in this file are defined by their keys which are to be used when throwing the exception.
 * <p>
 * Usage:<BR>
 * <UL>
 * <LI>wrap some other exception into IVException and rethrow it with a message from resorce and parameters:<br>
 *     <pre><CODE>
 *     ...
 *     } catch( IOException e ) {
 *       throw new IVException( e, Resource.ERRCMDFILEREAD, url.getName(), getCommandName() );
 *     }
 *     </CODE></pre>
 * <LI><CODE>throw new IVException( Resource.INFINITELOOP );</CODE>
 * </UL>
 *
 * @author Dmitry Skavish
 */
public class IVException extends Exception {

    private String message;
    private Throwable cause;
    private String key;
    private Object[] parms;
    private ResourceBundle bundle;

    public IVException() {
    }

    public IVException( String key ) {
        this.key = key;
    }

    public IVException( ResourceBundle bundle, String key ) {
        this.key = key;
        this.bundle = bundle;
    }

    public IVException( String key, Object[] parms ) {
        this.key = key;
        this.parms = parms;
    }

    public IVException( ResourceBundle bundle, String key, Object[] parms ) {
        this.key = key;
        this.parms = parms;
        this.bundle = bundle;
    }

    public IVException( String key, Throwable cause ) {
        this.key = key;
        this.cause = cause;
    }

    public IVException( ResourceBundle bundle, String key, Throwable cause ) {
        this.key = key;
        this.cause = cause;
        this.bundle = bundle;
    }

    public IVException( String key, Object[] parms, Throwable cause ) {
        this.key = key;
        this.parms = parms;
        this.cause = cause;
    }

    public IVException( ResourceBundle bundle, String key, Object[] parms, Throwable cause ) {
        this.key = key;
        this.parms = parms;
        this.cause = cause;
        this.bundle = bundle;
    }

    public IVException( Throwable cause ) {
        this.cause = cause;
    }

    public Throwable getCause() {
        return cause;
    }

    public String getMessageKey() {
        return key;
    }

    public String getMessage() {
        if( message == null ) {
            String ourMsg = bundle==null? Log.getMessage(key, parms): Log.getMessage(bundle, key, parms);
            StringBuffer msg = new StringBuffer();
            if (ourMsg != null) {
                msg.append(ourMsg);
            }
            if (getCause() != null) {
                String causeMsg = getCause().getMessage();
                if (causeMsg != null) {
                    if (ourMsg != null) {
                        msg.append("\n\t-> ");
                    }
                    msg.append(causeMsg);
                }
            }
            message = msg.toString();
        }
        return message;
    }

    /**
     * Prints this throwable and its backtrace to the specified print stream.
     *
     * @param s <code>PrintStream</code> to use for output
     */
    public void printStackTrace(PrintStream s) {
        if( Util.javaVersion >= 1.4 ) super.printStackTrace(s);
        else {
            synchronized (s) {
                s.println(this);
                IVVector trace = getStackTrace(this);
                for (int i=0; i < trace.size(); i++)
                    s.println(trace.elementAt(i));

                Throwable ourCause = getCause();
                if (ourCause != null)
                    printStackTraceAsCause(ourCause, s, trace);
            }
        }
    }

    /**
     * Print our stack trace as a cause for the specified stack trace.
     */
    private static void printStackTraceAsCause(Throwable t, PrintStream s, IVVector causedTrace) {

        // Compute number of frames in common between this and caused
        IVVector trace = getStackTrace(t);
        int m = trace.size()-1, n = causedTrace.size()-1;
        while (m >= 0 && n >=0 && trace.elementAt(m).equals(causedTrace.elementAt(n)) ) {
            m--; n--;
        }
        int framesInCommon = trace.size() - 1 - m;

        s.println("caused by: " + t);
        for (int i=0; i <= m; i++)
            s.println(trace.elementAt(i));
        if (framesInCommon != 0)
            s.println("\t... " + framesInCommon + " more");

        // Recurse if we have a cause
        if( t instanceof IVException ) {
            Throwable ourCause = ((IVException)t).getCause();
            if (ourCause != null)
                printStackTraceAsCause(ourCause, s, trace);
        }
    }

    /**
     * Prints this throwable and its backtrace to the specified
     * print writer.
     *
     * @param s <code>PrintWriter</code> to use for output
     * @since   JDK1.1
     */
    public void printStackTrace(PrintWriter s) {
        if( Util.javaVersion >= 1.4 ) super.printStackTrace(s);
        else {
            synchronized (s) {
                s.println(this);
                IVVector trace = getStackTrace(this);
                for (int i=0; i < trace.size(); i++)
                    s.println(trace.elementAt(i));

                Throwable ourCause = getCause();
                if (ourCause != null)
                    printStackTraceAsCause(ourCause, s, trace);
            }
        }
    }

    /**
     * Print our stack trace as a cause for the specified stack trace.
     */
    private static void printStackTraceAsCause(Throwable t, PrintWriter s, IVVector causedTrace) {

        // Compute number of frames in common between this and caused
        IVVector trace = getStackTrace(t);
        int m = trace.size()-1, n = causedTrace.size()-1;
        while (m >= 0 && n >=0 && trace.elementAt(m).equals(causedTrace.elementAt(n)) ) {
            m--; n--;
        }
        int framesInCommon = trace.size() - 1 - m;

        s.println("caused by: " + t);
        for (int i=0; i <= m; i++)
            s.println(trace.elementAt(i));
        if (framesInCommon != 0)
            s.println("\t... " + framesInCommon + " more");

        // Recurse if we have a cause
        if( t instanceof IVException ) {
            Throwable ourCause = ((IVException)t).getCause();
            if (ourCause != null)
                printStackTraceAsCause(ourCause, s, trace);
        }
    }

    private void super_printStackTrace( PrintWriter pw ) {
        super.printStackTrace(pw);
    }

    private static IVVector getStackTrace( Throwable t ) {
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw, true);

        if( t instanceof IVException ) {
            ((IVException)t).super_printStackTrace(pw);
        } else {
            t.printStackTrace(pw);
        }

        StringTokenizer st = new StringTokenizer(sw.getBuffer().toString(), Util.lineSeparator);
        IVVector v = new IVVector(20);
        while (st.hasMoreTokens()) {
            String s = st.nextToken();
            if( s.startsWith("\tat") )
                v.addElement(s);
        }
        return v;
    }

}
