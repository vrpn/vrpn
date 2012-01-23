package eu.ensam.ii.vrpn;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;

/**
 * A {@link android.widget.Spinner Spinner} that updates a Vrpn Analog channel.
 * 
 * <p>
 * Picking a value from the spinner will update a Vrpn Analog channel with the
 * position of the item that was picked in the item list. On the application side, the value will be delivered as a <em>double</em>.
 * </p>
 * <p>
 * In your layout, declare the following :
 * </p>
 * 
 * <pre>
 * &lt;eu.ensam.ii.vrpn.VrpnSpinner 
 *   android:layout_width="fill_parent" 
 *   android:layout_height="wrap_content"
 *   android:prompt="@string/the_prompt"
 *   android:entries="@array/the_array"
 *   app:vrpnAnalog="@id/VrpnAnalogMySpinner"
 *   /&gt;
 * </pre>
 * 
 * In a resource XML file located in <em>res/values</em>, define the prompt that
 * will be displayed in the choice dialog, as well as the list of values to choose from.</p><
 * 
 * <pre>
 * &lt;string name="the_prompt"&gt;Message displayed when the choice dialog is opened&lt;/string&gt;    
 * &lt;string-array name="the_array"&gt;
 *   &lt;item&gt;"Mars"&lt;/item&gt;
 *   &lt;item&gt;"Venus"&lt;/item&gt;
 * &lt;/string-array&gt;
 * </pre>
 * <p>
 * The root element of the layout must declare the following line just after the
 * <code>xmlns:android ... </code>line :</p>
 * <pre>
 *   xmlns:app="http://schemas.android.com/apk/res/your.package.name.here"
 * </pre>
 * <p>
 * <strong>Custom XML attributes :</strong>
 * </p>
 * <ul>
 * <li><code>app:vrpnAnalog</code> : the id of the Vrpn analog that will receive
 * updated from this widget. This value can be a literal or a resource Id that
 * can be kept in a <a href="package-summary.html#vrpnids">separate Vrpn Id
 * list</a></li>
 * </ul>
 */
public class VrpnSpinner extends android.widget.Spinner implements OnItemSelectedListener {

	private static final String TAG = "VrpnSpinner";

	private int _vrpnAnalogId;

	public VrpnSpinner(Context context) {
		super(context);
		init(null);
	}

	public VrpnSpinner(Context context, AttributeSet attrs) {
		super(context, attrs);
		init(attrs);
	}

	private void init(AttributeSet attrs) {
		// Get the custom attribute values
		TypedArray a = getContext().obtainStyledAttributes(attrs,
				R.styleable.VrpnSpinner);
		_vrpnAnalogId = a.getInt(R.styleable.VrpnSpinner_vrpnAnalog, 0);
		a.recycle();

		setOnItemSelectedListener(this);
		setSelection(0);

		// Trigger a Vrpn update to get a known initial state
		Log.d(TAG, "ctor " + _vrpnAnalogId + " position " + 0);
		VrpnClient.getInstance().sendAnalog(_vrpnAnalogId, 0);
	}

	/**
	 * 
	 * @see android.widget.AdapterView.OnItemSelectedListener#onItemSelected(android.widget.AdapterView, android.view.View, int, long)
	 */
	@Override
	public void onItemSelected(AdapterView<?> parent, View view, int position,
			long id) {
		Log.d(TAG, "onItemSelected " + _vrpnAnalogId + " position" + position);
		// Send the vrpn update
		// Note : The line below throws an exception in the Eclipse GUI preview
		VrpnClient.getInstance().sendAnalog(_vrpnAnalogId, position);
	}

	@Override
	public void onNothingSelected(AdapterView<?> arg0) {
	}
}
