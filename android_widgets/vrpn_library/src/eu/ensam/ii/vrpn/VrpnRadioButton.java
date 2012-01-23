package eu.ensam.ii.vrpn;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.CompoundButton;
import android.widget.RadioGroup;
import android.widget.CompoundButton.OnCheckedChangeListener;

/**
 * A {@link android.widget.RadioButton RadioButton} that updates a Vrpn Button channel.
 * 
 * <p>
 * The <code>VrpnRadioButton</code> is to be included inside a {@link RadioGroup}. The
 * <code>VrpnRadioButton</code> will send a <code>true</code> when its state changes to
 * checked and a <code> false</code> update when its state changes to unchecked.
 * Checking another button of the <code>RadioGroup</code> will uncheck the <code>RadioButton</code>.</p>
 * <p>in a <code>RadioGroup</code>, it is possible to include :
 * <ul>
 * <li>a <code>VrpnRadiobutton</code> and a <code>RadioButton</code>. Checking the <code>RadioButton</code> will uncheck the <code>VrpnRadioButton</code> that will send an update</li>
 * <li>several <code>VrpnRadioButton</code> widgets. In this case the checked and the unchecked widgets will send updates</li>
 * </ul> 
 * </p>
 * <p>
 * In your layout, declare the following :
 * </p>
 * 
 * <pre>
 * &lt;RadioGroup 
 *   android:id="@+id/RadioGroup01" 
 *   &gt; 
 *   &lt;eu.ensam.ii.vrpn.VrpnRadioButton 
 *     android:text="This way" 
 *     android:checked="true"
 *     app:vrpnButton="@id/VrpnButtonThisWay"	
 *   /&gt;
 *   &lt;RadioButton
 *     android:text="other Way"  
 *     android:checked="false"
 *   /&gt;	
 * &lt;/RadioGroup&gt;
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
 */

public class VrpnRadioButton extends android.widget.RadioButton implements
		OnCheckedChangeListener {
	private static final String TAG = "VrpnRadioButton";
	int _vrpnButtonId = 0;

	public VrpnRadioButton(Context context) {
		super(context);
		init(null);
	}

	public VrpnRadioButton(Context context, AttributeSet attrs) {
		super(context, attrs);
		init(attrs);
	}

	public VrpnRadioButton(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		init(attrs);
	}

	private void init(AttributeSet attrs) {
		setOnCheckedChangeListener(this);

		// get The Vrpn button id from the xml app:vrpnButton attribute
		TypedArray a = getContext().obtainStyledAttributes(attrs,
				R.styleable.VrpnRadioButton);
		_vrpnButtonId = a.getInt(R.styleable.VrpnRadioButton_vrpnButton, 0);
		a.recycle();

		// Trigger an initial Vrpn update
		Log.d(TAG, "ctor init " + _vrpnButtonId + " : " + isChecked());
		VrpnClient.getInstance().sendButton(_vrpnButtonId, isChecked());
	}

	@Override
	public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
		Log.d(TAG, "onCheckChanged " + _vrpnButtonId + " : " + isChecked);
		// Send the update through the Vrpn service
		VrpnClient.getInstance().sendButton(_vrpnButtonId, isChecked());

	}
}
