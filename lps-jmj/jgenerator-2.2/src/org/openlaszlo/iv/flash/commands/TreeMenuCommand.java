/*
 * $Id: TreeMenuCommand.java,v 1.1 2002/04/19 06:10:34 skavish Exp $
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
import org.openlaszlo.iv.flash.commands.*;
import org.openlaszlo.iv.flash.context.*;
import org.openlaszlo.iv.flash.util.*;

/**
 * TreeMenuCommand JGenerator Object
 *
 * @author  William L. Thomson Jr.
 * @company Obsidian-Studios Inc.
 * @version 1.0
 * @date April 16, 2002
 */
public class TreeMenuCommand extends GenericCommand {

    protected int                           itemPad;
    protected int                           upClipColumn,overClipColumn,numColumn,parentColumn,directionColumn,titleColumn,urlColumn,windowColumn,goToColumn,goToLabelColumn = -1;
    protected Script                        headerSymbol,footerSymbol;
    protected String                        dataSource,headerSymbolName,footerSymbolName,instanceName,closeLevel;

    protected void initColumnIndexes(String[][] data) {
        upClipColumn = findColumn("upClip",data);
        overClipColumn = findColumn("overClip",data);
        numColumn = findColumn("Num",data);
        parentColumn = findColumn("Parent",data);
        directionColumn = findColumn("Direction",data);
        titleColumn = findColumn("Title",data);
        urlColumn = findColumn("Url",data);
        windowColumn = findColumn("Window",data);
        goToColumn = findColumn("goto",data);
        goToLabelColumn = findColumn("gotoLabel",data);
        if( upClipColumn==-1 ) Log.error("TreeMenuCommand Error\nA upClip column was not specified in the data source.");
        if( overClipColumn==-1 ) Log.error("TreeMenuCommand Error\nA overClip column was not specified in the data source.");
        if( numColumn==-1 ) Log.error("TreeMenuCommand Error\nA num column was not specified in the data source.");
        if( parentColumn==-1 ) Log.error("TreeMenuCommand Error\nA parent column was not specified in the data source.");
        if( directionColumn==-1 ) Log.error("TreeMenuCommand Error\nA direction column was not specified in the data source.");
        if( titleColumn==-1 ) Log.error("TreeMenuCommand Error\nA title column was not specified in the data source.");
        if( urlColumn==-1 && goToColumn==-1 && goToColumn==-1 ) Log.error("TreeMenuCommand Error\nNeither a url or goto or gotoLabel column was not specified in the data source.");
    }
    protected void initParams(Context context) {
        dataSource = getParameter(context,"dataSource", "");
        headerSymbolName = getParameter(context,"headerSymbol", "");
        footerSymbolName = getParameter(context,"footerSymbol", "");
        instanceName = getParameter(context,"instanceName", "");
        closeLevel = getParameter(context,"closeLevel", "All");
        itemPad = getIntParameter(context,"itemPad",10);
        if( dataSource.equals("") ) Log.error("TreeMenuCommand Error\nA data source was not specified in authoring environment.");
        if( headerSymbolName.equals("") ) Log.error("TreeMenuCommand Error\nA header symbol was not specified in authoring environment.");
        if( footerSymbolName.equals("") ) Log.error("TreeMenuCommand Error\nA footer symbol was not specified in authoring environment.");
        if( instanceName.equals("") ) Log.error("TreeMenuCommand Error\nA instance name was not specified in authoring environment.");
    }
    protected void initSymbols(FlashFile flashFile) {
        headerSymbol = flashFile.getScript(headerSymbolName);
        footerSymbol = flashFile.getScript(footerSymbolName);
        if( headerSymbol==null ) Log.error("TreeMenuCommand Error\nCould not get the header script from the flash file.");
        if( footerSymbol==null ) Log.error("TreeMenuCommand Error\nCould not get the footer script from the flash file.");
    }
    public void doCommand (FlashFile flashFile,Context context,Script parentScript,int frameNum) {
        initParams(context);
        initSymbols(flashFile);
        String[][] data = null;
        try {
            UrlDataSource urlDataSource = new UrlDataSource(dataSource,flashFile);
            data = urlDataSource.getData();
        } catch( IVException ive ) {
            Log.error("TreeMenuCommand Error\nCould not Parse the datasource into a multi-dimensional array because :\n"+ive);
        } catch( IOException ioe ) {
            Log.error("TreeMenuCommand Error\nCould not Parse the datasource into a multi-dimensional array because :\n"+ioe);
        }
        if( data!=null ) {
            initColumnIndexes(data);
            makeMenu(flashFile,data);
        } else {
            Log.error("TreeMenuCommand Error\nThe datasource it empty.");
        }
    }
    protected void makeMenu(FlashFile flashFile,String[][] data) {
        Instance instance = getInstance();                                      // get an instance of the template
        instance.name = instanceName;                                           // set the name of the instance
        Script script = instance.copyScript();                                  // copy the instance's script
        Frame frame = null;
        if( closeLevel.equalsIgnoreCase("All") ) {                               // Check to see if we should close all menus
            script.getFrameAt(0).addStopAction();                               // add a stop action to frame 1
            script.newFrame();                                                  // create a new frame so we now have two
            frame = script.getFrameAt(1);                                       // frame 2 will represent menu open this and all sub menus
        } else
            frame = script.getFrameAt(0);                                       // get the first frame and show the first main menu
        frame.addStopAction();                                                  // and add a stop action to it
        String direction = "up";                                                // direction used for footer and header placement
        double buttonWidth = 0;
        double buttonHeight = 0;
        double y = 0;                                                           // y (vertical) to space out each button
        double x = 0;                                                           // x for later
        int row = 0;                                                            // row of data represents row in table
        int menuItem = 0;                                                      // number of items, buttons or menus in this menu
        if( flashFile.getMainScript().getBounds().getY()<flashFile.getMainScript().getBounds().getHeight()/2 ) direction = "down";
        for( row =1;row<data.length;row++ ) {
            if( data[row][parentColumn].equals("0") ) {                          // get the main menu data and create the main buttons and sub menus
                Script buttonScript = createButton(flashFile,data,row,menuItem);
                buttonWidth = buttonScript.getBounds().getWidth();              // get the width before creating and adding a sub menu
                buttonHeight = buttonScript.getBounds().getHeight();            // get the height before creating and adding a sub menu
                makeSubMenu(flashFile,data,row,buttonScript,buttonWidth,buttonHeight);
                frame.addInstance(buttonScript,row+1,AffineTransform.getTranslateInstance(x,y),null,"item"+menuItem);
                if( data[row][directionColumn].equalsIgnoreCase("down") ) {
                    direction = "down";
                    y += buttonHeight-itemPad;
                } else {
                    direction = "up";
                    y -= buttonHeight-itemPad;
                }
                menuItem++;
            }
        }
        Button2 invisibleButton = createInvisibleButton(script.getBounds());
        if( direction.equals("down") ) {
            frame.addInstance(invisibleButton,0,AffineTransform.getTranslateInstance(x,0),null);            // add the invisible button that will close the whole menu
            addHeader(frame,row,x,itemPad);
            addFooter(frame,row,x,y-itemPad);
        } else {
            frame.addInstance(invisibleButton,0,AffineTransform.getTranslateInstance(x,-(y+buttonHeight)),null); // add the invisible button that will close the whole menu
            addFooter(frame,row,x,buttonHeight-itemPad);
            addHeader(frame,row,x,y+buttonHeight-itemPad);
        }
    }
    protected void makeSubMenu(FlashFile flashFile,String[][] data,int row,Script buttonScript,double buttonWidth,double buttonHeight) {
        String direction = "up";
        double subButtonWidth = 0;
        double subButtonHeight = 0;
        double subX = buttonWidth;
        double subY = 0;
        int subRow = 0;
        int subMenuItem = 0;
        Frame innerFrame = null;
        for( subRow=1;subRow<data.length;subRow++ ) {
            if( Integer.parseInt(data[subRow][parentColumn])==row ) {
                if( innerFrame==null ) {
                    buttonScript.getFrameAt(0).addStopAction();
                    innerFrame = buttonScript.newFrame();
                    innerFrame.addStopAction();
                }
                Script subButtonScript = createButton(flashFile,data,subRow,subMenuItem);
                subButtonWidth = subButtonScript.getBounds().getWidth();      // get the width before creating and adding a sub menu
                subButtonHeight = subButtonScript.getBounds().getHeight();    // get the height before creating and adding a sub menu
                makeSubMenu(flashFile,data,subRow,subButtonScript,subButtonWidth,subButtonHeight);
                innerFrame.addInstance(subButtonScript,1,AffineTransform.getTranslateInstance(subX,subY),null,"item"+subMenuItem);
                if( data[subRow][directionColumn].equalsIgnoreCase("down") ) {
                    direction = "down";
                    subY += subButtonHeight-itemPad;
                } else {
                    direction = "up";
                    subY -= subButtonHeight-itemPad;
                }
                subMenuItem++;
            }
        }
        if( innerFrame!=null ) {
            if( direction.equals("down") ) {
                addHeader(innerFrame,subRow,subX,itemPad);
                addFooter(innerFrame,subRow,subX,subY-itemPad);
            } else {
                addFooter(innerFrame,subRow,subX,subButtonHeight-itemPad);
                addHeader(innerFrame,subRow,subX,subY+subButtonHeight-itemPad);
            }
        }
    }
    protected void addFooter(Frame frame,int level,double x,double y) {
        frame.addInstance(footerSymbol,level,AffineTransform.getTranslateInstance(x,y),null);
    }
    protected void addHeader(Frame frame,int level,double x,double y) {
        frame.addInstance(headerSymbol,level,AffineTransform.getTranslateInstance(x,y),null);
    }
    protected Script createButton(FlashFile flashFile,String[][] data,int row,int itemNumber) {
        String name = data[row][titleColumn];
        Script script = new Script(1);
        Frame frame = script.getFrameAt(0);
        Context buttonContext = null;
        try {
            buttonContext = ContextFactory.createContext(data,row);                             // create a new context
        } catch( IVException ive ) {
            Log.error("TreeMenuCommand Error\nCould not make new button context because :\n"+ive);
        }
        Script upClipScript = flashFile.getScript(data[row][upClipColumn]).copyScript();        // get a copy of the upClip script
        Script overClipScript = flashFile.getScript(data[row][overClipColumn]).copyScript();    // get a copy of the overClip script
        try {
            flashFile.processScript(upClipScript,buttonContext);
            flashFile.processScript(overClipScript,buttonContext);
        } catch( IVException ive ) {
            Log.error("TreeMenuCommand Error\nCould not process new button context because :\n"+ive);
        }
        Button2 button2 = new Button2();
        button2.addActionCondition(onRollOver(itemNumber));
        button2.addActionCondition(onRelease(data,row));
        button2.addButtonRecord(new ButtonRecord(ButtonRecord.Up,upClipScript,1,AffineTransform.getTranslateInstance(0,0),CXForm.newIdentity(true)));
        button2.addButtonRecord(new ButtonRecord(ButtonRecord.Over|ButtonRecord.Down,overClipScript,2,AffineTransform.getTranslateInstance(0,0),CXForm.newIdentity(true)));
        Shape shape = new Shape();
        shape.setBounds(upClipScript.getBounds());
        shape.setFillStyle0(FillStyle.newSolid(new Color(0,0,0)));
        shape.drawRectangle(upClipScript.getBounds());
        button2.addButtonRecord(new ButtonRecord(ButtonRecord.HitTest,shape,3,AffineTransform.getTranslateInstance(0,0),CXForm.newIdentity(true)));
        button2.setTrackAs(Button2.TrackAsMenu);
        frame.addInstance(button2,1,AffineTransform.getTranslateInstance(0,0),null,name);
        return(script);
    }
    protected Button2 createInvisibleButton(Rectangle2D rectangle2D) {
        Button2 button2 = new Button2();
        Program program = new Program();
        program.gotoFrame(0);
        button2.addActionCondition(ActionCondition.onRollOver(program));
        Shape shape = new Shape();
        shape.setBounds(rectangle2D);
        shape.setFillStyle0(FillStyle.newSolid(new Color(0,0,0)));
        shape.drawRectangle(rectangle2D);
        button2.addButtonRecord(new ButtonRecord(ButtonRecord.HitTest,shape,1,AffineTransform.getTranslateInstance(0,0),CXForm.newIdentity(true)));
        button2.setTrackAs(Button2.TrackAsButton);
        return(button2);
    }
    protected ActionCondition onRollOver(int itemNumber) {
        Program program = new Program();
        program.setTarget("../item"+(itemNumber-1));                            // tell the menu item just before this one to go to frame 1 menu closed
        program.gotoFrame(0);                                                   //to go to frame 1 menu closed
        program.setTarget("../item"+(itemNumber+1));                            // tell the menu item just after this one
        program.gotoFrame(0);                                                   // to go to frame 1 menu closed
        program.setTarget("");                                                  // go back to the timeline this button is on
        program.gotoFrame(1);                                                   // go to frame 2 menu open
        return(ActionCondition.onRollOver(program));
    }
    protected ActionCondition onRelease(String[][] data,int row) {
        Program program = new Program();
        if( urlColumn!=-1 )
            program.getURL(data[row][urlColumn],data[row][windowColumn]);
        else if( goToColumn!=-1 )
            program.gotoFrame(Integer.parseInt(data[row][goToColumn]));
        else if( goToLabelColumn!=-1 )
            program.gotoLabel(data[row][goToLabelColumn]);
        return(ActionCondition.onRelease(program));
    }
}
