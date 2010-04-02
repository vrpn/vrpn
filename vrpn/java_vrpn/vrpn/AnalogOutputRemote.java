
package vrpn;

public class AnalogOutputRemote extends VRPNDevice implements Runnable
{
	
	//////////////////
	// Public structures and interfaces
	
	public static final int MAX_CHANNELS = 128;
	
	// end of the public structures and interfaces
	///////////////////
	
	///////////////////
	// Public methods
	
	/**
	 * @exception java.lang.InstantiationException
	 *		If the analogOutput could not be created because of problems with
	 *      its native code and linking.
	 */
	public AnalogOutputRemote( String name, String localInLogfileName, String localOutLogfileName,
						  String remoteInLogfileName, String remoteOutLogfileName ) 
		throws InstantiationException
	{
		super( name, localInLogfileName, localOutLogfileName, remoteInLogfileName, remoteOutLogfileName );
	}
	
	public boolean requestValueChange( int channel, double value )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = this.requestValueChange_native( channel, value );
		}
		return retval;
	}
				
	public boolean requestValueChange( double[] values )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = this.requestValueChange_native( values );
		}
		return retval;
	}
	
	public synchronized native int getNumActiveChannels( );
	public int getMaxActiveChannels( )
	{  return AnalogRemote.MAX_CHANNELS;  }
	
	
	// end public methods
	////////////////////////
	
	
	////////////////////////
	// Protected methods
	//
		
	/**
	 * Initialize the native analogOutput object
	 * @param name The name of the analogOutput and host (e.g., <code>"Analog0@myhost.edu"</code>).
	 * @return <code>true</code> if the analogOutput was connected successfully, 
	 *			<code>false</code> otherwise.
	 */
	protected native boolean init( String name, String localInLogfileName, 
								   String localOutLogfileName, String remoteInLogfileName,
								   String remoteOutLogfileName );

	/**
	 * This should only be called from the method finalize()
	 */
	protected native void shutdownAnalogOutput( );
	
	protected native void mainloop( );
	protected native boolean requestValueChange_native( int channel, double value );
	protected native boolean requestValueChange_native( double[] values );
	
	/**
	 * Called when the thread is stopped/the device shuts down
	 */
	protected void stoppedRunning( )
	{
		synchronized( downInVrpnLock )
		{
			this.shutdownAnalogOutput( );
		}
	}

	// end protected methods
	///////////////////////
	
	
}
