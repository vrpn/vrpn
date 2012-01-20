package edu.unc.cs.vrpn;

import jni.JniLayer;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;

/**
 * Button event listener.
 * 
 * @author Ted
 * 
 */
public class ButtonListener implements OnTouchListener, OnClickListener
{
	private JniBuilder.Button button;
	private int buttonNumber;
	private JniLayer jniLayer;

	/**
	 * Creates a new instance of the {@link ButtonListener} class, connecting to a given
	 * button number in the JNI layer
	 * 
	 * @param buttonNumber - the number that the JNI layer will pass to VRPN
	 * @param jniCallback - the JNI Layer which will connect to VRPN clients
	 */
	public ButtonListener(int buttonNumber, JniLayer jniCallback)
	{
		this(null , buttonNumber , jniCallback);
	}

	public ButtonListener(JniBuilder.Button button, int buttonNumber, JniLayer jniCallback)
	{
		this.button = button;
		this.buttonNumber = buttonNumber;
		this.jniLayer = jniCallback;
	}

	@Override
	public void onClick(View v)
	{
		Log.d(Main.TAG , (this.button != null ? this.button.getName() + " (button " : "Button (")
				+ this.buttonNumber + ") clicked");
	}

	@Override
	public boolean onTouch(View view, MotionEvent event)
	{
		switch (event.getAction()) {
		case MotionEvent.ACTION_DOWN:
			this.jniLayer.updateButtonStatus(this.buttonNumber , true);
			break;
		case MotionEvent.ACTION_UP:
			this.jniLayer.updateButtonStatus(this.buttonNumber , false);
			break;
		}
		return false;
	}
}
