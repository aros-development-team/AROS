/*
 * $Id: MP3SoundCommand.java,v 1.5 2002/07/22 23:16:04 skavish Exp $
 *
 * ===========================================================================
 *
 */

package org.openlaszlo.iv.flash.commands;

import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.sound.*;
import org.openlaszlo.iv.flash.api.shape.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.cache.*;
import org.openlaszlo.iv.flash.url.*;

import org.openlaszlo.iv.flash.context.Context;
import java.io.*;

/**
 * Class to support the Insert MP3 generator template.<BR>
 *
 * @author James Taylor
 * @author Andrew Wason
 */

public class MP3SoundCommand extends GenericCommand {

    public MP3SoundCommand() {
    }

    public void doCommand( FlashFile file, Context context, Script parent, int frame ) throws IVException
    {
        // Get object parameters

        String filename = getParameter( context, "filename", "" );
        boolean cache = getBoolParameter( context, "cache", false );
        boolean stream = getBoolParameter( context, "stream", false );
        boolean stopAction = getBoolParameter( context, "stopaction", true );
        int delay = getIntParameter( context, "delay", 0 );
        String instancename = getParameter( context, "instancename" );

        IVUrl url = IVUrl.newUrl( filename, file );

        Script script = getInstance().copyScript();

        FlashBuffer fob = (FlashBuffer) MediaCache.getMedia(url);
        if( fob == null ) {
            try {
                fob = new FlashBuffer(url.getInputStream());
                MediaCache.addMedia(url, fob, fob.getSize(), cache);
            } catch( IOException e ) {
                throw new IVException(Resource.ERRCMDFILEREAD, new Object[] {filename, getCommandName()}, e);
            }
        }

        Frame stopFrame = null;

        try {
            if( stream ) {
                SoundStreamBuilder ssb = SoundStreamBuilder.newSoundStreamBuilder(fob, file.getFrameRate());
                SoundStreamHead head = ssb.getSoundStreamHead();

                // Add the SoundStreamHead to the current frame in the parent script
                parent.getFrameAt( frame ).addFlashObject( head );

                int frameCount = parent.getFrameCount();
                int f = frame;
                SoundStreamBlock block;

                while( ( block = ssb.getNextSoundStreamBlock() ) != null ) {
                    if( f >= frameCount ) {
                        parent.newFrame().addFlashObject( block );
                    } else {
                        parent.getFrameAt( f ).addFlashObject( block );
                    }

                    f++;
                }

                getInstance().def = Shape.newEmptyShape1();

                stopFrame = parent.getFrameAt( f - 1 );
            } else {
                MP3Sound sound = MP3Sound.newMP3Sound(fob);
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
        } catch( IOException e ) {
            throw new IVException(Resource.ERRCMDFILEREAD, new Object[] {filename, getCommandName()}, e);
        }

        if( instancename != null ) {
            getInstance().name = instancename;
        }

        if( stopAction ) {
            stopFrame.addStopAction();
        }
    }
}
