/******************************************************************************
 * Transcoder.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.media;

import java.io.InputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.BufferedInputStream;
import java.io.FileNotFoundException;
import java.io.File;
import java.awt.geom.Rectangle2D;
import java.util.Properties;

// JGenerator APIs
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.sound.*;
import org.openlaszlo.iv.flash.api.image.*;
import org.openlaszlo.iv.flash.util.*;

import org.openlaszlo.utils.SWFUtils;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.server.LPS;

import org.openlaszlo.sc.ScriptCompiler;

// Logger
import org.apache.log4j.*;

/**
 * Simple Media Transcoder
 *
 * Someday this should get build out.
 */
public class Transcoder {

    /** Logger */
    private static Logger mLogger = Logger.getLogger(Transcoder.class);

    /**
     * @return true if the transcoder can do the requested transcode
     * @param in input mime type
     * @param out output mime type
     */
    public static boolean canTranscode(String in, String out) {

        if (!out.equalsIgnoreCase(MimeType.SWF)) {

            if (out.equalsIgnoreCase(FontType.FFT) &&
                in.equalsIgnoreCase(FontType.TTF)) {
                return true;
            }
            return false;
        }

        if (in.equalsIgnoreCase(MimeType.JPEG) || 
            in.equalsIgnoreCase(MimeType.PNG)  || 
            in.equalsIgnoreCase(MimeType.GIF)  ||
            in.equalsIgnoreCase(MimeType.MP3)  ||
            in.equalsIgnoreCase(MimeType.XMP3) ||
            in.equalsIgnoreCase(MimeType.SWF)) {
            return true;
        }
        return false;
    }

    /**
     * @param input buffer of data to be transcoded, caller is responsible
     * for closing this stream someday.  
     * @param from type of input data
     * @param to type of output data
     * @param doStream true if transcode should create streaming audio
     * @throws TranscoderException if there is no transcoder for the request from/to 
     * types.
     */
    public static InputStream transcode(InputStream stream, 
            String from, String to, boolean doStream) 
        throws TranscoderException, IOException {

        if (!to.equalsIgnoreCase(MimeType.SWF)) {
            throw new TranscoderException("Unknown output mime-type: " + to);
        }

        mLogger.debug("Transcoding from " + from + " to " + to);

        // We assume this mime type is correct if we get it
        // NOTE: This will keep us from copying big swf video files
        // an extra time for now...
        if (from.equalsIgnoreCase(MimeType.SWF)) {
            return stream;
        } 

        // Try images
        if (from.equalsIgnoreCase(MimeType.JPEG) || 
            from.equalsIgnoreCase(MimeType.PNG) || 
            from.equalsIgnoreCase(MimeType.GIF) ||
            from.indexOf("image") != -1 ) {
            return convertImageToSWF(stream);
        } else if (from.equalsIgnoreCase(MimeType.MP3) ||
                   from.equalsIgnoreCase(MimeType.XMP3) ||
                   from.indexOf("audio") != -1)  {
            // Try audio
            return convertAudioToSWF(stream, doStream);
        }

        BufferedInputStream bis = null;
        try {
            if (!stream.markSupported()) {
                bis = new BufferedInputStream(stream);
                stream = bis;
            }
            String mime = guessSupportedMimeTypeFromContent(stream);
            if (mime != null) {
                InputStream out = null;
                if (mime.equals(MimeType.SWF)) {
                    out = bis;
                } else {
                    out = transcode(bis, mime, to, doStream);
                }
                // Keep us from closing the stream
                if (bis == out) {
                    bis = null;
                }
                return out;
            } else {
                throw new TranscoderException("can't guess a supported mime-type from content");
            }
        } finally {
            FileUtils.close(bis);
        }
    }

    /**
     * @return mime type based on stream
     */
    public static String guessSupportedMimeTypeFromContent(String fileName) 
        throws IOException {
        InputStream is = null;
        try {
            is = new BufferedInputStream(new FileInputStream(fileName));
            return guessSupportedMimeTypeFromContent(is);
        } finally {
            FileUtils.close(is);
        }
    }

    /**
     * @return mime type based on stream
     * stream must be rewindable or we can't guess
     */
    public static String guessSupportedMimeTypeFromContent(InputStream stream) 
        throws IOException {
        if (!stream.markSupported()) {
            return null;
        }
    
        try {
            stream.mark(stream.available());
            
            mLogger.debug("trying swf");
            if (SWFUtils.hasSWFHeader(stream)) {
                return MimeType.SWF;
            } 

            stream.reset();
            if (GIF.is(stream)) {
                return MimeType.GIF;
            }

            stream.reset();
            if (JPEG.is(stream)) {
                return MimeType.JPEG;
            }

            stream.reset();
            if (PNG.is(stream)) {
                return MimeType.PNG;
            }

            stream.reset();
            if (MP3.is(stream)) {
                return MimeType.MP3;
            }
        } finally {
            stream.reset();
        }

        return null;
    }

    /**
     * @param input File to be transcoded
     * @param from type of input data
     * @param to type of output data
     * @throws TranscoderException if there is no transcoder for 
     * the request from/to 
     */
    public static InputStream transcode(File input, String from, String to) 
        throws TranscoderException, IOException {

        if (to.equalsIgnoreCase(FontType.FFT)) {
            if (from.equalsIgnoreCase(FontType.TTF)) {
                return TTF2FFT.convert(input);
            } else {
                throw new TranscoderException("Unknown input font type: " 
                        + from);
            }
        } else {
            InputStream fis = new FileInputStream(input);
            InputStream out = null;
            try {
                out = transcode(fis, from , to, 
                        /* non-streaming media */false);
                return out;
            } finally {
                if (fis != null && fis != out) {
                    fis.close();
                }
            }
        }
    }

    /**
     * @param stream image input stream
     */
    private static final InputStream convertImageToSWF(InputStream stream) 
        throws IOException, TranscoderException {

        try {
            mLogger.debug("converting image to SWF");
    
            Bitmap bitmap = Bitmap.newBitmap(new FlashBuffer(stream));
            if (bitmap == null) {
                String msg = "corrupt image or unknown image type";
                throw new TranscoderException(msg);
            }
            mLogger.debug("done bitmap file");
            Instance inst = bitmap.newInstance();
            Script script;
            script = new Script(1);
            script.setMain();
            script.newFrame().addInstance(inst, 1);
            FlashFile file = FlashFile.newFlashFile();
            file.setVersion(5);
            file.setFrameSize(inst.getBounds());
            file.setMainScript(script);
            mLogger.debug("starting generate");
            FlashOutput out = file.generate();
            mLogger.debug("ending generate");
            return out.getInputStream();
        } catch (IVException e) {
            throw new TranscoderException("iv exception:" + e.getMessage());
        }
    }

    /**
     * @param stream audio input stream
     * @param doStream if true, convert to streaming audio output
     */
    private static final InputStream convertAudioToSWF(InputStream stream, boolean doStream) 
        throws IOException, TranscoderException {

        // Stream and add a stop play command.
        try {
            return convertAudioToSWF(stream, doStream, true, 0, 0);
        } catch (IVException e) {
            throw new TranscoderException("iv exception:" + e.getMessage());
        }
    }

    /**
     * @param stream audio input stream
     */
    private static final InputStream convertAudioToSWF(InputStream in, boolean stream, boolean stopAction, int delay, int startframe) 
        throws IOException, IVException {

        Script script;
        script = new Script(1);

        FlashFile file = FlashFile.newFlashFile();

        // 30 FPS gets us 16000/30/60 = 8 minutes 53 sec max 
        final int  MAX_SWF_FRAMES = 16000;
        int mFrameRate = 30;
        try {
            String f = LPS.getProperty("lps.swf.audio.framerate", "30");
            mFrameRate = Integer.parseInt(f);
        } catch (Exception e) {
            mLogger.error("Can't read property file for lps.swf.audio.framerate");
        }

        file.setFrameRate(mFrameRate << 8); 

        Frame stopFrame = null;

        FlashBuffer fib = new FlashBuffer(in);
        
        if( stream ) {
            mLogger.debug("transcoding streaming mp3");
            SoundStreamBuilder ssb = SoundStreamBuilder.newSoundStreamBuilder(fib, file.getFrameRate());
            SoundStreamHead head = ssb.getSoundStreamHead();

            // Add the SoundStreamHead to the current startframe in the script
            script.getFrameAt( startframe ).addFlashObject( head );

            int frameCount = script.getFrameCount();
            int f = startframe;
            SoundStreamBlock block;

            while( ( block = ssb.getNextSoundStreamBlock() ) != null ) {
                if( f >= frameCount ) {
                    script.newFrame().addFlashObject( block );
                } else {
                    script.getFrameAt( f ).addFlashObject( block );
                }

                f++;
                if (f >= MAX_SWF_FRAMES) {
                    String msg = 
                            "LPS hit max SWF frame count when converting this clip" 
                            + "; truncating it at " + MAX_SWF_FRAMES + " frames";
                    mLogger.warn(msg);
                    script.getFrameAt(0).addFlashObject(WarningProgram(msg));
                    break;
                }
            }

            stopFrame = script.getFrameAt( f - 1 );
        } else {
            mLogger.debug("transcoding non-streaming mp3");
            MP3Sound sound = MP3Sound.newMP3Sound(fib);
            // Set the delay if provided
            if( delay != 0 ) {
                sound.setDelaySeek( delay );
            }

            SoundInfo soundInfo = SoundInfo.newSoundInfo( 0 );
            StartSound startSound = StartSound.newStartSound( sound, soundInfo );

            Frame newFrame = script.newFrame();
            newFrame.addFlashObject( startSound );

            stopFrame = newFrame;
        }

        if( stopAction ) {
            stopFrame.addStopAction();
        }

        file.setVersion(5);
        file.setFrameSize(GeomHelper.newRectangle(0,0,0,0));
        file.setMainScript(script);
        FlashOutput out = file.generate();
        return out.getInputStream();
    }

    /**
     * Return a FlashObject that contains a program that
     * will print an LFC warning to the debugger
     */
    private static FlashObject WarningProgram(String msg) {
        String p = "_root.debug.write('" + msg + "');";
        byte[] action = ScriptCompiler.compileToByteArray(p, new Properties());
        return new DoAction(new Program(action, 0, action.length));
    }
}
