/*
 * $Id: MultipageListCommand.java,v 1.1 2002/04/19 06:10:34 skavish Exp $
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

package org.openlaszlo.iv.flash.commands;

import java.awt.geom.*;
import java.io.*;
import java.util.*;
import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.api.action.*;
import org.openlaszlo.iv.flash.api.button.*;
import org.openlaszlo.iv.flash.api.shape.*;
import org.openlaszlo.iv.flash.context.*;
import org.openlaszlo.iv.flash.util.*;

/**
 * Multipage List JGenerator Object
 *
 * @author  William L. Thomson Jr.
 * @company Obsidian-Studios Inc.
 * @version 1.0
 * @date April 17, 2002
 */
public class MultipageListCommand extends GenericCommand {

    protected int                                   textColumn,clipColumn,urlColumn,windowColumn = -1;
    protected Script                                externalMediaScript,prevSymbol,homeSymbol,nextSymbol,textSymbol,templateScript = null;
    protected String                                dataSource,prevSymbolName,homeSymbolName,nextSymbolName,textSymbolName,instanceName,externalMediaName;

    protected void initColumnIndexes(String[][] data) {
        textColumn = findColumn("Text",data);
        urlColumn = findColumn("Url",data);
        windowColumn = findColumn("Window",data);
        if (textColumn==-1) Log.error("MultipageListCommand Error\nA text column was not specified in the data source.");
        if (urlColumn!=-1 && windowColumn==-1) Log.warn("MultipageListCommand Warning\nA url was specified but a window column was not specified in the data source. Will end up using default window, _blank");
    }
    protected void initParams(Context context) {
        dataSource = getParameter(context,"datasource","");
        prevSymbolName = getParameter(context,"prevsym","");
        homeSymbolName = getParameter(context,"homesym","");
    nextSymbolName = getParameter(context,"nextsym","");
    textSymbolName = getParameter(context,"textsym","");
        instanceName = getParameter(context,"instancename","");
        if (dataSource.equals("")) Log.error("MultipageListCommand Error\nA data source was not specified in authoring environment.");
        if (prevSymbolName.equals("")) Log.error("MultipageListCommand Error\nA previous symbol was not specified in authoring environment.");
        if (homeSymbolName.equals("")) Log.error("MultipageListCommand Error\nA home symbol was not specified in authoring environment.");
        if (nextSymbolName.equals("")) Log.error("MultipageListCommand Error\nA next symbol was not specified in authoring environment.");
        if (textSymbolName.equals("")) Log.error("MultipageListCommand Error\nA text symbol was not specified in authoring environment.");
        if (instanceName.equals("")) Log.error("MultipageListCommand Error\nA instance name was not specified in authoring environment.");
    }
    protected void initSymbols(FlashFile flashFile) {
        prevSymbol = flashFile.getScript(prevSymbolName);
        homeSymbol = flashFile.getScript(homeSymbolName);
        nextSymbol = flashFile.getScript(nextSymbolName);
        textSymbol = flashFile.getScript(textSymbolName);
        if (prevSymbol==null) Log.error("MultipageListCommand Warning\nCould not get previous symbol script specified.");
        if (homeSymbol==null) Log.error("MultipageListCommand Warning\nCould not get home symbol script specified.");
        if (nextSymbol==null) Log.error("MultipageListCommand Warning\nCould not get next symbol script specified.");
    if (textSymbol==null) Log.error("MultipageListCommand Error\nCould not get text symbol script specified.");
    }
    public void doCommand(FlashFile flashFile,Context context,Script parentScript, int frames) {
        initParams(context);
        initSymbols(flashFile);
        String[][] data = null;
        try {
            UrlDataSource urlDataSource = new UrlDataSource(dataSource,flashFile);
            data = urlDataSource.getData();
        }
        catch (IVException ive) {
            Log.error("MultipageListCommand Error\nCould not Parse the datasource into a multi-dimensional array because :\n"+ive);
        }
        catch (IOException ioe) {
            Log.error("MultipageListCommand Error\nCould not Parse the datasource into a multi-dimensional array because :\n"+ioe);
        }
        if (data!=null) {
            initColumnIndexes(data);
            makeLists(flashFile,data);        // make the lists
        } else {
            Log.error("MultipageListCommand Error\nThe datasource it empty.");
        }
    }
    protected void makeLists(FlashFile flashFile,String[][] data) {
        Instance instance = getInstance();                                      // get an instance of the template
        instance.name = instanceName;                                           // set the name of the instance
        double width = instance.matrix.getScaleX()*2048;                        // determine the bounding box width
        double height = instance.matrix.getScaleY()*2048;                       // determine the bounding box height
        GeomHelper.deScaleMatrix(instance.matrix);                              // scale the instance, otherwise every clip will be distorted
        Script script = instance.copyScript();
        Frame frame = script.getFrameAt(0);
        frame.addStopAction();
        double prevHeight = prevSymbol.getBounds().getHeight();
        double prevWidth = prevSymbol.getBounds().getWidth();
        Button2 prevButton  = createButton(prevSymbol);
        prevButton.addActionCondition(onReleasePrev());
        double homeHeight = homeSymbol.getBounds().getHeight();
        double homeWidth = homeSymbol.getBounds().getWidth();
        Button2 homeButton  = createButton(homeSymbol);
        homeButton.addActionCondition(onReleaseHome());
        double nextHeight = nextSymbol.getBounds().getHeight();
        double nextWidth = nextSymbol.getBounds().getWidth();
        Button2 nextButton = createButton(nextSymbol);
        nextButton.addActionCondition(onReleaseNext());
        double symbolHeight = (prevHeight > nextHeight) ? prevHeight : nextHeight;
        double y = -height/2+symbolHeight;
        double x = -width/2;
        double clipHeight = 0;
        double newPageHeight = 0;                                                  // this will be a cumlative total of clip heights on a page
        double pageHeight = height-symbolHeight;                                                 // this will be used to determine the total height we have on a page
        int level = 1;
        for (int row =1;row<data.length;row++) {
            Script clipScript = textSymbol.copyScript();
            Context clipContext = null;
            try {
                clipContext = ContextFactory.createContext(data,row);           // create a new context
            }
            catch (IVException ive) {
                Log.error("MultipageListCommand Error\nCould not make new button context because :\n"+ive);
            }
            try {
                flashFile.processScript(clipScript,clipContext);
            }
            catch (IVException ive) {
                Log.error("MultipageListCommand Error\nCould not process new button context because :\n"+ive);
            }
            clipHeight = clipScript.getBounds().getHeight();                    // get the height before creating and adding a sub menu
            newPageHeight += clipHeight;                                        // add the clip height to our new page's height
            if(newPageHeight>pageHeight) {                                      // check to see if there is still room on the page
                frame = script.newFrame();                                      // since there is no more room, let's make another page on a new frame
                frame.addStopAction();                                          // add a stop action, unless you want ticking pages :)
                script.removeAllInstances(frame);                               // remove all the previous instances from this frame
                y = -height/2+symbolHeight;                                     // reset the y coordinate to start were the nav symbols stop
                newPageHeight = clipHeight;                                     // reset the page height to just the clip height that will be added
                level = 1;                                                      // start over again at the bottom level
            }
            if (urlColumn!=-1) {
                Button2 button2 = createButton(data,row,clipScript);
                frame.addInstance(button2,level,AffineTransform.getTranslateInstance(x,y),null);
            } else {
                frame.addInstance(clipScript,level,AffineTransform.getTranslateInstance(x,y),null);
            }
            y += clipHeight;                                                    // add the clip height to the current y position
            level++;
        }
        int frameCount = script.getFrameCount();
        if(frameCount>1) {                                                      // if we have more than one frame add the navigation buttons
            Frame tempFrame = script.getFrameAt(1);
            double navY = -height/2;
            tempFrame.addInstance(prevButton,100,AffineTransform.getTranslateInstance(x,navY),null);            // add the previous button to all frames except the first
            x += prevWidth+40;                                                                                  // add 40 to space out nav buttons
            tempFrame.addInstance(homeButton,101,AffineTransform.getTranslateInstance(x,navY),null);            // add the home button to all frames except the first
            x += homeWidth+40;                                                                                  // add 40 to space out nav buttons
            script.getFrameAt(0).addInstance(nextButton,102,AffineTransform.getTranslateInstance(x,navY),null); // add the next button to all frames including the first
            script.getFrameAt(frameCount-1).removeInstance(102);                                                // remove the next button from the last frame
        }
    }
    protected ActionCondition onReleasePrev() {
        Program program = new Program();
        program.prevFrame();
        return(ActionCondition.onRelease(program));
    }
    protected ActionCondition onReleaseHome() {
        Program homeProgram = new Program();
        homeProgram.gotoFrame(0);
        return(ActionCondition.onRelease(homeProgram));
    }
    protected ActionCondition onReleaseNext() {
        Program nextProgram = new Program();
        nextProgram.nextFrame();
        return(ActionCondition.onRelease(nextProgram));
    }
    protected Button2 createButton(Script symbol) {
        Button2 button2 = new Button2();
        int states = ButtonRecord.Up|ButtonRecord.Over|ButtonRecord.Down;                           // add the symbol to define the rest of the states
        button2.addButtonRecord(new ButtonRecord(states,symbol,1,AffineTransform.getTranslateInstance(0,0),CXForm.newIdentity(true)));
        Shape shape = new Shape();
        shape.setBounds(symbol.getBounds());
        shape.setFillStyle0(FillStyle.newSolid(new Color(0,0,0)));
        shape.drawRectangle(symbol.getBounds());
        button2.addButtonRecord(new ButtonRecord(ButtonRecord.HitTest,shape,2,AffineTransform.getTranslateInstance(0,0),CXForm.newIdentity(true)));
        button2.setTrackAs(Button2.TrackAsButton);
        return (button2);
    }
    protected Button2 createButton(String[][] data,int row,Script clipScript) {
        Button2 button2 = createButton(clipScript);
        Program program = new Program();
        if (windowColumn==-1) program.getURL(data[row][urlColumn],"_blank");
        else program.getURL(data[row][urlColumn],data[row][windowColumn]);
        button2.addActionCondition(ActionCondition.onPress(program));
        return(button2);
    }
}
