import vrpn.*;

public class TrackerTest 
	implements vrpn.TrackerRemote.PositionChangeListener,
	vrpn.TrackerRemote.VelocityChangeListener,
	vrpn.TrackerRemote.AccelerationChangeListener

{
	public void trackerPositionUpdate( TrackerRemote.TrackerUpdate u,
									   TrackerRemote tracker )
	{
		System.out.println( "Tracker message from vrpn: \n" +
							"\ttime:  " + u.msg_time.getTime( ) + "  sensor:  " + u.sensor + "\n" +
							"\tposition:  " + u.pos[0] + " " + u.pos[1] + " " + u.pos[2] + "\n" +
							"\torientation:  " + u.quat[0] + " " + u.quat[1] + " " +
							u.quat[2] + " " + u.quat[3] );
	}
	
	public void trackerVelocityUpdate( TrackerRemote.VelocityUpdate v,
									   TrackerRemote tracker )
	{
		System.out.println( "Tracker message from vrpn: \n" +
							"\ttime:  " + v.msg_time.getTime( ) + "  sensor:  " + v.sensor + "\n" +
							"\tposition:  " + v.vel[0] + " " + v.vel[1] + " " + v.vel[2] + "\n" +
							"\torientation:  " + v.vel_quat[0] + " " + v.vel_quat[1] + " " +
							v.vel_quat[2] + " " + v.vel_quat[3] + "\n" +
							"\t quat dt:  " + v.vel_quat_dt );
	}
	
	public void trackerAccelerationUpdate( TrackerRemote.AccelerationUpdate a,
										   TrackerRemote tracker )
	{
		System.out.println( "Acceleration message from vrpn: \n" +
							"\ttime:  " + a.msg_time.getTime( ) + "  sensor:  " + a.sensor + "\n" +
							"\tposition:  " + a.acc[0] + " " + a.acc[1] + " " + a.acc[2] + "\n" +
							"\torientation:  " + a.acc_quat[0] + " " + a.acc_quat[1] + " " +
							a.acc_quat[2] + " " + a.acc_quat[3] + "\n" +
							"\t quat dt:  " + a.acc_quat_dt );
	}
	

	public static void main( String[] args )
	{
		TrackerRemote tracker = new TrackerRemote( "Phantom@europium-cs" );
		
		TrackerTest test = new TrackerTest( );
		tracker.addPositionChangeListener( test );
		tracker.addVelocityChangeListener( test );
		tracker.addAccelerationChangeListener( test );
		
		tracker.setUpdateRate( 2 );
		
	}
	
	
}
