package vrpn;

import java.util.*;

public class TextSender extends VRPNDevice implements Runnable
{
	
	/////////////////////////////
	// public methods
	
	// NOTE:  because TextSender is technically a server, the logfile-name specifications
	// will do nothing.
	public TextSender( String name, String localInLogfileName, String localOutLogfileName,
					   String remoteInLogfileName, String remoteOutLogfileName ) 
		throws InstantiationException
	{
		super( name, localInLogfileName, localOutLogfileName, remoteInLogfileName, remoteOutLogfileName );
	}
	
	
	public boolean sendMessage( String msg, int type, int level, Date time )
	{
		boolean retval = false;
		synchronized( downInVrpnLock )
		{
			retval = this.sendMessage_native( msg, type, level, time.getTime() );
		}
		return retval;
	}

	// sends the specified message with 'normal' severity, 0-level and
	// the current time.
	public boolean sendMessage( String msg )
	{
		return sendMessage( msg, vrpn_TEXT_NORMAL, 0, vrpn_TEXT_NOW );
	}

	
	// end public methods
	//////////////////
	
	
	//////////////////
	// protected methods
	
	protected void stoppedRunning( )
	{
		synchronized( downInVrpnLock )
		{
			this.shutdownTextSender( );
		}
	}

	
	protected native void shutdownTextSender( );
	
	
	/**
	 * This should only be called from the method run()
	 */
	protected native void mainloop( );
	

	/**
	 * Initialize the native text receiver object
	 * @param name The name of the text sender and host (e.g., <code>"Messages0@myhost.edu"</code>).
	 * @return <code>true</code> if the tracker was connected successfully, 
	 *			<code>false</code> otherwise.
	 */
	protected native boolean init( String name, String localInLogfileName, 
								   String localOutLogfileName, String remoteInLogfileName,
								   String remoteOutLogfileName );

	protected native boolean sendMessage_native(  String msg, int type, int level, long time );
	
	//  end protected methods
	////////////////////
	
	

}
