/*
 * $Id: GenericXMLCommand.java,v 1.3 2002/02/24 02:10:19 skavish Exp $
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
import java.util.*;

/**
 * Base class for most XML commands
 */

public class GenericXMLCommand extends GenericCommand {

    protected String datasource;
    protected String select;

    /**
     * Init common parameters of xml based commands
     */
    protected void initParms( Context context ) throws IVException {
        datasource   = getParameter( context, "datasource" );
        select       = getParameter( context, "select" );
    }

    /**
     * Retrieve nearest parent GraphContext from contexts' stack
     *
     * @param context context to start searching from
     * @return found GraphContext or null
     */

    protected GraphContext retrieveGraphContext( Context context )
    {
        while ( context != null && ! ( context instanceof GraphContext ) )
        {
            context = context.getParent();
        }

        return ( GraphContext ) context;
    }

    /**
     * Return GraphContext for this command, either by creating one from datasource parameter
     * or returning nearest parent GraphContext or null
     *
     *
     *
     * @param file current FlashFile
     * @param context current context for this command
     * @return GraphContext for the command or null
     */

    protected GraphContext getGraphContext( FlashFile file, Context context ) throws IVException
    {
        if ( datasource != null )
        {
            try
            {
                return ( GraphContext )
                    ContextFactory.createContext( context, datasource, file, false );
            }
            catch( Exception e )
            {
                throw new IVException(Resource.ERRDATAREAD, new Object[] {datasource, getCommandName()}, e);
            }
        }
        else
        {
            return retrieveGraphContext( context );
        }
    }

    /**
     * Evaluate given Path in given Context and return the result as a String
     *
     * @param context Context to evaluate Path in
     * @param path String which will be executed
     * @param def default value returned in case of any errors
     * @return result as a string
     */

    protected String evalStringParameter ( Context context, String path, String def )
    {
        String rval = context.getValue ( path );

        if ( rval == null )
        {
            return def;
        }
        else
        {
            return rval;
        }
    }

    /**
     * Evaluate given Path in given Context and return the result as a boolean
     *
     * @param context Context to evaluate Path in
     * @param path Path which will be executed
     * @param def default value returned in case of any errors
     * @return result as boolean
     */

    protected boolean evalBoolParameter ( Context context, String path, boolean def )
    {
        return Util.toBool( context.getValue( path ), def );
    }

    /**
     * Evaluate given Path in given Context and return the result as a int
     *
     * @param context Context to evaluate Path in
     * @param path Path which will be executed
     * @param def default value returned in case of any errors
     * @return result as a int
     */

    protected int evalIntParameter( Context context, String path, int def )
    {
        return Util.toInt( context.getValue( path ), def );
    }

    /**
     * Evaluate given Path in given Context and return the result as a double
     *
     * @param context Context to evaluate Path in
     * @param path Path which will be executed
     * @param def default value returned in case of any errors
     * @return result as a double
     */
    protected double evalDoubleParameter( Context context,  String path, double def )
    {
        return Util.toDouble( context.getValue( path ), def );
    }
}

