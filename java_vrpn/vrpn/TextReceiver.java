package vrpn;

import java.util.*;

public class TextReceiver extends VRPNDevice implements Runnable
{
	////////////////////////////
	// public structures and interfaces
	
	public class TextMessage
	{
		public Date msg_time = new Date();
		public String msg = "";
		public int type = vrpn_TEXT_NORMAL;
		public int level = 0;
	}
	
	public interface TextMessageListener
	{
		public void receiveTextMessage( TextMessage t, TextReceiver r );
	}
	
	// end public structures and interfaces
	/////////////////////////////
	
	/////////////////////////////
	// public methods
	
	public TextReceiver( String name, String localInLogfileName, String localOutLogfileName,
						  String remoteInLogfileName, String remoteOutLogfileName ) 
		throws InstantiationException
	{
		super( name, localInLogfileName, localOutLogfileName, remoteInLogfileName, remoteOutLogfileName );
	}
	
	
	public synchronized void addTextListener( TextMessageListener listener )
	{
		textListeners.addElement( listener );
	}
	
	
	public synchronized boolean removeTextListener( TextMessageListener listener )
	{
		return textListeners.removeElement( listener );
	}
	
	// end public methods
	//////////////////
	
	
	//////////////////
	// protected methods
	
	protected void stoppedRunning( )
	{
		textListeners.removeAllElements( );
		synchronized( downInVrpnLock )
		{
			this.shutdownTextReceiver( );
		}
	}

	
	protected void handleTextMessage( long tv_sec, long tv_usec, 
									  int type, int level, String msg )
	{
		synchronized( notifyingListenersLock )
		{
			TextMessage t = new TextMessage( );
			t.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			t.type = type;
			t.level = level;
			t.msg = msg;
			
			Enumeration e = textListeners.elements();
			while( e.hasMoreElements() )
			{
				TextMessageListener l = (TextMessageListener) e.nextElement();
				l.receiveTextMessage( t, this );
			}
		}
	} // end method handleTextMessage
	
	
	protected native void shutdownTextReceiver( );
	
	
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

	
	//  end protected methods
	////////////////////
	
	
	/////////////////
	//  data members
	
	protected Vector textListeners = new Vector();
	protected final static Object notifyingListenersLock = new Object();
	
}
