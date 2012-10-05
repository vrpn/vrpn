package eu.ensam.ii.vrpn;

import android.util.FloatMath;
import org.json.JSONArray;
import org.json.JSONException;

/**
 * A basic 3-element float vector.
 * 
 * Used for some computations with SensorEvent values
 * 
 * @author Philippe
 *
 */
class Vec3f {
	public float x, y, z;
	
	Vec3f() {
		x = y = z = 0.0f;
	}
	
	Vec3f(float _x, float _y, float _z) {
		set(_x, _y, _z);
	}

	void set(float _x, float _y, float _z) {
		x = _x;
		y = _y;
		z = _z;
	}

	
	final float dot(final Vec3f v) {
		return x*v.x + y*v.y + z*v.z;
	}
	
	final Vec3f cross(final Vec3f v) {
		Vec3f w = new Vec3f(
			y*v.z - z*v.y,
			z*v.x - x*v.z,
			x*v.y - y*v.x);
		return w;
	}
	
	/**
	 * Computes a normalized vector
	 * @param u 
	 * @return normalized vector value
	 */
	static Vec3f normalize(final Vec3f u) {
		final float norm = u.norm();
		Vec3f v = u;
		v.x /= norm;
		v.y /= norm;
		v.z /= norm;
		return v;
	}

	/**
	 * Normalizes this vector
	 */
	final void normalize() {
		final float norm = norm();
		x /= norm;
		y /= norm;
		z /= norm;
	}
	
	/**
	 * Computes the norm of this vector
	 * 
	 * @return the float value of the norm of the object
	 */
	final float norm() {
		return FloatMath.sqrt(x*x + y*y + z*z);
	}
	
	/**
	 * Fill the first 3 elements of the JSON array with the values of this Vec3f object
	 * 
	 * Extraneous existing elements of the array are set to null
	 * 
	 * @param jsonArray the JSON array to be filled
	 * @throws JSONException
	 */
	final void fill(JSONArray jsonArray) throws JSONException {
		jsonArray.put(0, x);
		jsonArray.put(1, y);
		jsonArray.put(2, z);
		int i = 3;
		while (i < jsonArray.length()) {
			jsonArray.put(i++, null);
		}
	}
	
	Vec3f mul(float k) {
		return new Vec3f(x*k, y*k, z*k);
	}
}
