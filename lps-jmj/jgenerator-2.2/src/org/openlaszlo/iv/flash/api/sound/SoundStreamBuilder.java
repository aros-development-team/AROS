/*
 * $Id: SoundStreamBuilder.java,v 1.2 2002/02/15 23:44:28 skavish Exp $
 *
 * ===========================================================================
 *
 */

package org.openlaszlo.iv.flash.api.sound;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.url.*;

import java.io.*;

public class SoundStreamBuilder
{
    /** Holds the MP3 samples are read from */

    private MP3Helper mp3;

    /** Calculated ideal samples per block: sampleRate / frameRate */

    private int idealSamplesPerBlock = 0;

    /** Frame rate of containing file */

    private int frameRate = 0;

    /** Total samples completed so far */

    private int samplesCompleted = 0;

    /** Total sound blocks completed so far */

    private int blocksCompleted = 0;

    /** Saves us the trouble of mark and reset */

    private byte[] firstFrameCache = null;

    /** Data size (skavish 05/03/2001) */
    private int dataSize = 0;

    /** Create SoundStreamBuilder from url and frame rate */

    public static SoundStreamBuilder newSoundStreamBuilder( IVUrl url, int rate ) throws IOException, IVException
    {
        return newSoundStreamBuilder( Util.readUrl( url ), rate );
    }

    /** Create a SoundStreamBuilder for the provided buffer and frame rate */

    public static SoundStreamBuilder newSoundStreamBuilder( FlashBuffer fob, int rate ) throws IOException, IVException
    {
        SoundStreamBuilder o = new SoundStreamBuilder();

        o.mp3 = new MP3Helper( fob );
        o.frameRate = rate >> 8;
        o.dataSize = fob.getSize();

        return o;
    }

    /** Get the SoundStreamHead object for this sound */

    public SoundStreamHead getSoundStreamHead() throws IOException, IVException
    {
        SoundStreamHead o = new SoundStreamHead();

        firstFrameCache = mp3.nextFrame();

        o.streamCompression = Sound.COMPRESS_MP3;

        // Set the rate from the MP3's frequency

        switch ( mp3.getFrequency() )
        {
            case 11025: o.playbackRate = o.streamRate = Sound.RATE_11; break;
            case 22050: o.playbackRate = o.streamRate = Sound.RATE_22; break;
            case 44100: o.playbackRate = o.streamRate = Sound.RATE_44; break;
        }

        o.isPlayback16bit = o.isStream16bit = true;

        o.isPlaybackStereo = o.isStreamStereo = mp3.getStereo();

        // This can only be set once a block has been read

        o.streamSampleCount = idealSamplesPerBlock = ( mp3.getFrequency() / frameRate );

        return o;
    }

    /** Get the next StreamSoundBlock for this sound */

    public SoundStreamBlock getNextSoundStreamBlock() throws IOException, IVException
    {
        byte [] buffer;
        int sampleCount = 0;

        ByteArrayOutputStream out = new ByteArrayOutputStream();

        // Increment the blocks completed upfront, so the division in the
        // while loop works nicely.

        blocksCompleted++;

        // Add frames to the ouput buffer while the average samples per block
        // completed does not exceed the ideal.

        while ( ( samplesCompleted / blocksCompleted ) < idealSamplesPerBlock )
        {
            // Get the next frame ( unless one is already cached )

            if ( firstFrameCache == null )
            {
                buffer = mp3.nextFrame();
            }
            else
            {
                buffer = firstFrameCache;

                firstFrameCache = null;
            }

            // End loop prematurely if there are no more frames

            if ( buffer == null )
            {
                break;
            }

            // Write the frame to the output buffer

            out.write( buffer, 0, buffer.length );

            samplesCompleted += mp3.getSamplesPerFrame();
            sampleCount += mp3.getSamplesPerFrame();
        }

        // If no samples are in the buffer at this point, there must be no more sound

        if ( sampleCount == 0 )
        {
            return null;
        }

        // If all has gone well, build the new block and return it

        MP3SoundStreamBlock o = new MP3SoundStreamBlock();

        o.sampleCount = sampleCount;
        o.delaySeek = 0;

        o.data = new DataMarker( out.toByteArray(), 0, out.size() );

        return o;
    }

    /**
     * Return data size
     */
    public int getSize() {
        return dataSize;
    }

}
