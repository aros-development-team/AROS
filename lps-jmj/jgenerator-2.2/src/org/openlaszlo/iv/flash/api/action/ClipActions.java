/*
 * $Id: ClipActions.java,v 1.3 2002/05/16 05:08:41 skavish Exp $
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

package org.openlaszlo.iv.flash.api.action;

import java.io.PrintStream;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.context.Context;

/**
 * Collection of clip actions {link ClipAction}
 *
 * @author Dmitry Skavish
 */
public class ClipActions extends FlashItem {

    private int prefix;         // it's always a zero word, but since it's not documented it's safer to keep it
    private int mask;           // mask of handlers
    private IVVector clip_actions;  // vector of ClipAction

    public ClipActions() {}

    public void setPrefix( int prefix ) {
        this.prefix = prefix;
    }

    public int getPrefix() {
        return prefix;
    }

    /**
     * Sets what events this collection handles
     *
     * @param mask   mask of events (see flags of {@link ClipAction})
     */
    public void setMask( int mask ) {
        this.mask = mask;
    }

    /**
     * Gets mask of handled events
     *
     * @return mask of handled events
     */
    public int getMask() {
        return mask;
    }

    /**
     * Adds ClipAction to this collection
     *
     * @param action specified clip actions
     */
    public void addAction( ClipAction action ) {
        if( clip_actions == null ) {
            clip_actions = new IVVector();
        }
        clip_actions.addElement( action );
    }

    /**
     * Returns all clip actions of this collection.
     *
     * @return all clip actions of this collection.
     */
    public IVVector getActions() {
        return clip_actions;
    }

    public static ClipActions parse( Parser p ) {
        ClipActions a = new ClipActions();
        a.clip_actions = new IVVector();
        a.prefix = p.getUWord();
        int version = p.getFile().getVersion();
        a.mask = version>5? p.getUDWord(): p.getUWord();
        for(;;) {
            int flags = version>5? p.getUDWord(): p.getUWord();
            if( flags == 0 ) break;
            int length = p.getUDWord();
            int start = p.getPos();
            int end = start + length;
            Program program = new Program( p.getBuf(), start, end );
            a.clip_actions.addElement( new ClipAction(flags, program) );
            p.setPos( end );
        }
        return a;
    }

    public void write( FlashOutput fob ) {
        fob.writeWord( prefix );
        int version = fob.getFlashFile().getVersion();

        if( version > 5 ) fob.writeDWord(mask);
        else fob.writeWord( mask );

        clip_actions.write( fob );

        if( version > 5 ) fob.writeDWord( 0 );
        else fob.writeWord( 0 );
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"Clip Actions: mask=0x"+Util.w2h(mask)+" prefix="+prefix+" count="+clip_actions.size() );
        clip_actions.printContent( out, indent+"    " );
    }

    public void apply( Context context ) {
        for( int i=0; i<clip_actions.size(); i++ ) {
            ClipAction a = (ClipAction) clip_actions.elementAt(i);
            a.apply( context );
        }
    }

    public boolean isConstant() {
        for( int i=0; i<clip_actions.size(); i++ ) {
            ClipAction a = (ClipAction) clip_actions.elementAt(i);
            if( !a.isConstant() ) return false;
        }
        return true;
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((ClipActions)item).prefix = prefix;
        ((ClipActions)item).mask = mask;
        ((ClipActions)item).clip_actions = clip_actions.getCopy( copier );
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new ClipActions(), copier );
    }

}

