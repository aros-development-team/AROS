/*
 * $Id: Context.java,v 1.6 2002/07/03 23:03:44 skavish Exp $
 *
 * ==========================================================================
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

package org.openlaszlo.iv.flash.context;

import java.io.*;
import java.util.*;
import java.lang.reflect.*;
import org.openlaszlo.iv.flash.util.*;

/**
 * Abstract generator context<BR>
 * Contains variables with theirs values and reference to parent context<BR>
 *
 * @author Dmitry Skavish
 * @author James Taylor
 */

public abstract class Context {

    private Context parent;

    /**
     * Sets parent context
     *
     * @param parent parent context
     */
    public void setParent( Context parent ) {
        this.parent = parent;
    }

    /**
     * Returns parent context
     *
     * @return parent context
     */
    public Context getParent() {
        return parent;
    }

    /**
     * Retrieves value by name
     *
     * @param name expression to evaluate in this context
     * @return value of variable or null
     */
    public abstract String getValue( String name );

    /**
     * Queries the parent context for the value
     *
     * @param expr expression to evaluate in parent context
     * @return the value of evaluating the expression in the parent context,
     *         or null if there is no parent context.
     */
    protected String getValueFromParent( String expr ) {
        if ( parent != null ) {
            return parent.getValue( expr );
        } else {
            return null;
        }
    }

    /**
     * Traverse the tree of context and find the root - CommandContext
     *
     * @return found CommandContext or null
     */
    public CommandContext getCommandContext() {
        if( this instanceof CommandContext ) return (CommandContext) this;
        if( parent != null ) return parent.getCommandContext();
        return null;
    }

    /**
     * Applies this context to the specified string<BR>
     * In the specified string replaces all constructions like: {variable_name}
     * with corresponding variable's value or empty string if variable not found<BR>
     * {{ treated as a single brace
     *
     * @param s      string to replace in
     * @return string with all variables replaced by their values
     */
    public String apply( String s ) {
        if( s == null ) return s;
        int start = s.indexOf('{');
        if( start == -1 ) return s;

        //System.out.println( "length="+s.length()+", apply='"+s+"'" );
        int pos = 0;
        int from = 0;
        int slen = s.length();
        int extra = 10;
        char[] sa = new char[slen + extra];

        for(;;) {
            int start1 = start+1;
            if( start1 < slen && s.charAt(start1) == '{' ) {
                // case: {{
                s.getChars(from, start1, sa, pos);
                pos += start1-from;
                from = start1+1;
            } else {
                int cnt = 1;
                int end = start1;
                while( end<slen && cnt>0 ) {
                    char ch = s.charAt(end++);
                    if( ch == '}' ) cnt--;
                    else if( ch == '{' ) cnt++;
                }
                end--;
                if( end >= slen ) {
                    s.getChars(from, start, sa, pos);
                    pos += start-from;
                    break;
                }
                s.getChars(from, end, sa, pos);
                pos += start-from;

                // get variable name
                String varName = new String( sa, pos+1, end-start1 );
                String value = null;

                int varlen = varName.length();
                if( varlen>0 ) {
                    char ch = varName.charAt(0);
                    if( ch == '$' ) {   // check for command
                        String cmdCall = apply(varName).trim();
                        CommandContext cmdexec = getCommandContext();
                        if( cmdexec == null ) {
                            value = getValue(cmdCall);
                        } else {
                            value = cmdexec.executeCommand(this, cmdCall);
                        }
                    } else if( ch == '#' && varlen > 1 ) { // check for inline javascript, convinient way to use js
                        String js_text = varName.substring(1);
                        value = Util.executeJSString(this, js_text, null);
                    } else {
                        String varName1 = apply(varName).trim();
                        value = getValue(varName1);
                    }
                }

                if( value != null ) {
                    int vlen = value.length();
                    int vnlen = varName.length() + 2; // including braces

                    //  bug fixed - by yoonforh 2002-05-28 13:39:34
                    if( vlen > vnlen ) {
                        extra -= (vlen - vnlen);
                        if( extra < 0 ) {
                            extra = -extra;
                            char[] t = new char[sa.length + extra * 2];
                            System.arraycopy(sa, 0, t, 0, sa.length);
                            sa = t;
                        }
                    }
                    value.getChars(0, vlen, sa, pos);
                    pos += vlen;
                }
                from = end+1;
            }
            start = s.indexOf('{', from);
            if( start == -1 ) {
                s.getChars(from, slen, sa, pos);
                pos += slen-from;
                break;
            }
        }

        return new String(sa, 0, pos);
    }
}
