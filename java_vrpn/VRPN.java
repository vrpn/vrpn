

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
	
	
	// the arguments to this constructor are just stored for later reference.
	// 'null' is a reasonable value for any (although something else will 
	// surely break if 'name' is null).
	public VRPN( String name, String localInLogfileName, String localOutLogfileName,
				 String remoteInLogfileName, String remoteOutLogfileName )
	{
		this.connectionName = name;
		this.localInLogfileName = localInLogfileName;
		this.localOutLogfileName = localOutLogfileName;
		this.remoteInLogfileName = remoteInLogfileName;
		this.remoteOutLogfileName = remoteOutLogfileName;		
	}
	
	
	public String getConnectionName( )
	{ 
		return connectionName;
	}
	
	public String getLocalInLogfileName( )
	{ 
		return localInLogfileName;
	}
	
	public String getLocalOutLogfileName( )
	{ 
		return localOutLogfileName;
	}
	
	public String getRemoteInLogfileName( )
	{ 
		return remoteInLogfileName;
	}
	
	public String getRemoteOutLogfileName( )
	{ 
		return remoteOutLogfileName;
	}
	
	
	protected String connectionName = null;
	protected String localInLogfileName = null;
	protected String localOutLogfileName = null;
	protected String remoteInLogfileName = null;
	protected String remoteOutLogfileName = null;
	
	// the default constructor is private so that subclasses
	// are forced to tell us the connection name and the
	// various logfile names
	private VRPN( ) { }
} // end class VRPN
