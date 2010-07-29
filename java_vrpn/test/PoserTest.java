
import vrpn.*;

public class PoserTest implements vrpn.TrackerRemote.PositionChangeListener
{
	public void trackerPositionUpdate( TrackerRemote.TrackerUpdate u,
									   TrackerRemote tracker )
	{
		System.out.println( "Tracker position message from vrpn: \n" +
							"\ttime:  " + u.msg_time.getTime( ) + "  sensor:  " + u.sensor + "\n" +
							"\tposition:  " + u.pos[0] + " " + u.pos[1] + " " + u.pos[2] + "\n" +
							"\torientation:  " + u.quat[0] + " " + u.quat[1] + " " +
							u.quat[2] + " " + u.quat[3] );
	}


	public static void main( String[] args )
	{
		String poserName = "Poser0@localhost";
		PoserRemote poser = null;
		TrackerRemote tracker = null;

		try
		{
			tracker = new TrackerRemote( poserName, null, null, null, null );
		}
		catch( InstantiationException e )
		{
			// do something b/c you couldn't create the force device
			System.out.println( "We couldn't connect to tracker " + poserName + "." );
			System.out.println( e.getMessage( ) );
			return;
		}

		try
		{
			poser = new PoserRemote( poserName, null, null, null, null );
		}
		catch( InstantiationException e )
		{
			// do something b/c you couldn't create the force device
			System.out.println( "We couldn't connect to poser " + poserName + "." );
			System.out.println( e.getMessage( ) );
			return;
		}
		
		PoserTest test = new PoserTest();
		tracker.addPositionChangeListener( test );
		
		//try some poser methods
		
		double[] pos = { 0, 1, 2 };
		double[] quat = { 3, 4, 5, 6 };
		poser.requestPose( pos, quat );
		try
		{
			Thread.sleep( 1000 );
		}
		catch( Exception e )  { }
		poser.requestPoseRelative( pos, quat );
			
	}
}
