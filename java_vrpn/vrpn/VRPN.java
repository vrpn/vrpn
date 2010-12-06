

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
	
	static
	{
		try { System.loadLibrary( "java_vrpn" ); }
		catch( UnsatisfiedLinkError e )
		{
			System.out.println( e.getMessage( ) );
			System.out.println( "Error initializing java_vrpn." );
			System.out.println( " -- Unable to find native library." );
		}
		catch( SecurityException e )
		{
			System.out.println( e.getMessage( ) );
			System.out.println( "Security exception:  you couldn't load the native vrpn dll." );
		}
	} // end static initializer block
	
	
} // end class VRPN
