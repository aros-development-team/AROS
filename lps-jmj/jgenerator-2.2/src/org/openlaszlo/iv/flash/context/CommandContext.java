/*
 * $Id: CommandContext.java,v 1.3 2002/04/04 06:38:47 skavish Exp $
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

import org.openlaszlo.iv.flash.util.CommandExecutor;
import java.util.*;

/**
 * Context which can execute commands as well as substitute variables.
 *
 * @author Dmitry Skavish
 * @see org.openlaszlo.iv.flash.util.CommandExecutor
 */
public class CommandContext extends StandardContext {

    private CommandExecutor executor;

    /**
     * Creates command context without parent.
     *
     * @param executor command executor used to execute commands
     */
    public CommandContext( CommandExecutor executor ) {
        this( null, executor );
    }

    /**
     * Creates command context with parent.
     *
     * @param parent   parent context
     * @param executor command executor used to execute commands
     */
    public CommandContext( Context parent, CommandExecutor executor ) {
        setParent( parent );
        this.executor = executor;
    }

    public String getValue( String cmd ) {
        if( cmd.length() != 0 && cmd.charAt(0) == '$' ) {
            String res = executeCommand(this, cmd);
            if( res != null ) return res;
        }

        return super.getValue( cmd );
    }

    /**
     * Executes specified command.<p>
     * Supported the following syntax:<BR>
     *  <CODE>$command_name[([param1[,param2...]])]</CODE><br>
     * parameters can be commands as well, for example:<br>
     * <code>$command1($command2(p1,p2),p3)</code>
     *
     * @param context execution context (the context from which the command was invoked)
     * @param cmd    command with possible parameters
     * @return result of command execution, never null
     */
    public String executeCommand( Context context, String cmd ) {
        //System.out.println( "execute command: '"+cmd+"'" );
        int idx = cmd.indexOf('(');
        String name;
        Vector parms = null;
        if( idx == -1 ) {
            name = cmd.substring(1).trim();
        } else {
            name = cmd.substring(1,idx).trim();
            int i = idx+1;
            parms = new Vector();
            int l = cmd.length();
            int cnt = 0;
            int start = i;
            while( i<l && cnt>=0 ) {
                char ch = cmd.charAt(i);
                switch( ch ) {
                    case '(':
                        cnt++;
                        break;
                    case ')':
                        cnt--;
                        break;
                    case ',':
                        if( cnt == 0 ) {
                            String s = cmd.substring(start,i).trim();
                            parms.addElement(s);
                            start = i+1;
                        }
                        break;
                }
                i++;
            }
            i--;
            String s = cmd.substring(start,i).trim();
            if( parms.size() != 0 || s.length() != 0 ) {
                parms.addElement(s);
            } else {
                parms = null;
            }
        }
        if( parms != null ) {
            for( int i=0; i<parms.size(); i++ ) {
                String c = (String) parms.elementAt(i);
                if( c.length() != 0 && c.charAt(0) == '$' ) {
                    String res = executeCommand(context, c);
                    if( res == null ) res = "";
                    parms.setElementAt(res,i);
                }
            }
        }
        return executor.execute(context, name, parms);
    }

}


