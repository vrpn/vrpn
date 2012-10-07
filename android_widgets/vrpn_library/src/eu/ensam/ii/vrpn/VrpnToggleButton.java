package eu.ensam.ii.vrpn;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;

/**
 * A {@link android.view.ToggleButton ToggleButton} connected to a Vrpn Button.
 * 
 * <p>
 * This button sends a true update when its status changes to checked and false
 * when its status changes to unchecked. Unlike a
 * {@link VrpnPressButton}, the status of this widget changes when it is
 * pressed, but not when it is released, 
 * </p>
 * 
 * <p>
 * In your layout XML file, include the following into a container widget :
 * <pre>
 * &lt;eu.ensam.ii.vrpn.VrpnButton
 *   app:vrpnButton="@id/MyButton" 
 * /&gt;
 * </pre>
 * <p>
 * The root element of the layout must declare the following line just after the
 * <code>xmlns:android ... </code>line :
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
 */
public class VrpnToggleButton extends android.widget.ToggleButton implements
		OnCheckedChangeListener {
	private static final String TAG = "VrpnToggleButton";
	private int _vrpnButtonId;

	public VrpnToggleButton(Context context) {
		super(context);
		init(null);
	}

	public VrpnToggleButton(Context context, AttributeSet attrs) {
		super(context, attrs);
		init(attrs);
	}

	public VrpnToggleButton(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		init(attrs);
	}

	private void init(AttributeSet attrs) {
		setOnCheckedChangeListener(this);

		// get The Vrpn button id from the xml app:vrpnButton attribute
		TypedArray a = getContext().obtainStyledAttributes(attrs,
				R.styleable.VrpnRadioButton);
		_vrpnButtonId = a.getInt(R.styleable.VrpnToggleButton_vrpnButton, 0);
		a.recycle();

		// Trigger an initial Vrpn update
		Log.d(TAG, "ctor init " + _vrpnButtonId + " : " + isChecked());
		VrpnClient.getInstance().sendButton(_vrpnButtonId, isChecked());
	}

	@Override
	public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
		Log.d(TAG, "onCheckedChanged " + _vrpnButtonId + " : " + isChecked);
		VrpnClient.getInstance().sendButton(_vrpnButtonId,
				buttonView.isChecked());
	}
}
