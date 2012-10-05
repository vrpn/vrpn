package eu.ensam.ii.vrpn;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

/**
 * A {@link android.widget.Button Button} connected to a VRPN Button.
 * 
 * <p>
 * This button is linked to a Vrpn button. It sends sends a <code>true</code>
 * update when its status changes to checked and a <code>false</code> update
 * when its status changes to unchecked. The status of the VrpnPressButton
 * changes when it is pushed, and also when it is released, unlike a
 * {@link VrpnToggleButton}.
 * </p>
 * 
 * <p>
 * In your <code>res/layout.xml</code>, include the widget in
 * its parent layout :
 * </p>
 * <pre>
 * &lt;eu.ensam.ii.VrpnPressButton
 *   android:layout_width="fill_parent" 
 *   android:layout_height="wrap_content"
 *   app:vrpnButton="@id/MyButton"
 * &#47&gt;
 * </pre>
 * 
 * <p>
 * The root element of the layout must declare the following line just after the
 * <code>xmlns:android ... </code>line :
 * 
 * <pre>
 *   xmlns:app="http://schemas.android.com/apk/res/your.package.name.here"
 * </pre>
 * 
 * </p>
 * 
 * <p>
 * <strong>Custom XML attributes :</strong>
 * </p>
 * <ul>
 * <li><code>app:vrpnButton</code> : the id of the Vrpn button that will receive
 * updated from this widget. This value can be a literal or a resource Id that
 * can be kept in a <a href="package-summary.html#vrpnids">separate Vrpn Id
 * list</a></li>
 * </ul>
 * 
 */
public class VrpnPressButton extends android.widget.Button implements
		android.view.View.OnTouchListener {
	private String TAG = "VrpnPressButton";
	private int _vrpnButtonId = 0;

	public VrpnPressButton(Context context) {
		super(context);
		init(null);
	}

	public VrpnPressButton(Context context, AttributeSet attrs) {
		super(context, attrs);
		init(attrs);
	}

	public VrpnPressButton(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		init(attrs);
	}

	/**
	 * Widget initialization
	 * 
	 * @param attrs
	 *            the AttributeSet that is required to access to the XML custom
	 *            attributes
	 */
	private void init(AttributeSet attrs) {
		// this.onTouch() will be called when this widget is touched
		setOnTouchListener(this);

		/*
		 * Get The Vrpn button id from the xml app:vrpnButton attribute The
		 * custom attribute is declared is in res/values/attrs.xml
		 */
		TypedArray a = getContext().obtainStyledAttributes(attrs,
				R.styleable.VrpnRadioButton);
		_vrpnButtonId = a.getInt(R.styleable.VrpnRadioButton_vrpnButton, 0);
		a.recycle();

		// Send a "false" since a PressButton is always initially released
		Log.d(TAG, "ctor init :  button " + _vrpnButtonId + " released ");
		VrpnClient.getInstance().sendButton(_vrpnButtonId, false);
	}

	/**
	 * Called when a touch event is dispatched to this VrpnPressButton.
	 * 
	 * This method sends a <code>true</code> update when the button is pressed
	 * and a <code>false</code> update when the button is released. when it is
	 * released. No event is send while the button is pressed.
	 * 
	 * @see android.view.View.OnTouchListener#onTouch(android.view.View,
	 *      android.view.MotionEvent)
	 */
	@Override
	public boolean onTouch(View v, MotionEvent event) {
		final int action = event.getAction();
		/*
		 * Avoid unnecessary processing for irrelevant events Nothing is sent
		 * while the button is hold down (event=ACTION_MOVE)
		 */
		if (action != MotionEvent.ACTION_DOWN
				&& action != MotionEvent.ACTION_UP
				&& action != MotionEvent.ACTION_CANCEL) {
			// Mark the event has handled, so it is not be handled by
			// ToggleButton::onClick
			return true;
		}

		/*
		 * Send true when the button is pressed and false when it is released.
		 */
		switch (event.getAction()) {
		case MotionEvent.ACTION_DOWN:
			Log.d(TAG, " button " + _vrpnButtonId + " pressed ");
			VrpnClient.getInstance().sendButton(_vrpnButtonId, true);
			break;
		case MotionEvent.ACTION_UP:
		case MotionEvent.ACTION_CANCEL:
			Log.d(TAG, " button " + _vrpnButtonId + " released ");
			VrpnClient.getInstance().sendButton(_vrpnButtonId, false);
			break;
		default:
			break;
		}
		// Mark the event has handled, so it is not be handled by
		// ToggleButton::onClick
		return false;
	}

}
