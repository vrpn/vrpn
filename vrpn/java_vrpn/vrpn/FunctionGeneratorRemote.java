package vrpn;
import java.util.*;

import vrpn.ForceDeviceRemote.ForceChange;
import vrpn.ForceDeviceRemote.ForceChangeListener;

public class FunctionGeneratorRemote extends VRPNDevice implements Runnable
{
	//////////////////
	// Public structures and interfaces
	
	public interface Function { }
	
	public class Function_NULL implements Function { }
	
	public class Function_script implements Function
	{
		public String script = "";
		public Function_script( String script ) { this.script = script; }
	}
	
	public class Channel
	{
		public Function function = new Function_NULL();
	}
	
	public class ChannelReply
	{
		public Date msg_time = new Date();
		public int channelNumber = 0;
		public Channel channel = new Channel();
	}
	
	public interface ChannelReplyListener
	{
		public void fgChannelReply( ChannelReply r, FunctionGeneratorRemote fg );
	}
	
	public class StartReply
	{
		public Date msg_time = new Date();
		public boolean isStarted = false;
	}
	
	public class StopReply
	{
		public Date msg_time = new Date();
		public boolean isStopped = false;
	}
	
	public interface StartStopReplyListener
	{
		public void fgStartReply( StartReply r, FunctionGeneratorRemote fg );
		public void fgStopReply( StopReply r, FunctionGeneratorRemote fg );
	}
	
	public class SampleRateReply
	{
		public Date msg_time = new Date();
		public double sampleRate = 0;
	}
	
	public interface SampleRateReplyListener
	{
		public void fgSampleRateReply( SampleRateReply r, FunctionGeneratorRemote fg );
	}
	
	public class InterpreterReply
	{
		public Date msg_time = new Date();
		public String description = "";
	}
	
	public interface InterpreterReplyListener
	{
		public void fgInterpreterReply( InterpreterReply r, FunctionGeneratorRemote fg );
	}
	
	// end of the public structures and interfaces
	//////////////////////////////////
	

	////////////////////////////////
	// Public methods
	//
		
	/**
	 * @exception java.lang.InstantiationException
	 *		If the function generator could not be created because of problems with
	 *      its native code and linking.
	 */
	public FunctionGeneratorRemote( String name, String localInLogfileName, String localOutLogfileName,
						  			String remoteInLogfileName, String remoteOutLogfileName ) 
		throws InstantiationException
	{
		super( name, localInLogfileName, localOutLogfileName, remoteInLogfileName, remoteOutLogfileName );
	}
	
	
	public int setChannel( int channelNumber, Channel c )
	{
		int retval = 0;
		synchronized( downInVrpnLock )
		{
			retval = setChannel_native( channelNumber, c );
		}
		return retval;
	}
	
	
	public int requestChannel( int channelNumber )
	{
		int retval = 0;
		synchronized( downInVrpnLock )
		{
			retval = requestChannel_native( channelNumber );
		}
		return retval;
	}
	
	
	public int requestAllChannels( )
	{
		int retval = 0;
		synchronized( downInVrpnLock )
		{
			retval = requestAllChannels_native( );
		}
		return retval;
	}
	
	
	public int requestStart( )
	{
		int retval = 0;
		synchronized( downInVrpnLock )
		{
			retval = requestStart_native( );
		}
		return retval;
	}
	
	
	public int requestStop( )
	{
		int retval = 0;
		synchronized( downInVrpnLock )
		{
			retval = requestStop_native( );
		}
		return retval;
	}
	
	
	public int requestSampleRate( float rate )
	{
		int retval = 0;
		synchronized( downInVrpnLock )
		{
			retval = requestSampleRate_native( rate );
		}
		return retval;
	}
	
	
	public int requestInterpreterDescription( )
	{
		int retval = 0;
		synchronized( downInVrpnLock )
		{
			retval = requestInterpreterDescription_native( );
		}
		return retval;
	}
	
	
	public synchronized void addChannelReplyListener( ChannelReplyListener listener )
	{
		channelListeners.addElement( listener );
	}

	/**
	 * @return true on success; false on failure
	 */
	public synchronized boolean removeChannelReplyListener( ChannelReplyListener listener )
	{
		return channelListeners.removeElement( listener );
	}
	
	
	public synchronized void addStartStopReplyListener( StartStopReplyListener listener )
	{
		startStopListeners.addElement( listener );
	}

	/**
	 * @return true on success; false on failure
	 */
	public synchronized boolean removeStartStopReplyListener( StartStopReplyListener listener )
	{
		return startStopListeners.removeElement( listener );
	}
	
	
	public synchronized void addSampleRateReplyListener( SampleRateReplyListener listener )
	{
		sampleRateListeners.addElement( listener );
	}

	/**
	 * @return true on success; false on failure
	 */
	public synchronized boolean removeSampleRateReplyListener( SampleRateReplyListener listener )
	{
		return sampleRateListeners.removeElement( listener );
	}
	
	
	public synchronized void addInterpreterReplyListener( InterpreterReplyListener listener )
	{
		interpreterListeners.addElement( listener );
	}

	/**
	 * @return true on success; false on failure
	 */
	public synchronized boolean removeInterpreterReplyListener( InterpreterReplyListener listener )
	{
		return interpreterListeners.removeElement( listener );
	}
	
	
	// end public methods
	////////////////////////
	
	
	
	////////////////////////
	// Protected methods
	//
	
	protected native int setChannel_native( int channelNumber, Channel c );
	protected native int requestChannel_native( int channelNumber );
	protected native int requestAllChannels_native( );
	protected native int requestStart_native( );
	protected native int requestStop_native( );
	protected native int requestSampleRate_native( float rate );
	protected native int requestInterpreterDescription_native( );
	
	
	protected void stoppedRunning()
	{
		// remove all listeners
		channelListeners.removeAllElements();
		startStopListeners.removeAllElements();
		sampleRateListeners.removeAllElements();
		interpreterListeners.removeAllElements();
		
		// shut down the native object
		synchronized( downInVrpnLock )
		{
			this.shutdownFunctionGenerator( );
		}
	}

	
	/**
	 * Should be called only by mainloop(), a native method which is itself
	 * synchronized.  By extension, this method is synchronized (in that, for
	 * a given FunctionGeneratorRemote object, this method can only be called from one
	 * thread at a time).
	 */
	protected void handleChannelChange_NULL( long tv_sec, long tv_usec,  int channelNumber )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of FunctionGeneratorRemote from calling listeners' forceUpdate
		// method concurrently.
		synchronized( notifyingChannelListenersLock )
		{
			ChannelReply c = new ChannelReply();
			c.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			c.channelNumber = channelNumber;
			c.channel = new Channel();
			
			// notify all listeners
			Enumeration e = channelListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				ChannelReplyListener l = (ChannelReplyListener) e.nextElement( );
				l.fgChannelReply( c, this );
			}
		} 
	} // end method handleChannelChange
	
	
	protected void handleChannelChange_Script( long tv_sec, long tv_usec,  int channelNumber,
												String script )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of FunctionGeneratorRemote from calling listeners' forceUpdate
		// method concurrently.
		synchronized( notifyingChannelListenersLock )
		{
			ChannelReply c = new ChannelReply();
			c.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			c.channelNumber = channelNumber;
			c.channel = new Channel();
			c.channel.function = new Function_script( script );
			
			// notify all listeners
			Enumeration e = channelListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				ChannelReplyListener l = (ChannelReplyListener) e.nextElement( );
				l.fgChannelReply( c, this );
			}
		} 
	} // end method handleChannelChange
	
	
	protected void handleStartReply( long tv_sec, long tv_usec, boolean isStarted )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of FunctionGeneratorRemote from calling listeners' forceUpdate
		// method concurrently.
		synchronized( notifyingStartStopListenersLock )
		{
			StartReply s = new StartReply();
			s.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			s.isStarted = isStarted;
			
			// notify all listeners
			Enumeration e = channelListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				StartStopReplyListener l = (StartStopReplyListener) e.nextElement( );
				l.fgStartReply( s, this );
			}
		} 
	}
	
	
	protected void handleStopReply( long tv_sec, long tv_usec, boolean isStopped )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of FunctionGeneratorRemote from calling listeners' forceUpdate
		// method concurrently.
		synchronized( notifyingStartStopListenersLock )
		{
			StopReply s = new StopReply();
			s.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			s.isStopped = isStopped;
			
			// notify all listeners
			Enumeration e = channelListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				StartStopReplyListener l = (StartStopReplyListener) e.nextElement( );
				l.fgStopReply( s, this );
			}
		} 
	}
	
	
	protected void handleSampleRateReply( long tv_sec, long tv_usec, double rate )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of FunctionGeneratorRemote from calling listeners' forceUpdate
		// method concurrently.
		synchronized( notifyingSampleRateListenersLock )
		{
			SampleRateReply s = new SampleRateReply();
			s.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			s.sampleRate = rate;
			
			// notify all listeners
			Enumeration e = channelListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				SampleRateReplyListener l = (SampleRateReplyListener) e.nextElement( );
				l.fgSampleRateReply( s, this );
			}
		} 	
	}
	
	
	protected void handleInterpreterDescription( long tv_sec, long tv_usec, String desc )
	{
		// putting the body of this function into a synchronized block prevents
		// other instances of FunctionGeneratorRemote from calling listeners' forceUpdate
		// method concurrently.
		synchronized( notifyingInterpreterListenersLock )
		{
			InterpreterReply r = new InterpreterReply();
			r.msg_time.setTime( tv_sec * 1000 + (int) (tv_usec/1000.0) );
			r.description = desc;
			
			// notify all listeners
			Enumeration e = channelListeners.elements( );
			while( e.hasMoreElements( ) )
			{
				InterpreterReplyListener l = (InterpreterReplyListener) e.nextElement( );
				l.fgInterpreterReply( r, this );
			}
		} 
	}

	
	/**
	 * Initialize the native function generator object
	 * @param name The name of the function generator and host (e.g., <code>"Phantom0@myhost.edu"</code>).
	 * @return <code>true</code> if the force device was connected successfully, 
	 *			<code>false</code> otherwise.
	 */
	protected native boolean init( String name, String localInLogfileName,
									String localOutLogfileName, String remoteInLogfileName,
									String remoteOutLogfileName );

	/**
	 * This should only be called from the method finalize()
	 */
	protected native void shutdownFunctionGenerator( );
	
	
	protected native void mainloop( );

	// end protected methods
	///////////////////////
	
	///////////////////
	// data members
	
	protected Vector channelListeners = new Vector();
	protected Vector startStopListeners = new Vector();
	protected Vector sampleRateListeners = new Vector();
	protected Vector interpreterListeners = new Vector();

	/**
	 * @see vrpn.TrackerRemote#notifyingChangeListenersLock
	 */
	protected final static Object notifyingChannelListenersLock = new Object( );
	protected final static Object notifyingStartStopListenersLock = new Object( );
	protected final static Object notifyingSampleRateListenersLock = new Object( );
	protected final static Object notifyingInterpreterListenersLock = new Object( );
	

}
