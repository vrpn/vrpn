import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.text.DecimalFormat;

import vrpn.*;


/**
 * @author Oliver Otto
 * @version 1.0
 *
 * This program is comparable with the printcereal and vrpn_prind_devices program
 * from the original vrpn client programs. It can read the input from any analog, 
 * tracker and button device.
 */
public class PrintCereals
	implements vrpn.ButtonRemote.ButtonChangeListener,
	vrpn.AnalogRemote.AnalogChangeListener,
	vrpn.TrackerRemote.PositionChangeListener,
	vrpn.TrackerRemote.VelocityChangeListener,
	vrpn.TrackerRemote.AccelerationChangeListener
{
	String stationName = null;
	ButtonRemote button = null;	
	AnalogRemote analog = null;
	TrackerRemote tracker = null;

	/**
	 * @param name The station name to which we connect
	 */
	public PrintCereals(String name)
	{
		stationName = name;
		
		// Button init
		 try
		 {
			 button = new ButtonRemote( stationName, null, null, null, null );
		 }
		 catch( InstantiationException e )
		 {
			 // do something b/c you couldn't create the button
		 	 System.out.println( "We couldn't connect to button " + stationName + "." );
			 System.out.println( e.getMessage( ) );
			 return;
		 }
		
		 button.addButtonChangeListener( this );

		// Analog init		
		try
		{
			analog = new AnalogRemote(stationName,null,null,null,null);
		}
		catch( InstantiationException e )
		{
			// do something b/c you couldn't create the analog
			System.out.println( "We couldn't connect to analog " + stationName + "." );
			System.out.println( e.getMessage( ) );
			return;
		}
		
		analog.addAnalogChangeListener( this );
		
		
		try
		{
			//tracker = new TrackerRemote( trackerName, "localIn", "localOut", "remoteIn", "remoteOut" );
			tracker = new TrackerRemote( stationName, null, null, null, null );
		}
		catch( InstantiationException e )
		{
			// do something b/c you couldn't create the tracker
			System.out.println( "We couldn't connect to tracker " + stationName + "." );
			System.out.println( e.getMessage( ) );			
			return;
		}

		
		tracker.addPositionChangeListener( this );
		tracker.addVelocityChangeListener( this );
		tracker.addAccelerationChangeListener( this );

		tracker.setUpdateRate( 2 );
			
		 
	}
	
	public void shutdown()
	{
		button.removeButtonChangeListener(this);
		analog.removeAnalogChangeListener(this);
	}
	
	/** 
	 * This method prints out every button update change.
	 * @param u  Used to read all necessary information ot of the active channels
	 * @param button Used to get the number of active channels
	 * (non-Javadoc)
	 * @see vrpn.ButtonRemote.ButtonChangeListener#buttonUpdate(vrpn.ButtonRemote.ButtonUpdate, vrpn.ButtonRemote)
	 */
	public void buttonUpdate( ButtonRemote.ButtonUpdate u,
									   ButtonRemote button )
	{
		System.out.println(this.stationName +" B"+ u.button +" -> "+ u.state);
	}
	
	/**
	 * This method prints out every analog update change.
	 * @param u  Used to read all necessary information ot of the active channels
	 * @param tracker Used to get the number of active channels
	 * (non-Javadoc)
	 * @see vrpn.AnalogRemote.AnalogChangeListener#analogUpdate(vrpn.AnalogRemote.AnalogUpdate, vrpn.AnalogRemote)
	 */
	public void analogUpdate( vrpn.AnalogRemote.AnalogUpdate u,
							  vrpn.AnalogRemote tracker )
	{ 
		DecimalFormat f = new DecimalFormat(" 0.00;-0.00");		
		
		System.out.print( this.stationName + " Analogs:");		
		for(int i=0; i<tracker.getNumActiveChannels(); i++)
		{
			System.out.print(" "+f.format(u.channel[i]));
		}
		System.out.println("");		
	}
	
	/**
	 * This method prints out every tracker position update change.
	 * @param u  Used to read all necessary information ot of the active channels
	 * @param tracker Used to get the number of active channels
	 */
	public void trackerPositionUpdate( TrackerRemote.TrackerUpdate u,
									   TrackerRemote tracker )
	{
		DecimalFormat f = new DecimalFormat(" 0.00;-0.00");
		
		System.out.println( "Tracker "+ this.stationName +", sensor " + u.sensor + ":\n" +
							"\tpos ( " + f.format(u.pos[0]) + ", " + f.format(u.pos[1]) + ", " + f.format(u.pos[2]) + "); " +
							"quat ( " + f.format(u.quat[0]) + ", " + f.format(u.quat[1]) + ", " +
							f.format(u.quat[2]) + ", " + f.format(u.quat[3])+ ")"  );
	}
	
	/**
	 * This method prints out every tracker velocity update change.
	 * @param v  Used to read all necessary information ot of the active channels
	 * @param tracker Used to get the number of active channels
	 */	
	public void trackerVelocityUpdate( TrackerRemote.VelocityUpdate v,
									   TrackerRemote tracker )
	{
		System.out.println( "Tracker velocity message from vrpn: \n" /* +
							"\ttime:  " + v.msg_time.getTime( ) + "  sensor:  " + v.sensor + "\n" +
							"\tvelocity:  " + v.vel[0] + " " + v.vel[1] + " " + v.vel[2] + "\n" +
							"\torientation:  " + v.vel_quat[0] + " " + v.vel_quat[1] + " " +
							v.vel_quat[2] + " " + v.vel_quat[3] + "\n" +
							"\t quat dt:  " + v.vel_quat_dt */ );
	}

	/**
	 * This method prints out every tracker acceleration update change.
	 * @param a  Used to read all necessary information ot of the active channels
	 * @param tracker Used to get the number of active channels
	 */		
	public void trackerAccelerationUpdate( TrackerRemote.AccelerationUpdate a,
										   TrackerRemote tracker )
	{
		System.out.println( "Tracker acceleration message from vrpn: \n" /* +
							"\ttime:  " + a.msg_time.getTime( ) + "  sensor:  " + a.sensor + "\n" +
							"\tposition:  " + a.acc[0] + " " + a.acc[1] + " " + a.acc[2] + "\n" +
							"\torientation:  " + a.acc_quat[0] + " " + a.acc_quat[1] + " " +
							a.acc_quat[2] + " " + a.acc_quat[3] + "\n" +
							"\t quat dt:  " + a.acc_quat_dt */ );
	}
	
	/**
	 * @param args Expect to get the station name to to which we want to connect.
	 * @throws Exception If there is no agrumnet, a usage information is shown.
	 */
	public static void main(String[] args)
		throws Exception {
			/* check command-line options */
			if (args.length<1) {
				System.err.println("Usage: java "+PrintCereals.class.getName()+" "+
                                 "Device_name");
				System.err.println("Device_name:  VRPN name of data source to contact");
				System.err.println("    example:  Magellan0@localhost");
				System.exit(2);
			}		
		PrintCereals[] new_station = new PrintCereals[args.length];
									  
		for(int i=0; i < args.length; i++) 
		{
			// parse args			
			new_station[i] = new PrintCereals ( args[i]);

			// initialize the PC/station
			System.out.println("Button's name is "+new_station[i].stationName);
			System.out.println("Analog's name is "+new_station[i].stationName);
			//System.out.println("Dial's name is "+new_station[i].stationName);
					System.out.println("Button update: B<number> is <newstate>");			System.out.println("Analog update: Analogs: [new values listed]");			//System.out.println("Dial update: Dial# spun by [amount]");
		
		} // end args for loop 
						
		
		// wait for q to quit
		BufferedReader in = new BufferedReader(new InputStreamReader(System.in));
		while(true)
		{
			String s = in.readLine(); //in.readLine();			
			if (s.equalsIgnoreCase("q"))
			{
				for(int i=0; i < args.length; i++) 
				{
					new_station[i].shutdown();
/* This seems to have no effect on the error message we get when finishing the program
					try
					{
						new_station[i].finalize();
					}
					catch (Throwable e)
					{
						System.out.println("Could not finalize station "+i+" of "+new_station[i].stationName);
					}
*/					
				}
				System.exit(0);				
			}		
		}
	}
}
