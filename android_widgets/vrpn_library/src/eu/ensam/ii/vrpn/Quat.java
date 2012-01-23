package eu.ensam.ii.vrpn;

import org.json.JSONArray;
import org.json.JSONException;

import android.util.FloatMath;

/**
 * A simple float quaternion
 * 
 * @author Philippe
 *
 */
class Quat {
	Vec3f v = new Vec3f();
	float w = 0.0f;
	
	Quat() {
	}
	
	/**
	 * Computes a rotation quaternion that rotates <code>s</code> to <code>t</code>
	 * 
	 * @param s
	 * @param t
	 */
	void setRotate(final Vec3f s, final Vec3f t) {
		// from Real-Time Rendering, 3d ed p 79
		final float e = s.dot(t);
		final float k = FloatMath.sqrt(2.0f * (1.0f + e));
		v = s.cross(t).mul(1.0f / k); 
		w = k / 2.0f;
	}
	
	/**
	 * Fill the first 4 elements of the JSON array with the values of this quaternion object
	 * 
	 * Extraneous existing elements of the array are set to null
	 * 
	 * @param jsonArray the JSON array to be filled
	 * @throws JSONException
	 */
	final void fill(JSONArray jsonArray) throws JSONException {
		jsonArray.put(0, v.x);
		jsonArray.put(1, v.y);
		jsonArray.put(2, v.z);
		jsonArray.put(3, w);
		int i = 4;
		while (i < jsonArray.length()) {
			jsonArray.put(i++, null);
		}
	}

}
