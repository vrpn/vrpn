package edu.unc.cs.vrpn;

import jni.JniLayer;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;

/**
 * Handles input from the phone's accelerometer.
 * 
 * @author Ted
 * 
 */
public class AccelerometerListener implements SensorEventListener
{
	private JniBuilder builder;
	private JniBuilder.Analog accelerometer;
	private JniLayer layer;
	private int cachedJniValue;
	private String last = "";
	private Main activity;

	public AccelerometerListener(JniBuilder builder, JniBuilder.Analog accelerometer)
	{
		this.builder = builder;
		this.accelerometer = accelerometer;
		if (this.builder.isLocked())
		{
			this.layer = this.builder.toJni();
		}

		this.cachedJniValue = this.builder.getAnalogIndex(this.accelerometer);
	}
	
	public AccelerometerListener(JniBuilder builder, JniBuilder.Analog accelerometer, Main activity)
	{
		this.builder = builder;
		this.accelerometer = accelerometer;
		if (this.builder.isLocked())
		{
			this.layer = this.builder.toJni();
		}

		this.cachedJniValue = this.builder.getAnalogIndex(this.accelerometer);
		this.activity = activity;
	}

	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy)
	{

	}

	@Override
	public void onSensorChanged(SensorEvent event)
	{
		if (this.layer == null && this.builder.isLocked())
		{
			this.layer = this.builder.toJni();
		}
		
		for (int i = 0, stop = (int) Math.min(this.accelerometer.getChannels() , event.values.length), analog = this.cachedJniValue; i < stop; i++)
		{
			this.layer.updateAnalogVal(analog , i , event.values[i]);
		}
		
		this.layer.reportAnalogChg(this.cachedJniValue);
		
		if (this.activity != null)
		{
			String report = String.format("{%.2f, %.2f, %.2f}" , event.values[0], event.values[1], event.values[2]);
			if (!report.equals(this.last))
			{
				this.activity.logAccelerometer(report);
				this.last = report;
			}
		}
	}

}
