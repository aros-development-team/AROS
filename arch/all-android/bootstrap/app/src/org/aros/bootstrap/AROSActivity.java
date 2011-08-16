package org.aros.bootstrap;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Canvas;
import android.os.Bundle;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import java.lang.String;

public class AROSActivity extends Activity
{
	static final int ID_ERROR_DIALOG = 0;
	static final int ID_ALERT_DIALOG = 1;

    private CharSequence errStr;
    private DisplayView rootView;
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
    	Log.d("AROS.UI", "Activity created");

    	AROSBootstrap app = (AROSBootstrap) getApplication();
    	rootView = new DisplayView(this, app);
    	super.onCreate(savedInstanceState);
        setContentView(rootView);

    	// Notify application object about our creation
    	app.ui = this;
    }

    @Override
    public void onDestroy()
    {
    	Log.d("AROS.UI", "Activity destroyed");
    	
    	AROSBootstrap app = (AROSBootstrap) getApplication();
    	app.ui = null;
    	
    	super.onDestroy();
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

	// This is for far future. Android already supports TV out,
	// just it always displays the same picture as device's screen.
	// But what if things change one day?
	// 'id' parameter is reserved to identify a particular display.
	public DisplayView GetDisplay(int id)
	{
		return rootView;
	}
}

// This is our display class
class DisplayView extends ViewGroup
{
	private AROSBootstrap main;

	public DisplayView(Context context, AROSBootstrap app)
	{
		super(context);

		Width  = 0;
		Height = 0;
		main = app;
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

		main.Boot();
	}

	// Display size - for AROS driver
	public int Width;
	public int Height;
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
