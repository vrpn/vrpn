package eu.ensam.ii.vrpn;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;

/**
 * This {@link android.view.SurfaceView SurfaceView} updates two Vrpn Analog channels with the coordinates of the
 * Surface point that is touched.
 * 
 * <p>
 * Updates are sent as long astouch events occur within the surface. The
 * X channel range from
 * <code>app:leftX</code> to <code>app:rightX</code>,
 * and the Y channel values range from <code>app:topY</code> to
 * <code>app:bottomY</code>.
 * </p>
 * 
 * <p>
 * In your layout, declare the following (note the <code>app:</code> attributes)
 * </p>
 * 
 * <pre>
 * &lt;eu.ensam.ii.vrpn.VrpnSurface 
 *   android:layout_width="fill_parent" 
 *   android:layout_height="fill_parent"
 *   app:leftX="-1"
 *   app:rightX="1"
 *   app:topY="0"
 *   app:bottomY="1"
 *   app:vrpnAnalogX="@id/MyVrpnAnalogX"
 *   app:vrpnAnalogY="@id/MyVrpnAnalogY"
 * /&gt;
 * </pre>
 * 
 * <p>
 * 
 * <p>
 * The root element of the layout must declare the following line just after the
 * <code>xmlns:android ... </code>line :
 * <pre>
 *   xmlns:app="http://schemas.android.com/apk/res/your.package.name.here"
 * </pre>
 * </p>
 
 * <p>
 * <strong>Custom XML attributes :</strong>
 * </p>
 * <ul>
 * <li><code>app:vrpnAnalogX</code> : the id of the Vrpn analog that will
 * receive the X position of the touch event from this widget. The update value
 * sent to the Vrpn analog ranges from the <code>app:leftX</code> value to the
 * <code>app:rightX</code> value.</li>
 * <li><code>app:vrpnAnalogY</code> : the id of the Vrpn analog that will
 * receive the Y position of the touch event from this widget. The update value
 * sent to the Vrpn analog ranges from the <code>app:bottomY</code> value to the
 * <code>app:topX</code> value.</li>
 * <li><code>app:leftX</code> : the numeric value mapped onto the left border of
 * the surface.</li>
 * <li><code>app:rightX</code> : the numeric value mapped onto the right border
 * of the surface. May be greater or lower than <code>app:leftX</code>.</li>
 * <li><code>app:bottomY</code> : the numeric value mapped onto the top border
 * of the surface</li>
 * <li><code>app:topY</code> : the numeric value mapped onto the top border of
 * the surface. May be greater or lower than <code>app:bottomY</code></li>
 * </ul>
 * 
 * The <code>app:vrpnAnalogX</code> and <code>app:vrpnAnalogY</code> ids can be
 * literals or a resource Ids. These resource ids may be kept in a <a
 * href="package-summary.html#vrpnids">separate Vrpn Id list</a>
 * 
 */
public class VrpnSurface extends android.view.SurfaceView implements
		OnTouchListener {
	protected String TAG = "VrpnSurface";
	int _vrpnAnalogXId = 0;
	int _vrpnAnalogYId = 0;

	float _leftX, _rightX, _topY, _bottomY;

	public VrpnSurface(Context context) {
		super(context);
		init(null);
	}

	public VrpnSurface(Context context, AttributeSet attrs) {
		super(context, attrs);
		init(attrs);
	}

	public VrpnSurface(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		init(attrs);
	}

	/**
	 * Perform initialization that is common to the constructors
	 * 
	 * @param attrs
	 */
	private void init(AttributeSet attrs) {
		setOnTouchListener(this);

		// get The Vrpn button id from the xml app:vrpnButton attribute
		TypedArray a = getContext().obtainStyledAttributes(attrs,
				R.styleable.VrpnSurface);
		_vrpnAnalogXId = a.getInt(R.styleable.VrpnSurface_vrpnAnalogX, 0);
		_vrpnAnalogYId = a.getInt(R.styleable.VrpnSurface_vrpnAnalogY, 0);
		_leftX = a.getFloat(R.styleable.VrpnSurface_leftX, 0.0f);
		_rightX = a.getFloat(R.styleable.VrpnSurface_rightX, 1.0f);
		_topY = a.getFloat(R.styleable.VrpnSurface_topY, 0.0f);
		_bottomY = a.getFloat(R.styleable.VrpnSurface_bottomY, 1.0f);
		a.recycle();

		// Start at origin
		// TODO start at the middle
		Log.d(TAG, "ctor init :  " + _vrpnAnalogXId + " " + _vrpnAnalogYId);
		VrpnClient.getInstance().sendAnalog(_vrpnAnalogXId, 0);
		VrpnClient.getInstance().sendAnalog(_vrpnAnalogYId, 0);
	}

	/**
	 * Called when a touch event is dispatched to this widget.
	 * 
	 * <p>
	 * If the touch event occurs within the surface of the view, two analog Vrpn
	 * updates are send for the X and y position. Updates are sent when
	 * ACTION_DOWN, ACTION_MOVE, and ACTION_UP are received. the event history
	 * is ignored.
	 * </p>
	 * 
	 * @see android.view.View.OnTouchListener#onTouch(View, MotionEvent)
	 * @see android.view.MotionEvent
	 */
	@Override
	public boolean onTouch(View v, MotionEvent event) {
		final int action = event.getAction();
		if (action == MotionEvent.ACTION_OUTSIDE) {
			return false;
		}
		final float x = event.getX();
		final float y = event.getY();
		final float w = getWidth();
		final float h = getHeight();

		if (x < 0 || x > w || y < 0 || y > h) {
			// dragging the touch point may trigger values outside the view
			// dimensions
			return false;
		}

		/*
		 * event reports absolute View X/Y coordinates, convert to the
		 * appropriate bounds then send the updates
		 */
		final float vrpnX = (x / w) * (_rightX - _leftX) + _leftX;
		final float vrpnY = (y / h) * (_bottomY - _topY) + _topY;
		Log.d(TAG, "touch action x/y = " + action + " " + vrpnX + "/" + vrpnY);
		VrpnClient.getInstance().sendAnalog(_vrpnAnalogXId, vrpnX);
		VrpnClient.getInstance().sendAnalog(_vrpnAnalogYId, vrpnY);
		return true;
	}

}
