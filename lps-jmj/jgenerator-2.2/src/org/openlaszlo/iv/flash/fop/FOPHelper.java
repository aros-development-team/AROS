/*
 * $Id: FOPHelper.java,v 1.3 2002/02/24 02:10:19 skavish Exp $
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

package org.openlaszlo.iv.flash.fop;

import org.openlaszlo.iv.flash.util.*;

import org.openlaszlo.iv.flash.api.*;

import org.apache.fop.apps.*;
import org.apache.fop.messaging.MessageHandler;
import org.apache.fop.viewer.*;

import org.xml.sax.*;
import org.apache.xerces.parsers.*;

import java.awt.geom.*;
import java.io.*;
import java.net.*;
import java.util.*;

/**
 * FOPHelper
 *
 * @author James Taylor
 */

public class FOPHelper
{
    private InputSource foInput;

    private String linkHandler;

    private Script script;
    private Rectangle2D maxPageBounds;

    /**
     * Changes the XSLFO InputSource, this input source is
     * requierd in order to generate a SWF movie.
     *
     * @param input_source inputsource pointing to the XSLFO file/stream
     */

    public void setXSLFOInputSource( InputSource inputSource )
    {
        this.foInput = inputSource;
    }

    public void setXSLFOInputSource( String s )
    {
        this.foInput = new InputSource( new StringReader( s ) );
    }

    public void setLinkHandler( String s )
    {
        linkHandler = s;
    }

    /**
     * Generates the SWF movie from the passed input parameters using FOP.
     */

    public void render() throws IVException, IOException, FOPException
    {
        SWFRenderer renderer = new SWFRenderer();

        renderer.setLinkHandler( linkHandler );

        Driver driver = new Driver();

        driver.addElementMapping( "org.apache.fop.fo.StandardElementMapping" );
        // driver.addElementMapping( "org.apache.fop.svg.SVGElementMapping" );

        driver.setRenderer( renderer );
        driver.setInputSource( this.foInput );

        driver.run();

        // Store results

        this.script = renderer.getScriptBuilder().getScript();

        script.getFrameAt( 0 ).addStopAction();

        this.maxPageBounds = renderer.getScriptBuilder().getMaxPageBounds();
    }

    public Script getScript()
    {
        return script;
    }

    public Rectangle2D getMaxPageBounds()
    {
        return maxPageBounds;
    }
}
