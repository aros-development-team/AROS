/*
 * $Id: SetEnvironmentCommand.java,v 1.4 2002/05/29 03:54:49 skavish Exp $
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

package org.openlaszlo.iv.flash.commands;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.util.*;

import org.openlaszlo.iv.flash.context.*;
import java.io.*;

/**
 * Set environment in the following format:<BR>
 * NAME,VALUE<BR>
 * name1, value1<BR>
 * name2, value2<BR>
 * name3, value3<BR>
 * name4, value4<BR>
 */
public class SetEnvironmentCommand extends GenericCommand {

    public SetEnvironmentCommand() {
    }

    public void doCommand( FlashFile file, Context context, Script parent, int frame ) throws IVException {
        String datasource = getParameter( context, "datasource" );

        if( datasource != null ) {
            try {
                if( context instanceof FakeContext ) {

                    Context myContext = ContextFactory.createContext( context.getParent(), datasource, file, false );

                    ((FakeContext)context).setContext( myContext );

                } else {
                    // normally we should not come here, because there may be only one setEnvironment command
                    // on a timeline, and only this command sets new context to FakeContext
                    // so if a context to be read is not StandardContext or if parent context is not StandardContext
                    // then we throw an exception
                    if( !(context instanceof StandardContext) ) {
                        throw new IVException( Resource.EXPECTSTDCONTEXT );
                    }

                    // this will throw an exception if the context to be read is not standard
                    String[][] data = ContextFactory.readStandardContext( datasource, file );
                    ContextFactory.setStandardContextData( (StandardContext) context, data );
                }
            } catch( IOException e ) {
                throw new IVException(Resource.ERRDATAREAD, new Object[] {datasource, getCommandName()}, e);
            } catch( Exception e ) {
                throw new IVException(Resource.INVALDATASOURCE, new Object[] {datasource, getCommandName()}, e);
            }
        }
    }

    public boolean isGlobal() {
        return true;
    }
}
