package org.aros.bootstrap;

import java.io.FileDescriptor;
import android.app.Application;
import android.content.Intent;
import android.os.Environment;
import android.util.Log;

public class AROSBootstrap extends Application
{
	public AROSActivity ui;
	private DisplayServer Server;
	private Boolean started;
	private int DisplayWidth;
	private int DisplayHeight;

	// Commands sent to us by AROS display driver
	static final int cmd_Query =  1;
	static final int cmd_Nak   = -1;
	
	@Override
	public void onCreate()
	{
        Log.d("AROS", "Started");
        started = false;
	}

	@Override
	public void onLowMemory()
	{
		Log.d("AROS", "Memory panic");
		// TODO: Here we may send a command to display driver to flush libraries
	}
	
	public void Boot()
	{
		// We get here after the activity is laid out and we know screen size.
		// However we need to actually run AROS only once, when the application is
		// first started. The activity may be flushed by Android OS if it's hidden,
		// in this case it will be recreated from scratch.
		if (started)
		{
			Log.d("AROS", "Already running");
			return;
		}

		// Cache information about our display. It needs to be persistent,
		// since theoretically we may need it while activity is flushed.
		// Remember that AROS is running out of DalvikVM's control.
		DisplayView d = ui.GetDisplay(0);
		DisplayWidth  = d.Width;
		DisplayHeight = d.Height;

	    // Get external storage path 
	    String extdir = Environment.getExternalStorageDirectory().getAbsolutePath();
	    String arosdir = extdir + "/AROS";
	    FileDescriptor readfd = new FileDescriptor();
	    FileDescriptor writefd = new FileDescriptor();

	    Log.d("AROS", "Starting AROS, root path: " + arosdir);

	    int rc = Start(arosdir, readfd, writefd);

	    if (rc == 0)
	    {
	    	started = true;

	      	// Initialize and run display server thread
	        AROSBootstrap.this.Server = new DisplayServer(AROSBootstrap.this, readfd, writefd);
	        AROSBootstrap.this.Server.start();
		}
	}

    public void DisplayError(String text)
    {
    	// Make sure the activity is visible
    	Intent myIntent = new Intent(this, AROSActivity.class);
    	startActivity(myIntent);

    	// Now display the message
    	ui.DisplayError(text);
    }

	private void DoCommand(int cmd, int[] params)
	{
		switch (cmd)
		{
		case cmd_Query:
			Log.d("AROS", "cmd_Query(" + params[0] + ")");

			Server.ReplyCommand(cmd, DisplayWidth, DisplayHeight);
			break;

		default:
			Log.d("AROSDisplay", "Unknown command " + cmd);

			Server.ReplyCommand(cmd_Nak, cmd);
			break;
		}
	}
	
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
    		// It's tedious to type AROSBootstrap.this. every time,
    		// so i simply forward the event to outer class.
			AROSBootstrap.this.DoCommand(Command, Parameters);
    	}
    }

    static
    {
        System.loadLibrary("AROSBootstrap");
    }
	
	// Declaration of AROSBootstrap C code entry point
    private native int Start(String dir, FileDescriptor rfd, FileDescriptor wfd);
}
