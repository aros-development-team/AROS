/*
 * $Id: Timeline.java,v 1.3 2002/08/08 23:26:54 skavish Exp $
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

import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.context.Context;
import java.io.*;
import java.util.*;
import java.awt.geom.*;

/**
 * Flash timeline
 * <P>
 * A timeline contains zero or more {@link Frame frames}
 *
 * @author Dmitry Skavish
 * @see Frame
 * @see Script
 */
public final class Timeline extends IVVector {

    /**
     * Creates empty timeline
     */
    public Timeline() {}

    /**
     * Creates empty timeline of specified capacity
     */
    public Timeline( int capacity ) {
        super( capacity );
    }

    /**
     * Creates timeline from specified vector
     * <P>
     * Copies all the frames from specified vector
     *
     * @param data   vector of Frames
     */
    public Timeline( IVVector data ) {
        super( data );
    }

    /**
     * Creates new empty frame, adds it to the end of this timeline and returns it
     *
     * @return new frame
     */
    public Frame newFrame() {
        Frame f = new Frame();
        addFrame(f);
        return f;
    }

    /**
     * Adds specified frame to the end of this timeline
     *
     * @param o      frame to be added
     */
    public void addFrame( Frame o ) {
        addElement( o );
    }

    /**
     * Returns index of specified frame in the timeline or -1
     *
     * @param frame  specified frame
     * @return index of specified frame in the timeline or -1
     */
    public int getFrameIndex( Frame frame ) {
        return find(frame);
    }

    /**
     * Removes frame from this timeline at specified index
     *
     * @param index  specified index
     * @return removed frame
     */
    public Frame removeFrameAt( int index ) {
        return (Frame) removeElementAt( index );
    }

    /**
     * Removes specified frame from this timeline
     *
     * @param o      frame to be removed
     */
    public void removeFrame( Frame o ) {
        removeElement( o );
    }

    /**
     * Returns frame from this timeline at specified index
     *
     * @param index  specified index
     * @return frame at specified index
     */
    public Frame getFrameAt( int index ) {
        return (Frame) elementAt( index );
    }

    /**
     * Replaces frame at specified index with specified one
     *
     * @param o      specified frame
     * @param index  specified index
     */
    public void setFrameAt( Frame o, int index ) {
        setElementAt( o , index );
    }

    /**
     * Inserts specified number of empty frames
     * <P>
     * Beginning from frame number 'from' inserts 'num' empty frame in the timeline.
     *
     * @param from   start inserting frames beginning from this one
     * @param num    number of empty frames to insert
     */
    public void insertFrames( int from, int num ) {
        insertObjects( from, num );
        for( int i=from; i<from+num; i++ ) {
            setFrameAt( new Frame(), i );
        }
    }

    /**
     * Inserts one empty frame at specified index
     *
     * @param index  specified index
     * @return new inserted frame
     */
    public Frame insertFrame( int index ) {
        insertFrames( index, 1 );
        return getFrameAt(index);
    }

    /**
     * Returns number of frames in the timeline
     *
     * @return number of frames
     */
    public int getFrameCount() {
        return size();
    }

    /**
     * Writes this timeline to flash buffer
     * <P>
     * Has to be used only when writing NON main timelines
     *
     * @param fob    flash buffer to write to
     */
    public void write( FlashOutput fob ) {
        for( int i=0; i<top; i++ ) {
            Frame fo = (Frame) objects[i];
            fo.write(fob);
        }
    }


    /**
     * Writes this timeline to flash buffer
     * <P>
     * Has to be used only when writing main timeline
     *
     * @param fob    flash buffer to write to
     * @param dc     dependencies collector used when writing
     */
    public void generate( FlashOutput fob, DepsCollector dc ) {
        generate(fob, dc, 0);
    }

    /**
     * Writes this timeline to flash buffer
     * <P>
     * Has to be used only when writing main timeline
     *
     * @param fob    flash buffer to write to
     * @param dc     dependencies collector used when writing
     * @param off    frame offset to start from
     */
    public void generate( FlashOutput fob, DepsCollector dc, int off ) {
        for( int i=off; i<top; i++ ) {
            Frame fo = (Frame) objects[i];
            fo.generate( fob, dc );
        }
    }


    public void collectDeps( DepsCollector dc ) {
        for( int i=0; i<top; i++ ) {
            Frame fo = (Frame) objects[i];
            fo.collectDeps(dc);
        }
    }

    public void apply( Context context ) {
        for( int i=0; i<top; i++ ) {
            Frame fo = (Frame) objects[i];
            fo.apply(context);
        }
    }

    public void process( FlashFile file, Context context ) throws IVException {
        for( int i=0; i<top; i++ ) {
            Frame fo = (Frame) objects[i];
            fo.process(file, context);
        }
    }

    public void doCommand( FlashFile file, Context context, Script parent ) throws IVException {
        for( int i=0; i<top; i++ ) {
            Frame fo = (Frame) objects[i];
            fo.doCommand(file, context, parent, i);
        }
    }

    public void addBounds( Rectangle2D rect ) {
        for( int i=0; i<top; i++ ) {
            Frame fo = (Frame) objects[i];
            fo.addBounds( rect );
        }
    }

    public boolean isConstant() {
        for( int i=0; i<top; i++ ) {
            Frame fo = (Frame) objects[i];
            if( !fo.isConstant() ) return false;
        }
        return true;
    }

    public void printContent( PrintStream out, String indent ) {
        for( int i=0; i<top; i++ ) {
            Frame fo = (Frame) objects[i];
            out.print( indent+"Frame #"+i );
            if( fo.getName() != null ) {
                out.println( " name='"+fo.getName()+"' "+(fo.isAnchor()?"anchor":"") );
            } else {
                out.println();
            }
            fo.printContent( out, indent );
        }
    }

    public IVVector getCopy( ScriptCopier copier ) {
        Timeline t = new Timeline( size() );
        for( int i=0; i<top; i++ ) {
            Frame fo = (Frame) objects[i];
            t.setElementAt( fo.getCopy(copier), i );
        }
        return t;
    }
}

