/*
 * $Id: MP3Sound.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
 *
 * ===========================================================================
 *
 */

package org.openlaszlo.iv.flash.api.sound;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.url.*;
import org.openlaszlo.iv.flash.api.*;

import java.io.*;

/**
 * A sound whoose SOUNDDATA is in mp3 format
 *
 * @author James Taylor
 *
 */

public class MP3Sound extends Sound
{
    public int delaySeek;
    public DataMarker data;

    /** Create MP3Sound from url */

    public static MP3Sound newMP3Sound( IVUrl url ) throws IVException, IOException
    {
        return newMP3Sound( Util.readUrl( url ) );
    }

    /** Create MP3Sound from buffer */

    public static MP3Sound newMP3Sound( FlashBuffer fob ) throws IVException, IOException
    {
        byte [] buffer;

        // MP3Helper will load the MP3 from the url -- FIXME: need to consider the media cache here

        MP3Helper mp3 = new MP3Helper( fob );

        // Read the mp3 frames and put the into an array in memory. Must read
        // before extracting properties.

        FlashBuffer fb = new FlashBuffer( fob.getSize() );

        while ( ( buffer = mp3.nextFrame() ) != null )
        {
            fb.writeArray( buffer, 0, buffer.length );
        }

        MP3Sound sound = new MP3Sound();

        sound.compressFormat = COMPRESS_MP3;

        // Set the rate from the MP3's frequency

        switch ( mp3.getFrequency() )
        {
            case 11025: sound.rate = RATE_11; break;
            case 22050: sound.rate = RATE_22; break;
            case 44100: sound.rate = RATE_44; break;
        }

        // MP3's are always 16 bit

        sound.isSample16bit = true;

        // Stereo includes anything that isn't mono ( joint stereo, dual mono )

        sound.isStereo = mp3.getStereo();

        // Sample count as determined for just the frames read

        sound.sampleCount = mp3.getSamples();

        // Default the delay seek to 0 until a better idea comes along

        sound.delaySeek = 0;

        // Datamarker containing just valid MP3 frames
        // no copying here!
        sound.data = new DataMarker( fb.getBuf(), 0, fb.getSize() );

        return sound;
    }

    public int getSize() {
        return data.buffer.length;
    }

    /** Write MP3Sound to output buffer */

    public void write( FlashOutput fob )
    {
        fob.writeTag( getTag(), 2 + 1 + 4 + 2 + data.length() );
        fob.writeDefID( this );
        fob.initBits();
        fob.writeBits( compressFormat, 4 );
        fob.writeBits( rate, 2 );
        fob.writeBool( isSample16bit );
        fob.writeBool( isStereo );
        fob.flushBits();
        fob.writeDWord( sampleCount );
        fob.writeWord( delaySeek );         // Only addition for MP3SOUNDDATA
        data.write(fob);
    }

    /** Copies this sound into the provided item ( must be an MP3Sound ) */

    protected FlashItem copyInto( FlashItem item, ScriptCopier copier )
    {
        super.copyInto( item, copier );

        ( ( MP3Sound ) item ).delaySeek = delaySeek;
        ( ( MP3Sound ) item ).data = ( DataMarker ) data.getCopy();

        return item;
    }

    /** Duplicates this sound */

    public FlashItem getCopy( ScriptCopier copier )
    {
        return copyInto( new MP3Sound(), copier );
    }

    /** DelaySeek accessor */

    public int getDelaySeek() { return delaySeek; }

    /** DelaySeek mutator */

    public void setDelaySeek( int val ) { delaySeek = val; }
}
