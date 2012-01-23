import vrpn.*;

public class AnalogTest 
	implements vrpn.AnalogRemote.AnalogChangeListener
{
	public void analogUpdate( vrpn.AnalogRemote.AnalogUpdate u,
							  vrpn.AnalogRemote tracker )
	{
		
		System.out.println( "Analog message from vrpn: \n" +
							"\ttime:  " + u.msg_time.getTime( ) + "\n" /*+
							"\tposition:  " + u.pos[0] + " " + u.pos[1] + " " + u.pos[2] + "\n" +
							"\torientation:  " + u.quat[0] + " " + u.quat[1] + " " +
							u.quat[2] + " " + u.quat[3] */ );
		
	}
	

	public static void main( String[] args )
	{
		String analogName = "tetra_force_server@palladium-cs:4700";
		AnalogRemote analog = null;
		AnalogOutputRemote ao = null;
		try
		{
			analog = new AnalogRemote( analogName, null, null, null, null );
			ao = new AnalogOutputRemote( analogName, null, null, null, null );
		}
		catch( InstantiationException e )
		{
			// do something b/c you couldn't create the analog
			System.out.println( "We couldn't connect to analog " + analogName + "." );
			System.out.println( e.getMessage( ) );
			return;
		}
		
		AnalogTest test = new AnalogTest( );
		analog.addAnalogChangeListener( test );
		
		ao.requestValueChange( 2, 5 );
		
	}
}
