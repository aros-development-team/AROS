/*
 * $Id: SoundInfo.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
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

package org.openlaszlo.iv.flash.api.sound;

import java.io.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;

public final class SoundInfo extends FlashItem {

    public static final int SYNC_NO_MULTIPLE = 0x10;
    public static final int SYNC_STOP        = 0x20;
    public static final int HAS_ENVELOPE     = 0x08;
    public static final int HAS_LOOPS        = 0x04;
    public static final int HAS_OUT_POINT    = 0x02;
    public static final int HAS_IN_POINT     = 0x01;

    private int flags;
    private int inPoint;
    private int outPoint;
    private int loopCount;
    private IVVector envelopes;

    public SoundInfo() {}

    public static SoundInfo parse( Parser p ) {
        SoundInfo s = new SoundInfo();
        int flags = s.flags = p.getUByte();
        if( (flags&HAS_IN_POINT)!=0 ) {
            s.inPoint = p.getUDWord();
        }
        if( (flags&HAS_OUT_POINT)!=0 ) {
            s.outPoint = p.getUDWord();
        }
        if( (flags&HAS_LOOPS)!=0 ) {
            s.loopCount = p.getUWord();
        }
        if( (flags&HAS_ENVELOPE)!=0 ) {
            int nPoints = p.getUByte();
            s.envelopes = new IVVector(nPoints);
            for( int i=0; i<nPoints; i++ ) {
                s.envelopes.addElement( SoundEnvelope.parse(p) );
            }
        }
        return s;
    }

    public static SoundInfo newSoundInfo( int flags ) {

        SoundInfo soundInfo = new SoundInfo();

        soundInfo.flags = flags;

        soundInfo.envelopes = new IVVector( 0 );

        return soundInfo;

    }

    public int length() {

        int length = 1; // Flags

        if ( ( flags & HAS_IN_POINT) != 0 ) { length += 4; }

        if ( ( flags & HAS_OUT_POINT ) != 0 ) { length += 4; }

        if ( ( flags & HAS_LOOPS ) !=0 ) { length += 2; }

        if ( ( flags & HAS_ENVELOPE ) != 0 ) { length += ( 1 + ( 8 * envelopes.size() ) ); }

        return length;

    }

    public void write( FlashOutput fob ) {
        fob.writeByte(flags);
        if( (flags&HAS_IN_POINT)!=0 ) {
            fob.writeDWord(inPoint);
        }
        if( (flags&HAS_OUT_POINT)!=0 ) {
            fob.writeDWord(outPoint);
        }
        if( (flags&HAS_LOOPS)!=0 ) {
            fob.writeWord(loopCount);
        }
        if( (flags&HAS_ENVELOPE)!=0 ) {
            int nPoints = envelopes.size();
            fob.writeByte(nPoints);
            envelopes.write(fob);
        }
    }

    public void printContent( PrintStream out, String indent ) {
        out.println( indent+"SoundInfo:" );
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
        super.copyInto( item, copier );
        ((SoundInfo)item).flags = flags;
        ((SoundInfo)item).inPoint = inPoint;
        ((SoundInfo)item).outPoint = outPoint;
        ((SoundInfo)item).loopCount = loopCount;
        ((SoundInfo)item).envelopes = envelopes!=null? envelopes.getCopy(copier): null;
        return item;
    }

    public FlashItem getCopy( ScriptCopier copier ) {
        return copyInto( new SoundInfo(), copier );
    }

    /**
     * Sound Envelope
     */
    public static class SoundEnvelope extends FlashItem {
        public int mark44;
        public int level0;
        public int level1;
        public SoundEnvelope() {}

        public static SoundEnvelope parse( Parser p ) {
            SoundEnvelope s = new SoundEnvelope();
            s.mark44 = p.getUDWord();
            s.level0 = p.getUWord();
            s.level1 = p.getUWord();
            return s;
        }
        public void write( FlashOutput fob ) {
            fob.writeDWord(mark44);
            fob.writeWord(level0);
            fob.writeWord(level1);
        }
        public void printContent( PrintStream out, String indent ) {}

        protected FlashItem copyInto( FlashItem item, ScriptCopier copier ) {
            ((SoundEnvelope)item).mark44 = mark44;
            ((SoundEnvelope)item).level0 = level0;
            ((SoundEnvelope)item).level1 = level1;
            return item;
        }
        public FlashItem getCopy( ScriptCopier copier ) {
            return copyInto( new SoundEnvelope(), copier );
        }
    }

}

