/*
 * $Id: XMLReplicateMovieClipCommand.java,v 1.7 2002/06/12 23:54:14 skavish Exp $
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
import org.openlaszlo.iv.flash.context.Context;
import org.openlaszlo.iv.flash.context.GraphContext;
import java.io.*;
import java.util.*;

public class XMLReplicateMovieClipCommand extends GenericXMLCommand {

    public XMLReplicateMovieClipCommand() {}

    public void doCommand( FlashFile file, Context context, Script parent, int frameNum ) throws IVException
    {
        super.initParms( context );

        // parameters

        boolean expand = getBoolParameter( context, "expand", true );

        int order;
        String sorder = getParameter( context, "order", "none" );

        if ( sorder.equalsIgnoreCase( "descending" ) ) order = -1;
        else if ( sorder.equalsIgnoreCase( "ascending" ) ) order = 1;
        else order = 0;

        String sortby = getParameter( context, "sortby", "'none'" );

        // read data and create context

        GraphContext gc = getGraphContext( file, context );

        // select nodes

        List contextList = gc.getValueList( select );

        // sort items

        if ( order != 0 )
        {
            contextList = GraphContext.sortValueList( contextList, sortby, order == 1 );
        }

        Instance inst = getCommandInstance(file, context, parent, frameNum);
        Script template = inst.getScript();

        Script newScript = new Script(20);

        // process datasource

        Context myContext;
        ListIterator iter = contextList.listIterator();

        int totalFrames = 0;
        while( iter.hasNext() )
        {
            myContext = ( Context ) iter.next();

            Script myScript = template.copyScript();
            file.processScript( myScript, myContext );

            totalFrames += myScript.getFrameCount();

            // shift all the layers up, this fix some flash player bug
            myScript.reserveLayers(1, newScript.getMaxDepth());

            newScript.appendScript( myScript );
        }

        inst.setScript( newScript );

        if( expand && !isComponent() ) {
            // create new frames if needed
            parent.getFrameAt(frameNum+totalFrames-1);
        }
    }

}
