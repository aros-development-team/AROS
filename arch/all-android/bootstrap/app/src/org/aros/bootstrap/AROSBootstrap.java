package org.aros.bootstrap;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.ViewGroup;

import java.lang.String;

public class AROSBootstrap extends Activity
{
	static final int ID_ERROR_DIALOG = 0;
	static final int ID_ALERT_DIALOG = 1;

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
    }

    // This callback actually launches the bootstrap.
    // It is implemented as a nested class because it has to implement
    // Runnable interface.
	class Booter implements Runnable
	{
		public void run()
		{
	        // Get external storage path 
	        String extdir = Environment.getExternalStorageDirectory().getAbsolutePath();
	        Log.d("AROS", "Starting AROS, external storage is: " + extdir);

	        int rc = Start(extdir + "/AROS");

	        DisplayError("Bootstrap exited with rc" + rc);
		}
	}
	
    public native int Start(String dir);
	
    private CharSequence errStr;
    private DisplayView rootView;
}

class DisplayView extends ViewGroup
{
	public DisplayView(Context context, AROSBootstrap.Booter callback)
	{
		super(context);

		width  = 0;
		height = 0;
		booter = callback; 
	}

	@Override 
	public void onLayout(boolean c, int left, int top, int right, int bottom)
	{
		if (!c)
			return;
		
		width = right - left;
		height = bottom - top;
		Log.d("AROS", "Screen size set: " + width + "x" + height);

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

	public int width, height;
    private AROSBootstrap.Booter booter;
}
