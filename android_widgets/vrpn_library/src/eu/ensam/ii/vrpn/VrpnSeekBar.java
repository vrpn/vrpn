package eu.ensam.ii.vrpn;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

/**
 * A {@link android.widget.SeekBar SeekBar} that updates a Vrpn Analog channel.
 * 
 * <p>
 * The seekbar include a line with the min, max and current
 * values of the seekbar, and the seekbar itself.
 * 
 * The seekbar progress is converted into a value in the 
 * <code>app:minValue</code> .. <code>app:maxValue</code>
 *  range. This value is then sent out to the Vrpn Analog channel whose
 * id is specified in the <code>app:vrpnAnalog</code> attribute. The <code>VrpnSeekBar</code>
 * moves in increments that are one hundredth of the value range. 
 * </p>
 * <p>
 * In your layout, declare the following (note the app:attributes)
 * </p>
 * 
 * <pre>
 * &lt;eu.ensam.ii.vrpn.VrpnSeekBar 
 *   android:layout_width="fill_parent"
 *   android:layout_height="wrap_content"
 *   app:minValue="-3"
 *   app:minValue="3"
 *   app:minValue="0" 
 *   app:vrpnAnalog="@id/MyVrpnAnalogId"
 * /&gt;
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
 * <li><code>app:vrpnAnalog</code> : the id of the Vrpn analog that will receive
 * updates from this widget. This value can be a literal or a resource Id that
 * can be kept in a <a href="package-summary.html#vrpnids">separate Vrpn Id
 * list</a></li>
 * <li><code>app:minValue</code> : the numeric value mapped onto the left
 * position of the VrpnSeekbar</li>
 * <li><code>app:maxValue</code> : the numeric value mapped onto the left
 * position of the VrpnSeekbar</li>
 * <li><code>app:defaultValue</code> : the initial numeric value of the VrpnSeekbar;</li>
 * <li><code>app:hideValues</code> : <code>true</code> will cause the line with
 * the minimum, maximum and current values to be hidden, <code>false</code> will cause
 * the line to be displayed (default)</li>
 * </ul>
 * 
 * @author Philippe
 * 
 */
public class VrpnSeekBar extends android.widget.LinearLayout implements
		OnSeekBarChangeListener {
	// Partly adapted from
	// http://www.codeproject.com/KB/android/seekbar_preference.aspx

	private static final String TAG = "VrpnSeekBar";

	private static final float DEFAULT_MIN_VALUE = 0.0f;
	private static final float DEFAULT_MAX_VALUE = 100.0f;

	private float _minValue;
	private float _maxValue;
	private float _currentValue;
	private TextView _currentValueText;
	private CharSequence _title;
	private int _vrpnAnalogId;
	private SeekBar _seekBar;
	private boolean _hideValues;

	public VrpnSeekBar(Context context) {
		this(context, null);
	}

	public VrpnSeekBar(Context context, AttributeSet attrs) {
		super(context, attrs);

		// Get the custom attribute values
		TypedArray a = getContext().obtainStyledAttributes(attrs,
				R.styleable.VrpnSeekBar);
		_minValue = a
				.getFloat(R.styleable.VrpnSeekBar_minValue, DEFAULT_MIN_VALUE);
		_maxValue = a
				.getFloat(R.styleable.VrpnSeekBar_maxValue, DEFAULT_MAX_VALUE);
		_currentValue = a.getFloat(R.styleable.VrpnSeekBar_defaultValue,
				(_maxValue + _minValue) / 2.0f);
		_title = a.getText(R.styleable.VrpnSeekBar_title);
		_vrpnAnalogId = a.getInt(R.styleable.VrpnSeekBar_vrpnAnalog, 0);
		boolean hideTitle = a.getBoolean(R.styleable.VrpnSeekBar_hideTitle, false);
		_hideValues = a.getBoolean(R.styleable.VrpnSeekBar_hideValues, false);
		a.recycle();

		// Load the view layout
		LayoutInflater li = (LayoutInflater) context
				.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		li.inflate(R.layout.vrpn_seekbar, this, true);

		// Load or hide the title
		TextView titleView = (TextView) findViewById(R.id.txtSeekBarTitle);
		if (hideTitle) {
			titleView.setVisibility(GONE);
		} else {
			titleView.setText(_title);
		}

		// Load or hide the layout with the values
		if (_hideValues) {
			((LinearLayout) findViewById(R.id.lytSeekBarValues))
					.setVisibility(GONE);

		} else {
			((TextView) findViewById(R.id.txtSeekBarMinValue)).setText(Float
					.toString(_minValue));
			((TextView) findViewById(R.id.txtSeekBarMaxValue)).setText(Float
					.toString(_maxValue));
			_currentValueText = (TextView) findViewById(R.id.txtSeekBarCurrentValue);
		}

		// Setup the SeekBar
		_seekBar = (SeekBar) findViewById(R.id.seekBarWidget);
		_seekBar.setOnSeekBarChangeListener(this);

		// This triggers OnProgressChanged() and a VrpnUpdate
		int currentValue = (int) (100.0 * (_currentValue - _minValue) / (_maxValue - _minValue));
		_seekBar.setProgress(currentValue);
	}

	/**
	 * Called when the progress of the SeekBar changes
	 * 
	 * Compute the value from the progress and send a VrpnUpdate to the analog
	 * channel that is associated to the widget with the custom attribute
	 * app:vrpnAnalog
	 */
	@Override
	public void onProgressChanged(SeekBar seekBar, int progress,
			boolean fromUser) {
		// progress is in range 0..100 : map to min..max
		_currentValue = (_maxValue - _minValue) / 100 * progress + _minValue;
		if (!_hideValues) {
			_currentValueText.setText(String.format("%.2f", _currentValue));
		}

		Log.d(TAG, "onProgressChanged " + _vrpnAnalogId + " progress/value: "
				+ progress + " " + _currentValue);
		// Send the vrpn update
		// Note : The line below throws an exception in the Eclipse GUI preview
		VrpnClient.getInstance().sendAnalog(_vrpnAnalogId, _currentValue);

	}

	@Override
	public void onStartTrackingTouch(SeekBar seekBar) {
	}

	@Override
	public void onStopTrackingTouch(SeekBar seekBar) {
	}

}
