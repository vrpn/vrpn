import java.io.*;
import vrpn.*;

public class TempImagerTest
	implements TempImagerRemote.DescriptionChangeListener,
	TempImagerRemote.RegionChangeListener
{

	public void tempImagerRegionUpdate( TempImagerRemote.TempImagerRegionUpdate u,
								 TempImagerRemote t )
	{
		System.out.println( "Got a region update" );
		
	}
	
	public void tempImagerDescriptionUpdate( TempImagerRemote.TempImagerDescriptionUpdate u,
									  TempImagerRemote t )
	{
		System.out.println( "Got a description update" );
	}
	
	
	public static void main( String[] args )
	{
		String imagerName = "tempImager@localhost";
		TempImagerRemote imager = null;
		try
		{
			imager = new TempImagerRemote( imagerName );
		}
		catch( InstantiationException e )
		{
			// do something b/c you couldn't create the imager
			System.out.println( "We couldn't connect to tempImager " + imagerName + "." );
			System.out.println( e.getMessage( ) );
			return;
		}
		
		TempImagerTest test = new TempImagerTest( );
		imager.addDescriptionChangeListener( test );
		imager.addRegionChangeListener( test );
		
		imager.setTimerPeriod( 10 );
		
		try
		{
			System.in.read();
		}
		catch( IOException e )
		{
		}
	}


}