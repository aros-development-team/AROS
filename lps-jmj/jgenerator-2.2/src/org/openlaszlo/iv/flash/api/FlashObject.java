/*
 * $Id: FlashObject.java,v 1.3 2002/07/16 20:17:29 skavish Exp $
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

package org.openlaszlo.iv.flash.api;

import java.io.PrintStream;

import java.awt.geom.Rectangle2D;

import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.context.Context;

/**
 * Abstract base class for flash objects which can be placed on Flash timeline (usually tags) and generated back
 *
 * @author Dmitry Skavish
 * @see FlashDef
 * @see FlashItem
 */
public abstract class FlashObject extends FlashItem {

    /**
     * Defines whether this object is constant for generator processing or not<BR>
     * Possible values are:<BR>
     * <UL>
     * <LI>0 - it is not known yet whether this object is constant or not
     * <LI>1 - this object is constant
     * <LI>2 - this object is not a constant
     *
     * @see #isConstant
     */
    private byte isConstant = (byte)0;

    /**
     * Returns tag id of this object
     *
     * @return tag id of this object
     * @see org.openlaszlo.iv.flash.util.Tag
     */
    public abstract int getTag();

    /**
     * Collects all dependencies this objects has in given {@link DepsCollector}<br>
     * For example object of {@link org.openlaszlo.iv.flash.api.button.Button Button} class depends on objects of
     * {@link FlashDef} class, i.e. consists of flash definitions.
     * It means that all objects this object depends on have to be generated
     * <B>before</B> this object is generated.
     *
     * @param dc     collector of dependent objects
     */
    public void collectDeps( DepsCollector dc ) {}

    /**
     * Collects all possible fonts this object uses (if any)
     *
     * @param fc     fonts collector
     */
    public void collectFonts( FontsCollector fc ) {}

    /**
     * Generates this object to FlashOutput, but first generates all dependencies<br>
     * Usually you don't need to override this method<BR>
     * Method:<BR>
     * <UL>
     * <LI>collects all dependencies of this object
     * <LI>writes these dependencies first to FlashOutput
     * <LI>writes this object itself
     * </UL>
     *
     * @param fob    buffer to write to
     * @param dc     dependencies collector
     * @see #write
     * @see #collectDeps
     */
    public void generate( FlashOutput fob, DepsCollector dc ) {
        //Object id = this instanceof FlashDef?(Object)new Integer(((FlashDef)this).getID()):(Object)this;
        //System.out.println( "FlashObject.generate: ID="+id );
        dc.startCollect();
        collectDeps( dc );
        IVVector collected = dc.getCollected();
        //System.out.println( "generate collected:" );
        for( int i=0; i<collected.size(); i++ ) {
            FlashObject d = (FlashObject) collected.elementAt(i);
            if( !dc.isGenerated(d) ) {
                dc.addGenerated(d);
                d.generate( fob, dc );
            }
        }
        dc.endCollect();
        //System.out.println( "FlashObject.write: ID="+id );
        write( fob );
    }

    /**
     * Processes this flash object in specified context and flash file
     *
     * @param file    flash file
     * @param context generator context
     * @exception IVException
     */
    public void process( FlashFile file, Context context ) throws IVException {}

    /**
     * Returns true of this object was processed
     *
     * @return true of this object was processed
     */
    public boolean isProcessed() {
        return true;
    }

    /**
     * Sets this object as processed
     */
    public void setProcessed() {}

    /**
     * Applies given context to this object<BR>
     * If this object is not constant ({@link #isConstant})
     * then it may be altered by generator context if it has generator variables
     * somewhere inside.<BR>
     *
     * @param context generator context to be applied to this object
     */
    public void apply( Context context ) {}

    /**
     * Returns true if this object is not subject to generator substitutions or commands<BR>
     * Override this method if you know the dynamic nature of content of your object and want
     * to avoid unneccesary copying or/and execution.<BR>
     * Constant objects are not duplicated and are not processed by generator.<BR>
     *
     * @return true if object is constant
     * @see #isConstant
     */
    protected boolean _isConstant() {
        return false;
    }

    protected void setConstant( boolean is_const ) {
        isConstant = is_const? (byte)1: (byte)2;
    }

    /**
     * Returns true if this object is not subject to generator substitutions or commands<BR>
     * You usually don't need to override this method, this method is merely a cache for
     * {@link #_isConstant} method.
     *
     * @return true if object is constant
     * @see #isConstant
     */
    public boolean isConstant() {
        if( isConstant == (byte)0 ) {
            isConstant = _isConstant()? (byte)1: (byte)2;
        }
        return isConstant == (byte)1;
    }

    /**
     * Returns rectangular bounds of this object<BR>
     * It's ok to return null if this object does not have any bounds,
     * as for example {@link org.openlaszlo.iv.flash.api.action.DoAction}
     *
     * @return bounds of this object or null
     */
    public Rectangle2D getBounds() {
        return null;
    }

    public void printContent( PrintStream out, String indent ) {
        int tag = getTag();
        String name;
        if( tag >= Tag.tagNames.length || tag < 0 ) name = "Unknown";
        else name = Tag.tagNames[tag];
        out.println( indent+"Tag: 0x"+Util.b2h(getTag())+" '"+name+"'" );
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((FlashObject)item).isConstant = isConstant;
        return item;
    }

}

