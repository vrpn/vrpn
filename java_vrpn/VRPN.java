

package vrpn;

public class VRPN
{
	

	/**
	 * All VRPN objects must assert this lock before making any calls
	 * into native code.  A corollary of this is that no native methods
	 * may be public; all native methods must be wrapped by other methods
	 * that first synchronize on this lock before calling the native
	 * method.
	 */
	protected final static Object downInVrpnLock = new Object( );
}
