/*
 * $Id: DepsCollector.java,v 1.3 2002/07/03 23:03:44 skavish Exp $
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
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.text.*;

/**
 * This class serves as storage for collecting an flash object's dependencies.<P>
 * Some flash objects depends on others. For example Button depends on
 * shapes and/or sounds. PlaceObject depends on corresponding
 * flash definition etc. According to flash file format all dependencies
 * have to be placed in the file before the object itself.
 * This class helps in accomplishing this task.
 *
 * @author Dmitry Skavish
 * @see FontsCollector
 */
public final class DepsCollector {

    protected Hashtable allGenerated = new Hashtable();
    protected IVVector collected = new IVVector(20);
    //protected IVVector[] collected = new IVVector[20];
    protected int current = -1;
    protected IVVector fonts;

    /**
     * Creates collector from {@link FontsCollector}.
     *
     * @param fc     fonts collector
     */
    public DepsCollector( FontsCollector fc ) {
        fonts = fc.getFonts();
        //for( int i=0; i<collected.length; i++ ) collected[i] = new IVVector();
    }

    /**
     * Collects flash item
     *
     * @param item   item to collect
     */
    public void addDep( FlashItem item ) {
        getLevel(current).addElement(item);
    }

    /**
     * Returns true if specified item has been generated
     *
     * @param item   item to be checked as generated
     * @return true if specified item has been generated
     */
    public boolean isGenerated( FlashItem item ) {
        return allGenerated.containsKey(item);
    }

    /**
     * Adds specified item to the list of generated item
     *
     * @param item   item to be added as generated
     */
    public void addGenerated( FlashItem item ) {
        allGenerated.put(item,item);
    }

    /**
     * Collects specified font.<p>
     * Check for given {@link Font} if there is corresponding
     * {@link FontDef} already collected, if not then collect this font as well.
     *
     * @param font   font to collect
     */
    public void addDep( Font font ) {
        for( int i=0; i<fonts.size(); i++ ) {
            FontDef fdef = (FontDef) fonts.elementAt(i);
            if( fdef.getFont() == font ) {
                addDep( fdef );
                return;
            }
        }
    }

    /**
     * Starts next level of collection.
     */
    public void startCollect() {
        getLevel(++current).reset();
        //for( int i=0; i<current; i++ ) System.out.print( "  " );
        //System.out.println( "start collect: current="+current );
    }

    /**
     * Stops current collection level.
     */
    public void endCollect() {
        //for( int i=0; i<current; i++ ) System.out.print( "  " );
        --current;
        //System.out.println( "end collect: current="+current );
    }

    /**
     * Returns collected objects.
     *
     * @return vector of all objects collected on current level
     */
    public IVVector getCollected() {
        return getLevel(current);
    }

    private IVVector getLevel( int num ) {
        IVVector level;
        if( num >= collected.size() ) {
            level = new IVVector();
            collected.setElementAt(level, num);
        } else {
            level = (IVVector) collected.elementAt(num);
            if( level == null ) {
                level = new IVVector();
                collected.setElementAt(level, num);
            }
        }
        return level;
    }

}
