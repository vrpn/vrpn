package eu.ensam.ii.vrpn;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.net.Uri;
import android.util.Log;

/**
 * Send updates button, analog and tracker updates to the JsonNet Vrpn server.
 * 
 * <p>
 * A application that uses the Vrpn widgets must initialize the unique instance
 * of this class by calling {@link #setUri(Context, Uri) setUri} before the
 * widgets are used.
 * </p>
 * <p>
 * This class is not thread-safe. The methods <code>sendButton</code> and
 * <code>sendAnalog</code> must be called from the main UI thread
 * </p>
 * <p>
 * Currently the messages neither include a timestamp nor a sequence number
 * </p>
 * 
 * TODO Add timestamps and sequence numbers Add a method to allow sending TODO
 * tracker updates from the widgets
 */
public class VrpnClient implements SensorEventListener {

	private static VrpnClient _instance;

	private static String LOG_TAG = "VrpnClient";
	private SensorManager _sensorManager;

	private boolean _isTiltTrackerEnabled = false;
	private boolean _isRawGyroscopeEnabled = false;
	private boolean _isRawAccelerometerEnabled = false;

	private int _tiltTrackerSequenceNumber = 0;
	private static final Vec3f Zpos = new Vec3f(0.0f, 0.0f, 1.0f);

	float _values[] = new float[3];

	/*
	 * Create the objects once to avoid repeated allocations in frequently
	 * called functions. As long as the VrpnClient methods that update the
	 * object are called from the same thread, no problem. This is currently the
	 * case, since the sensor updates are call on the main and the other updates
	 * are called from UI widgets callbacks, also on the main UI thread..
	 */
	private DatagramSocket _socket;
	private DatagramPacket _packet;
	JSONArray _jsonArray = new JSONArray();
	private JSONObject _sensorJsonObject = new JSONObject();
	private JSONObject _buttonJsonObject = new JSONObject();
	private JSONObject _analogJsonObject = new JSONObject();
	private Quat _quat = new Quat();
	private Vec3f _v = new Vec3f();

	/*
	 * These Ids must match those declared in vrpn_Tracker_Android.cpp
	 */
	private static final String MSG_KEY_TYPE = "type";
	private static final String MSG_KEY_SEQUENCE_NUMBER = "sn";
	private static final String MSG_KEY_TIMESTAMP = "ts";

	private static final String MSG_KEY_TRACKER_ID = "id";
	private static final String MSG_KEY_TRACKER_QUAT = "quat";
	@SuppressWarnings("unused")
	private static final String MSG_KEY_TRACKER_POS = "pos";

	// Values for MSG_KEY_TRACKER_ID
	private static final int MSG_SENSOR_ID_TILT = 0;
	@SuppressWarnings("unused")
	private static final int MSG_SENSOR_ID_RAW_ACCELEROMETER = 1;
	@SuppressWarnings("unused")
	private static final int MSG_SENSOR_ID_RAW_GYROSCOPE = 2;

	private static final String MSG_KEY_BUTTON_ID = "button";
	private static final String MSG_KEY_BUTTON_STATUS = "state";

	private static final String MSG_KEY_ANALOG_CHANNEL = "num";
	private static final String MSG_KEY_ANALOG_DATA = "data";

	private final int MSG_TYPE_TRACKER = 1;
	private final int MSG_TYPE_BUTTON = 2;
	private final int MSG_TYPE_ANALOG = 3;

	/**************************************************************************
	 * 
	 * Create / Destroy
	 * 
	 **************************************************************************/
	/**
	 * Returns the unique VrpnClient instance
	 */
	public static VrpnClient getInstance() {
		if (_instance == null) {
			_instance = new VrpnClient();
		}
		return _instance;
	}

	/**
	 * Initializes the address and port of the VRPN server.
	 * 
	 * <p>
	 * This method must be called before sending any update to the server.
	 * </p>
	 * Note that the widgets trigger updates when they they are displayed for
	 * the first time. So this method must be called before calling
	 * {@link android.app.Activity#setContentView(int) setContentView} in the
	 * main {@link android.app.Activity activity}
	 * {@link android.app.Activity#onCreate setContentView}
	 * <code>onCreate</code> method
	 * 
	 * @param host
	 *            a string that is a valid host name or IP address
	 * @param port
	 *            the port number of the VRPN server
	 */
	public void setupVrpnServer(InetAddress hostAddress, int port) {

		Log.d(LOG_TAG, "server:host = " + hostAddress + ":" + port);
		/*
		 * Initialize the packet destination
		 */
		_packet.setAddress(hostAddress);
		_packet.setPort(port);
	}

	/**
	 * Load the target server address and port from the shared preferences
	 * 
	 * @param context
	 */
	void init(Context context) {
		_sensorManager = (SensorManager) context
				.getSystemService(Context.SENSOR_SERVICE);
		enableOrDisableSensors();
	}

	/**
	 * Send an update to a Vrpn Button.
	 * 
	 * This update will trigger a button callback on the application side
	 * 
	 * @param buttonId
	 *            the button number. Invalid button numbers are ignored on this
	 *            side, but may trigger an error message on the Vrpn server
	 * @param buttonState
	 *            the new status of the button
	 */
	public void sendButton(int buttonId, boolean buttonState) {
		try {
			_buttonJsonObject.put(MSG_KEY_TYPE, MSG_TYPE_BUTTON);
			_buttonJsonObject.put(MSG_KEY_BUTTON_ID, buttonId);
			_buttonJsonObject.put(MSG_KEY_BUTTON_STATUS, buttonState);
		} catch (JSONException e1) {
			e1.printStackTrace();
		}
		_sendObject(_buttonJsonObject);
	}

	/**
	 * Send an update to a Vrpn Analog channel.
	 * 
	 * This update will trigger an analog callback on the application side
	 * 
	 * @param channelId
	 *            The channel number. Invalid channel numbers are ignored on
	 *            this side but, they may trigger an error message on the Vrpn
	 *            server
	 * @param channelValue
	 *            the data that will be sent on the analog channel
	 */
	public void sendAnalog(int channelId, double channelValue) {
		try {
			_analogJsonObject.put(MSG_KEY_TYPE, MSG_TYPE_ANALOG);
			_analogJsonObject.put(MSG_KEY_ANALOG_CHANNEL, channelId);
			_analogJsonObject.put(MSG_KEY_ANALOG_DATA, channelValue);
		} catch (JSONException e1) {
			e1.printStackTrace();
		}
		_sendObject(_analogJsonObject);
	}

	/**
	 * Enables or disable the tilt tracker.
	 * 
	 * The tilt tracker send a tracker update with a quaternion that rotates the
	 * "earth vertical up" vector to the +Z vector in device coordinates. See
	 * {@link android.hardware.SensorEvent SensorEvent} for a description of the
	 * device coordinate system. This quaternion can be used to retrieve the
	 * roll and pitch of the device.
	 * 
	 * @param enable
	 *            <code>true</code> enables the tilt tracker updates and
	 *            <code>false</code> disable the tilt tracker updates
	 */
	public void enableTiltTracker(boolean enable) {
		_isTiltTrackerEnabled = enable;
		enableOrDisableSensors();
	}

	/**************************************************************************
	 * 
	 * Sensorlistener implementation
	 * 
	 **************************************************************************/
	/**
	 * Called by the system when the sensor accuracy changes
	 * 
	 * This method is empty and should not be called by widgets
	 * 
	 */
	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy) {
	}

	/**
	 * Called by the system when a sensor data changes.
	 * 
	 * This method retrieves the Android sensor object. If an vrpn Tracker is
	 * active that involves this sensor object this method sends a Vrpn tracker
	 * update to the appropriate tracker. packages the sensor data into a JSON
	 * object then send the JSON object onto an UDP socket. The target socket
	 * endpoint configuration is configured by calling setupVrpnServer
	 * 
	 */
	@Override
	public void onSensorChanged(SensorEvent event) {
		final int sensorType = event.sensor.getType();

		// retrieve and update the adequate sequence number.
		switch (sensorType) {
		case Sensor.TYPE_ACCELEROMETER:
			if (_isTiltTrackerEnabled) {
				buildTiltTrackerObject(event.values, event.timestamp);
				_sendObject(_sensorJsonObject);
			}
			if (_isRawAccelerometerEnabled) {
				// To do
			}
			break;
		case Sensor.TYPE_GYROSCOPE:
			if (_isRawGyroscopeEnabled) {
				// To do
			}
			break;

		default:
			return;
		}

		// Add other trackers here

		// Send the data onto the network
	}

	/****************************************************************************
	 * Private methods
	 ****************************************************************************/

	private VrpnClient() {
		try {
			_socket = new DatagramSocket();
		} catch (SocketException e) {
			// Check this device IP network configuration
			e.printStackTrace();
		}

		/*
		 * There is a single _packet instance to avoid numerous object creation
		 * in onSensorChanged()
		 */
		byte dummy[] = { 0 };
		_packet = new DatagramPacket(dummy, dummy.length);

		// Set the address with anything since _sendObject will be called before
		// setUri has been called // why
		try {
			_packet.setAddress(InetAddress.getLocalHost());
		} catch (UnknownHostException e) {
		}
	}

	/**
	 * Send the string representation of a Json Object.
	 * 
	 * @param[in] o the json object to send
	 */
	private void _sendObject(final JSONObject o) {
		Log.d(LOG_TAG, o.toString());

		// Send the data onto the network
		_packet.setData(o.toString().getBytes());
		try {
			_socket.send(_packet);
		} catch (SocketException e) {
			// Local network not ready, no route to host, ...
			;
		} catch (IOException e) {
			// TODO Auto-generated catch block : send a notification ?
			e.printStackTrace();
		}
	}

	/**
	 * Build a tilt tracker JSON object
	 * 
	 * This method updates the _sensorJsonObject object
	 * 
	 * @param x
	 * @param y
	 * @param z
	 * @param timestamp
	 */
	private void buildTiltTrackerObject(final float[] values, long timestamp) {
		_v.set(values[0], values[1], values[2]);
		_v.normalize();
		/*
		 * When the device is at rest, the acceleration vector is -g (earth
		 * vertical up) in device coordinate system (Z out of the screen, that
		 * is Z up when the device lays on a table). Compute the rotation +Z -->
		 * -g
		 */
		_quat.setRotate(Zpos, _v);

		// Build a Json message
		try {
			_sensorJsonObject.put(MSG_KEY_TYPE, MSG_TYPE_TRACKER);
			_sensorJsonObject.put(MSG_KEY_SEQUENCE_NUMBER,
					_tiltTrackerSequenceNumber++);
			_sensorJsonObject.put(MSG_KEY_TIMESTAMP, timestamp);
			_sensorJsonObject.put(MSG_KEY_TRACKER_ID, MSG_SENSOR_ID_TILT);

			_quat.fill(_jsonArray);
			_sensorJsonObject.put(MSG_KEY_TRACKER_QUAT, _jsonArray);
		} catch (JSONException e2) {
			// TODO Auto-generated catch block
			e2.printStackTrace();
		}
	}

	/**
	 * Update the device sensor activation status.
	 */
	private void enableOrDisableSensors() {
		final int rate = SensorManager.SENSOR_DELAY_GAME;
		final Sensor accelerometer = _sensorManager
				.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);

		_sensorManager.unregisterListener(this);

		if (_isTiltTrackerEnabled) {
			_sensorManager.registerListener(this, accelerometer, rate);
		} else {
			/*
			 * We are closing down the tilt tracker, Send an update with
			 * horizontal position
			 */
			buildTiltTrackerObject(new float[] { 0.0f, 0.0f, 1.0f },
					System.nanoTime());
			_sendObject(_sensorJsonObject);
		}
	}

}
