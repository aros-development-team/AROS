/*
 * $Id: MP3Helper.java,v 1.4 2002/06/24 13:51:55 awason Exp $
 *
 * ===========================================================================
 *
 */

package org.openlaszlo.iv.flash.util;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.url.*;
import org.openlaszlo.iv.flash.api.*;

import java.io.*;

/**
 * Class for parsing MP3 files, but only accepts those which the flash player
 * understands.
 *
 * @author James Taylor
 *
 */

public class MP3Helper
{
    private InputStream in;

    private boolean isSynched = false;

    // Per frame porperties ( but should be constant except pad and framesize )

    private int mode;
    private int layer;
    private int bitrate;
    private int frequency;
    private int pad;
    private int frameSize;

    private int samplesPerFrame;

    // Counts samples read _so far_

    private int samples = 0;

    private boolean stereo;

    /* MP3 has a constant number of samples per frame */

    private static int SAMPLES_PER_FRAME_V1 = 1152;
    private static int SAMPLES_PER_FRAME_V2 = 576;


    /* Flash only supports mpeg audio layer 3, so only the bitrates for layer 3
     * are included here. Row 1 is for version 1, row 2 is for version 2 and 2.5
     */

    private static int bitrates[][] =
    {
        { -1, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1 },
        { -1, 8,  16, 24, 32, 40, 48, 56,  64,  80,  96, 112, 128, 144, 160, -1 }
    };

    /* Frequencies that flash supports. Probably a waste to use a 2d array here
     * since flash only allows the first column ( 00 for frequency ).
     *
     */

    private static int frequencies[][] =
    {
        { 11025, -1, -1, -1 },
        {    -1, -1, -1, -1 },
        { 22050, -1, -1, -1 },
        { 44100, -1, -1, -1 }
    };

    /**
     * Creates a new MP3Helper which reads MP3Frames from the supplied buffer.
     *
     */

    public MP3Helper( FlashBuffer fob ) throws IOException, IVException
    {
        in = fob.getInputStream();
        skipID3(fob);
    }

    /*
     * Skip ID3 header.
     * If it is not using an "unsynchronization scheme", it can confuse sync()
     * http://www.id3.org/develop.html
     */
    private void skipID3( FlashBuffer fob ) throws IOException {
        int b1, b2, b3, b4;
        // Do nothing if no valid ID3 header
        if (fob.getSize() < 10
            || fob.getByteAt(0) != 'I' || fob.getByteAt(1) != 'D' || fob.getByteAt(2) != '3'
            || fob.getUByteAt(3) >= 0xff || fob.getUByteAt(4) >= 0xff
            || (b1 = fob.getUByteAt(6)) >= 0x80 || (b2 = fob.getUByteAt(7)) >= 0x80
            || (b3 = fob.getUByteAt(8)) >= 0x80 || (b4 = fob.getUByteAt(9)) >= 0x80)
            return;

        // Skip ID3 data - size is a 28bit integer, with high bit of each byte ignored
        in.skip(10 + (b1<<21 | b2<<14 | b3<<7 | b4));
    }

    /**
     * Reads the header of the next MP3 frame, synchronzing the stream first
     * if that has not yet been done.
     *
     * @return the 4 byte header, or null if there are no more frames.
     *
     */

    public byte[] readHeader() throws IOException, IVException
    {
        byte[] header;

        if ( isSynched == false )
        {
            header = sync();
        }
        else
        {
            header = new byte[ 4 ];

            if ( in.read( header, 0, 4 ) != 4 )
            {
                return null;
            }

        }

        // Verify the header

        if ( ( ( header[0] & 0xFF ) != 0xFF ) )
        {
            return null;
        }

        // Parse header data

        mode = ( header[1] >>> 3 ) & 0x03;

        samplesPerFrame = ( mode == 3 ) ? SAMPLES_PER_FRAME_V1 : SAMPLES_PER_FRAME_V2;

        layer = ( header[1] >>> 1 ) & 0x03;

        // Only layer 3 files are valid for the flash player

        if ( layer != 1 )
        {
            throw new IVException( Resource.INVLMP3LAYER );
        }

        // Get bitrate from bitrates array -- varies with mode

        bitrate = bitrates[ ( mode == 3 ) ? 0 : 1][ ( header[2] >>> 4 ) & 0x0F ];

        // Get frequency -- also varies with mode

        frequency = frequencies[ mode ][ ( header[2] >>> 2 ) & 0x03 ];

        if ( frequency == -1 )
        {
            throw new IVException( Resource.INVLMP3FREQUENCY );
        }

        // Channel mode

        stereo = ( ( ( header[3] >>> 6 )  & 0x03 ) != 3 );

        // Determine is this frame is padded

        pad = ( ( header[2] >>> 1 ) & 0x01 );

        // Increment the sample counter

        samples += samplesPerFrame;

        // Calculate the frame size

        frameSize = ( ( ( mode == 3 ) ? 144 : 72 ) * bitrate * 1000 / frequency + pad );

        return header;
    }

    /**
     * Reads from the input stream until it finds an MP3 header.
     *
     * @return the 4 byte header.
     *
     */

    private byte[] sync() throws IOException, IVException
    {
        byte[] header = new byte[ 4 ];

        while ( in.read( header, 0, 1 ) == 1 )
        {
            if ( (header[0] & 0xFF) == 0xFF )
            {
                if ( in.read( header, 1, 1 ) != 1 )
                    throw new IVException( Resource.INVLMP3 );
                if ( (header[1] & 0xE0) == 0xE0 )
                    break;
            }
        }

        // Read remainder of header

        if ( in.read( header, 2, 2 ) != 2 )
        {
            throw new IVException( Resource.INVLMP3 );
        }

        isSynched = true;

        return header;
    }

    /**
     * Processes the next frame in the stream, and sets the frame properties
     * ( mode, layer, bitrate, frequency, framesize, pad )
     *
     * @return the contents of the frame.
     *
     */

    public byte[] nextFrame() throws IOException, IVException
    {
        byte[] header;
        byte[] frame;

        header = readHeader();

        if ( header == null )
        {
            return null;
        }

        int datasize = frameSize - header.length;

        frame = new byte[ frameSize ];

        System.arraycopy( header, 0, frame, 0, header.length );

        if( in.read( frame, header.length, datasize ) == -1 )
        {
            throw new IVException( Resource.INVLMP3 );
        }

        return frame;
    }

    /** Frequency accessor */

    public int getFrequency() { return frequency; }

    /** Stereo accessor */

    public boolean getStereo() { return stereo; }

    /** Samples accessor */

    public int getSamples() { return samples; }

    /** SamplesPerFrame accessor */

    public int getSamplesPerFrame() { return samplesPerFrame; }
}
