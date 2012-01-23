package edu.unc.cs.vrpn;

import jni.JniLayer;
import android.widget.SeekBar;

/**
 * Track changes to the value of a seek bar and notify VRPN clients. This code is fairly
 * self-explanatory and seekbar-specific, so it is not documented as extensively as
 * {@link Main} and {@link JniBuilder}
 * 
 * @author Ted
 * 
 */
public class VrpnSeekBarChangeListener implements SeekBar.OnSeekBarChangeListener
{
	/**
	 * The JniBuilder which has an analog for this seekbar
	 */
	private JniBuilder builder;
	/**
	 * The JniLayer which {@link VrpnSeekBarChangeListener#builder} created
	 */
	private JniLayer layer = null;
	/**
	 * A reference to this seekbar analog from {@link VrpnSeekBarChangeListener#builder}.
	 * This could be recreated by going to that object, but it is cached here.
	 */
	private JniBuilder.Analog analog;
	/**
	 * Another cached JniBuilder value
	 */
	private int cachedAnalog;

	/**
	 * It is necessary for the SeekBarChangeListener to have a reference to the
	 * {@link Main} activity. For obvious reasons this is not particularly loosely-coupled
	 * so it would be best to refactor this before attempting serious modifications.
	 */
	private Main activity = null;
	private String msg;

	public VrpnSeekBarChangeListener(JniBuilder builder, JniBuilder.Analog analog)
	{
		this.builder = builder;
		this.analog = analog;
		this.cachedAnalog = this.builder.getAnalogIndex(this.analog);
		if (this.builder.isLocked())
			this.layer = builder.toJni();
	}

	public VrpnSeekBarChangeListener(JniBuilder builder, JniBuilder.Analog analog, Main activity)
	{
		this(builder , analog);
		this.activity = activity;
	}

	@Override
	public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser)
	{
		this.layer.updateAnalogVal(this.cachedAnalog , 0 , progress); // Should compute
																		// these
		this.layer.reportAnalogChg(this.cachedAnalog);
		if (this.activity != null)
		{
			this.msg = "Seek bar at " + progress + "%";
			this.activity.logSlider(msg);
		}
	}

	@Override
	public void onStartTrackingTouch(SeekBar seekBar)
	{
		if (this.activity != null)
			this.activity.logSlider("Seekbar Pressed");
	}

	@Override
	public void onStopTrackingTouch(SeekBar seekBar)
	{
		if (this.activity != null)
		{
			this.activity.logSlider(this.msg + " (released)");
		}
	}

}
