package org.aros.bootstrap;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Canvas;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;

import java.io.DataOutputStream;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.String;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;

public class AROSBootstrap extends Activity
{
	static final int ID_ERROR_DIALOG = 0;
	static final int ID_ALERT_DIALOG = 1;

	public AROSBootstrap()
	{
		
	}
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
    	// Create an instance of the callback that starts up native code
    	// It will be called from within our DisplayView class after
    	// display layout is done
    	AROSBootstrap.Booter booter = new AROSBootstrap.Booter();

    	rootView = new DisplayView(this, booter);
    	super.onCreate(savedInstanceState);
        setContentView(rootView);
    }

    public void DisplayError(String text)
    {
    	errStr = text;
    	showDialog(ID_ERROR_DIALOG);
    }

    public void DisplayAlert(String text)
    {
    	errStr = text;
    	showDialog(ID_ALERT_DIALOG);

    	Looper.loop();
    }

    public Dialog onCreateDialog(int id)
    {
		AlertDialog.Builder b = new AlertDialog.Builder(this);
		DialogInterface.OnClickListener okEvent; 
    	
    	switch (id)
    	{
    	case ID_ERROR_DIALOG:
    		b.setTitle(R.string.error);
    		okEvent = new DialogInterface.OnClickListener()
        	{	
    			@Override
    			public void onClick(DialogInterface dialog, int which)
    			{
    				System.exit(0);
    			}
    		};
    		break;

    	case ID_ALERT_DIALOG:
    		b.setTitle(R.string.guru);
    		okEvent = new DialogInterface.OnClickListener()
        	{
    			@Override
    			public void onClick(DialogInterface dialog, int which)
    			{
    				// TODO: break run loop and return
    				System.exit(0);
    			}
    		};
    		break;
    	
    	default:
    		return null;
    		
    	}
    	b.setMessage(errStr);
    	b.setCancelable(false);
    	b.setPositiveButton(R.string.ok, okEvent);

    	return b.create();
    }

    static
    {
        System.loadLibrary("AROSBootstrap");
        Log.d("AROS", "Started");
    }

	// This is for far future. Android already supports TV out,
	// just it always displays the same picture as device's screen.
	// But what if things change one day?
	// 'id' parameter is reserved to identify a particular display.
	public DisplayView GetDisplay(int id)
	{
		return rootView;
	}

	private void ReplyCommand(int cmd, int... response)
	{
		int len = (response.length + 1) * 4;
		ByteBuffer bb = ByteBuffer.allocate(len);        
        IntBuffer ib = bb.asIntBuffer();
 
        ib.put(0, cmd);
        ib.put(response, 1, response.length);
 
        try
        {
			InputPipe.write(bb.array());
		}
        catch (IOException e)
        {
        	Log.v("AROS", "Error writing input pipe");
        	System.exit(0);
		}
	}
	
	private void DoCommand(int cmd, int[] params) throws IOException
	{
		switch (cmd)
		{
		case DisplayServer.cmd_Query:
			Log.d("AROS", "cmd_Query( " + params[0] + " )");

			DisplayView d = GetDisplay(params[0]);
			ReplyCommand(cmd, d.Width, d.Height);
			break;

		default:
			Log.d("AROSDisplay", "Unknown command " + cmd);
			
			ReplyCommand(DisplayServer.cmd_Nak, cmd);
			break;
		}
	}

	/* *** Callbacks follow *** */
	
    // This callback actually launches the bootstrap.
    // It is implemented as a nested class because it has to implement
    // Runnable interface.
	class Booter implements Runnable
	{
		public void run()
		{
	        // Get external storage path 
	        String extdir = Environment.getExternalStorageDirectory().getAbsolutePath();
	        String arosdir = extdir + "/AROS";
	        FileDescriptor readfd = new FileDescriptor();
	        FileDescriptor writefd = new FileDescriptor();

	        Log.d("AROS", "Starting AROS, root path: " + arosdir);

	        int rc = Start(arosdir, readfd, writefd);

	        if (rc == 0)
	        {
	        	FileInputStream displaypipe = new FileInputStream(readfd);
	        	FileOutputStream inputpipe = new FileOutputStream(writefd);

	        	AROSBootstrap.this.InputPipe = new DataOutputStream(inputpipe);
	        	DisplayServer srv = new DisplayServer(AROSBootstrap.this, displaypipe);

	        	srv.start();
	        }
		}
	}

	// Declaration of AROSBootstrap C code entry point
    private native int Start(String dir, FileDescriptor rfd, FileDescriptor wfd);

    // This orders processing of a command from server
    class ServerCommand implements Runnable
    {
    	private int Command;
    	private int[] Parameters;

    	public ServerCommand(int cmd, int... params)
    	{
    		Command = cmd;
    		Parameters = params;
    	}
  
    	public void run()
    	{
    		try
    		{
				AROSBootstrap.this.DoCommand(Command, Parameters);
			}
    		catch (IOException e)
    		{
				Log.d("AROS", "Failed to write data to pipe");
				System.exit(0);
			}
    	}
    }
    
    private CharSequence errStr;
    private DisplayView rootView;
    private DataOutputStream InputPipe;
}

// This is our display class
class DisplayView extends ViewGroup
{
	public DisplayView(Context context, AROSBootstrap.Booter callback)
	{
		super(context);

		Width  = 0;
		Height = 0;
		booter = callback;
	}

	public BitmapView NewBitMap(int width, int height)
	{
		BitmapView bm = new BitmapView(null, width, height);

		addView(bm);
		return bm;
	}

	@Override
	protected void onLayout(boolean c, int left, int top, int right, int bottom)
	{
		if (!c)
			return;

		Width = right - left;
		Height = bottom - top;
		Log.d("AROS", "Screen size set: " + Width + "x" + Height);

		// Just in case - run bootstrap only once after we are created
		if (booter == null)
			return;

		// Order to run bootstrap on next input loop run
		// (after layout is complete)
		Handler h = new Handler();
		h.post(booter);

		// Next time we won't run the native bootstrap
		booter = null;
	}

	// Display size - for AROS driver
	public int Width;
	public int Height;

    private AROSBootstrap.Booter booter;
}

// This is our bitmap class
class BitmapView extends View
{
	public BitmapView(Context context, int w, int h)
	{
		super(context);

		width  = w;
		height = h;
		Data   = new int[w * h];
	}

	@Override
	protected void onDraw(Canvas c)
	{	
		c.drawBitmap(Data, 0, width, 0, 0, width, height, false, null);
	}

	public int[] Data;
	private int width, height;
}
