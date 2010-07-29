import vrpn.*;
import vrpn.AuxiliaryLoggerRemote.LoggingReport;

public class AuxiliaryLoggerTest implements AuxiliaryLoggerRemote.LoggingReportListener
{
	
	public void loggingReport( LoggingReport u, AuxiliaryLoggerRemote logger ) 
	{
		System.out.println( "logging report at (" + u.msg_time + "):  " );
		System.out.println( "\tlocal in:  " + ( u.localInLogfileName == null ? "(null)" : u.localInLogfileName ) );
		System.out.println( "\tlocal out:  " + ( u.localOutLogfileName == null ? "(null)" : u.localOutLogfileName ) );
		System.out.println( "\tremote in:  " + ( u.remoteInLogfileName == null ? "(null)" : u.remoteInLogfileName ) );
		System.out.println( "\tremote out:  " + ( u.remoteOutLogfileName == null ? "(null)" : u.remoteOutLogfileName ) );
		
	}

	
	public static void main( String args[] )
	{
		String loggerName = "ImageStream0@localhost";
		AuxiliaryLoggerRemote l = null;
		
		try
		{
			l = new AuxiliaryLoggerRemote( loggerName );
		}
		catch( InstantiationException e )
		{
			System.out.println( "couldn't connect to auxiliary logger \'" + loggerName + "\'" );
			System.out.println( e.getMessage() );
			return;
		}
		
		AuxiliaryLoggerTest alt = new AuxiliaryLoggerTest();
		l.addLoggingReportListener( alt );
		
		String logfilename = "file" + (int) (Math.random() * 1000);
		l.sendLoggingRequest( logfilename, null, null, null );
		System.out.println( "made logging request for " + logfilename + ", null, null, null" );
		
		try{ Thread.sleep( 3000 ); }
		catch( InterruptedException e ) { }

		l.sendLoggingStatusRequest();
		System.out.println( "made logging status request" );

		try{ Thread.sleep( 3000 ); }
		catch( InterruptedException e ) { }
		
		l.sendLoggingRequest( null, null, null, null );
		System.out.println( "made logging request for null, null, null, null" );

		try{ Thread.sleep( 3000 ); }
		catch( InterruptedException e ) { }
	}


}
