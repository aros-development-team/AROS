package org.aros.bootstrap;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.RelativeLayout;

import java.lang.String;

public class AROSActivity extends Activity
{
	static final int ID_ERROR_DIALOG = 0;
	static final int ID_ALERT_DIALOG = 1;

	static final int AT_DeadEnd = 0x80000000;

    private CharSequence errStr;
    private int alertCode;
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
		AROSBootstrap app = (AROSBootstrap) getApplication();

    	Log.d("AROS.UI", "Activity destroyed");

    	// Notify application about our death only if we are current activity.
    	// This is needed for correct activity instance switch (between PortraitActivity
    	// and LandscapeActivity). The activity isn't finished instantly when finish() is
    	//called on it, instead, it's finished with delay, after another activity takes its place.
    	if (app.ui == this)
    		app.ui = null;

    	super.onDestroy();
    }

    public void DisplayError(String text)
    {
    	errStr = text;
    	showDialog(ID_ERROR_DIALOG);
    }

    public void DisplayAlert(int code, String text)
    {
    	alertCode = code;
    	errStr = text;
    	showDialog(ID_ALERT_DIALOG);
    }
  
    public void Show(int id, BitmapData data)
    {
    	rootView.Show(data);
    }

    public BitmapView GetBitmap(int id)
    {
    	return rootView.GetBitmap(id);
    }

    public Dialog onCreateDialog(int id)
    {
		AlertDialog.Builder b = new AlertDialog.Builder(this);
		DialogInterface.OnClickListener okEvent;
		int okTitle;
 
    	switch (id)
    	{
    	case ID_ERROR_DIALOG:
    		b.setTitle(R.string.error);
    		okEvent = new DialogInterface.OnClickListener()
        	{	
    			public void onClick(DialogInterface dialog, int which)
    			{
    				System.exit(0);
    			}
    		};
    		
        	okTitle = R.string.ok;
    		break;

    	case ID_ALERT_DIALOG:
    		b.setTitle(R.string.guru);

    		okEvent = new DialogInterface.OnClickListener()
    		{
    			public void onClick(DialogInterface dialog, int which)
    			{
    				AROSBootstrap app = (AROSBootstrap) getApplication();

    				app.Resume();
   				}
   			};

   			if ((alertCode & AT_DeadEnd) != 0)
   			{   				
   				DialogInterface.OnClickListener cancelEvent = new DialogInterface.OnClickListener()
   	    		{
   	    			public void onClick(DialogInterface dialog, int which)
   	    			{
   	    				AROSBootstrap app = (AROSBootstrap) getApplication();

   	    				app.Quit();
   	   				}
   	   			};

   	        	b.setNegativeButton(R.string.shutdown, cancelEvent);
   	        	okTitle = R.string.reboot;
    		}
   			else
   			{
   				okTitle = R.string.resume;
   			}
    		break;

    	default:
    		return null;
    		
    	}

    	b.setMessage(errStr);
    	b.setCancelable(false);
    	b.setPositiveButton(okTitle, okEvent);
    	
    	return b.create();
    }
}

// This is our display class
class DisplayView extends RelativeLayout
{
	private AROSBootstrap main;
	private BitmapView bitmap;
	int LastPointerX      = -1;
	int LastPointerY      = -1;
	int LastPointerAction = -1;
	int LastKey			  = -1;

	static final int IECODE_UP_PREFIX = 0x80;

	public DisplayView(Context context, AROSBootstrap app)
	{
		super(context);
		main = app;

		// The bitmap view here magically appears to have the same size as DisplayView
		// and positioned at (0, 0). Thanks to RelativeLayout.
		bitmap = new BitmapView(context, app);
		addView(bitmap);

		setId(0);
		setFocusable(true);
		setFocusableInTouchMode(true);
		requestFocus();
	}

	public void Show(BitmapData data)
	{
		bitmap.Show(data);
	}

	public BitmapView GetBitmap(int id)
	{
		return bitmap;
	}

	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh)
	{
		super.onSizeChanged(w, h, oldw, oldh);
	
		if (!main.started)
		{
			// We get here only once, before AROS is booted up.
			// Here we need to detect three constants:
			// - Total screen width
			// - Total screen height
			// - Size of screen titlebar (Android's one, do not confuse with AROS one)
			// These three constants allow AROS to determine screen size in both portrait
			// and landscape variants. This allows us to represent two orientations as two different
			// screenmodes
			WindowManager wm = (WindowManager)main.getSystemService(Context.WINDOW_SERVICE);
			Display d = wm.getDefaultDisplay(); 
			int displayWidth  = d.getWidth();
			int displayHeight = d.getHeight();
			int o = getResources().getConfiguration().orientation;

			// Calculate titlebar size.
			// This assumes that there are no other system's own elements on the screen except titlebar.
			// Currently this always seems to be true.
			// The second assumption is that titlebar has the same height in both orientations.
			main.TitlebarSize  = displayHeight - h;
			main.DisplayWidth  = displayWidth;
			main.DisplayHeight = displayHeight;
			main.Orientation   = o;

			Log.d("AROS.Boot", "Screen size: " + displayWidth + "x" + displayHeight +
				  ", Titlebar size: "+ main.TitlebarSize + ", Orientation: " + o);

			// Start up AROS
			main.ColdBoot();
		}
	}

	@Override
	public boolean onTrackballEvent(MotionEvent e)
	{
		// Trackball reports "normalized" offsets. According to a documentation,
		// 1.0 stands for a single DPAD key press, and "trackball may report more fine-graned values"
		// Since we don't know what's "more fine-graned", and there seem to be no exact definition
		// of these units, we just take a sign of offsets.
		int x = (int)Math.signum(e.getX());
		int y = (int)Math.signum(e.getY());
		int action = e.getAction();

//		Log.v("AROS.Input", "Trackball action " + action + " at (" + e.getX() + ", " + e.getY() + ")");

		main.ReportMouse(x, y, action);
		return true;
	}

	@Override
	public boolean onTouchEvent(MotionEvent e)
	{
		int x = (int)e.getX();
		int y = (int)e.getY();
		int action = e.getAction();

		// This filters out small touchscreen jitter. Since we round up coordinates to
		// integers, we can get repeating actions, since the actual coordinates will differ by
		// small fraction.
		if ((x != LastPointerX) || (y != LastPointerY) || (action != LastPointerAction))
		{
			LastPointerX      = x;
			LastPointerY      = y;
			LastPointerAction = action;

//			Log.v("AROS.Input", "Touch action " + action + " at (" + x + ", " + y + ")");
			main.ReportTouch(x, y, action);
		}

		return true;
	}

	@Override
	public boolean onKeyDown(int code, KeyEvent e)
	{
		// Android autorepeats keys if held down. Here we suppress this.
		if (code != LastKey)
		{
//			Log.v("AROS.Input", "KeyDown " + code);

			LastKey = code;
			main.ReportKey(code, 0);
		}
		return true;
	}

	@Override
	public boolean onKeyUp(int code, KeyEvent e)
	{
//		Log.v("AROS.Input", "KeyUp " + code);

		if (LastKey == -1)
		{
			// XPeria quirk: Backspace doesn't generate KeyDown event, only KeyUp
			main.ReportKey(code, 0);
		}
		else
			LastKey = -1;

		main.ReportKey(code, IECODE_UP_PREFIX);

		return true;
	}
}

// This is our bitmap class
class BitmapView extends View
{
	private AROSBootstrap main;
	private Bitmap pixbuf = null;

	public BitmapView(Context context, AROSBootstrap app)
	{
		super(context);
		main = app;

		BitmapData bm = app.Bitmap;
		Show(bm);
		if (pixbuf != null)
		{
			// The activity has been recreated after the program was running
			// in the background for a long time.
			// Make sure we have the recent bitmap data.
			main.UpdateBitmap(pixbuf, bm, 0, 0, bm.Width, bm.Height);
		}
	}

	public void Show(BitmapData bm)
	{
		if (bm == null)
		{
//			Log.d("AROS.UI", "Show(NULL)");
			// Drop our bitmap object
			pixbuf = null;
			// Enforce a redraw. AROS will not call Update() here.
			invalidate();
		}
		else
		{
			Log.d("AROS.UI", "Show (" + bm.Width + " x " + bm.Height + ")");
			pixbuf = Bitmap.createBitmap(bm.Width, bm.Height, Bitmap.Config.ARGB_8888);

			// Explicitly disable alpha channel on the bitmap.
			// AROS provides us with a == 0 in pixel data. On some devices it was reported to produce blank screen.
			// setHasAlpha() is declared public since API level 12, however it seems to be present, but private,
			// in earlier versions too. Just in case, it's surrounded by try...catch.
			try
			{
				pixbuf.setHasAlpha(false);
			}
			catch (Exception e)
			{
				Log.d("AROS.UI", "setHasAlpha() not supported");
			}

			// Sometimes screen invalidation happens between this and first Update method call.
			// This causes bad visual effect (black screen flashes for a moment.
			// Here we instantly get initial contents to prevent this.
			main.UpdateBitmap(pixbuf, bm, 0, 0, bm.Width, bm.Height);
		}
	}

	public void Update(BitmapData bm, int x, int y, int width, int height)
	{
//		Log.d("AROS.UI", "Update (" + x + ", " + y + ", " + width + ", " + height + ")");

		// Copy a portion from AROS memory to our bitmap object
		// Unfortunately we can't create a direct bitmap placed at
		// a specified memory location, so we can't get around this copy.
		// Using int[] is even worse, we also can't create a direct int[].
		// Additionally, using int[] proved to be very slow
		main.UpdateBitmap(pixbuf, bm, x, y, width, height);
		invalidate(x, y, x + width, y + height);
	}

	@Override
	protected void onDraw(Canvas c)
	{
//		Log.d("AROS.UI", "onDraw( " + pixbuf +" )");
		if (pixbuf == null)
		{
			c.drawColor(0);
		}
		else
		{
			c.drawBitmap(pixbuf, 0, 0, null);
		}
	}
}
