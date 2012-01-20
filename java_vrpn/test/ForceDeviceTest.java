import vrpn.*;

public class ForceDeviceTest 
	implements vrpn.ForceDeviceRemote.ForceChangeListener,
	vrpn.ForceDeviceRemote.SCPChangeListener,
	vrpn.ForceDeviceRemote.ForceErrorListener

{
	public void forceUpdate( ForceDeviceRemote.ForceChange f,
							 ForceDeviceRemote forceDevice )
	{
		System.out.println( "ForceDevice force message from vrpn: \n" /*+
							"\ttime:  " + u.msg_time.getTime( ) + "  sensor:  " + u.sensor + "\n" +
							"\tposition:  " + u.pos[0] + " " + u.pos[1] + " " + u.pos[2] + "\n" +
							"\torientation:  " + u.quat[0] + " " + u.quat[1] + " " +
							u.quat[2] + " " + u.quat[3] */ );
	}
	
	public void scpUpdate( ForceDeviceRemote.SCPChange s,
						   ForceDeviceRemote forceDevice )
	{
		System.out.println( "ForceDevice surface contact message from vrpn: \n" /* +
							"\ttime:  " + v.msg_time.getTime( ) + "  sensor:  " + v.sensor + "\n" +
							"\tvelocity:  " + v.vel[0] + " " + v.vel[1] + " " + v.vel[2] + "\n" +
							"\torientation:  " + v.vel_quat[0] + " " + v.vel_quat[1] + " " +
							v.vel_quat[2] + " " + v.vel_quat[3] + "\n" +
							"\t quat dt:  " + v.vel_quat_dt */ );
	}
	
	public void forceError( ForceDeviceRemote.ForceError e,
							ForceDeviceRemote forceDevice )
	{
		System.out.println( "ForceDevice force error message from vrpn: \n" /* +
							"\ttime:  " + a.msg_time.getTime( ) + "  sensor:  " + a.sensor + "\n" +
							"\tposition:  " + a.acc[0] + " " + a.acc[1] + " " + a.acc[2] + "\n" +
							"\torientation:  " + a.acc_quat[0] + " " + a.acc_quat[1] + " " +
							a.acc_quat[2] + " " + a.acc_quat[3] + "\n" +
							"\t quat dt:  " + a.acc_quat_dt */ );
	}
	

	public static void main( String[] args )
	{
		String forceDeviceName = "Phantom@gold-cs";
		ForceDeviceRemote forceDevice = null;
		try
		{
			forceDevice = new ForceDeviceRemote( forceDeviceName, null, null, null, null );
		}
		catch( InstantiationException e )
		{
			// do something b/c you couldn't create the force device
			System.out.println( "We couldn't connect to force device " + forceDeviceName + "." );
			System.out.println( e.getMessage( ) );
			return;
		}
		
		ForceDeviceTest test = new ForceDeviceTest( );
		forceDevice.addForceChangeListener( test );
		forceDevice.addSCPChangeListener( test );
		forceDevice.addForceErrorListener( test );
		
		// try out the various force device methods...
		
	}
	
	
}
