/*
 * $Id: SoundStreamHead.java,v 1.4 2002/07/18 06:02:22 skavish Exp $
 *
 * ===========================================================================
 *
 */

package org.openlaszlo.iv.flash.api.sound;

import java.io.PrintStream;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;

public class SoundStreamHead extends FlashObject
{
    public int reserved;                                   // 4 bits ( reserved, always 0 )
    public int playbackRate;           // 2 bits
    public boolean isPlayback16bit;    // 1 bit
    public boolean isPlaybackStereo;   // 1 bit

    public int streamCompression;      // 4 bits
    public int streamRate;             // 2 bits
    public boolean isStream16bit;      // 1 bit
    public boolean isStreamStereo;     // 1 bit

    public int streamSampleCount;      // 1 word

                                       // Total: 4 bytes
    private int tagCode = -1;

    public int getTag()
    {
        if( tagCode != -1 ) return tagCode;
        if ( streamCompression == Sound.COMPRESS_ADPCM
             && streamRate     == Sound.RATE_5_5
             && isStream16bit  == true )
        {
            return Tag.SOUNDSTREAMHEAD;
        }
        else
        {
            return Tag.SOUNDSTREAMHEAD2;
        }
    }

    public static SoundStreamHead parse( Parser p )
    {
        SoundStreamHead o = new SoundStreamHead();

        o.tagCode = p.getTagCode();
        p.initBits();

        //p.skipBits( 4 ); // Reserved
        o.reserved = p.getBits(4);

        o.playbackRate = p.getBits( 2 );
        o.isPlayback16bit = p.getBool();
        o.isPlaybackStereo = p.getBool();

        o.streamCompression = p.getBits( 4 );

        o.streamRate = p.getBits( 2 );
        o.isStream16bit = p.getBool();
        o.isStreamStereo = p.getBool();

        o.streamSampleCount = p.getUWord();

        return o;
    }

    public void write( FlashOutput fob )
    {
        fob.writeTag( getTag(), 4 );

        fob.initBits(); // Prepare to write to bit buffer

        //fob.writeBits( 0, 4 ); // Reserved
        fob.writeBits(reserved, 4);

        fob.writeBits( playbackRate, 2 );
        fob.writeBool( isPlayback16bit );
        fob.writeBool( isPlaybackStereo );

        fob.flushBits(); // End of first byte

        fob.writeBits( streamCompression, 4 );
        fob.writeBits( streamRate, 2 );
        fob.writeBool( isStream16bit );
        fob.writeBool( isStreamStereo );

        fob.flushBits(); // End of second byte

        fob.writeWord( streamSampleCount );
    }

    public boolean isConstant()
    {
        return true;
    }

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier )
    {
        super.copyInto( item, copier );

        ( (SoundStreamHead) item ).tagCode      = tagCode;
        ( (SoundStreamHead) item ).playbackRate = playbackRate;
        ( (SoundStreamHead) item ).isPlayback16bit = isPlayback16bit;
        ( (SoundStreamHead) item ).isPlaybackStereo = isPlaybackStereo;

        ( (SoundStreamHead) item ).streamCompression = streamCompression;
        ( (SoundStreamHead) item ).streamRate = streamRate;
        ( (SoundStreamHead) item ).isStream16bit = isStream16bit;
        ( (SoundStreamHead) item ).isStreamStereo = isStreamStereo;

        ( (SoundStreamHead) item ).streamSampleCount = streamSampleCount;

        return item;
    }
    public FlashItem getCopy( ScriptCopier copier )
    {
        return copyInto( new SoundStreamHead(), copier );
    }

    public void printContent( PrintStream out, String indent )
    {
        if ( getTag() == Tag.SOUNDSTREAMHEAD2 )
        {
            out.println( indent + "SoundStreamHead2" );
        }
        else
        {
            out.println( indent + "SoundStreamHead" );
        }

        out.println( indent + "    Playback Rate: " + Sound.rates[ playbackRate ] );
        out.println( indent + "    Playback 16 Bit: " + isPlayback16bit );
        out.println( indent + "    Playback Stereo: " + isPlaybackStereo );

        out.println( indent + "    Stream Compression: " + Sound.compressions [ streamCompression ] );
        out.println( indent + "    Stream Rate: " + Sound.rates[ streamRate ] );
        out.println( indent + "    Stream 16 Bit: " + isStream16bit );
        out.println( indent + "    Stream Stereo: " + isStreamStereo );

        out.println( indent + "    Stream Sample Count: " + streamSampleCount );
    }
}
