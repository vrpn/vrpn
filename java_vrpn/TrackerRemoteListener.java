/***************************************************************************
 * Use this class to store updates in a vector and get them when you want
 *
 * How it is currently set up...
 * 
 * 1. The vector is emptied when all the updates are returned
 * 2. The vector keeps only the latest update when the mode is set to last
 * 
 * It's easy to change these settings, but if you're too lazy to do it,
 * contact Tatsuhiro Segi (segi@email.unc.edu)
 ***************************************************************************/


import vrpn.*;
import java.util.Vector;


public class TrackerRemoteListener
	implements vrpn.TrackerRemote.PositionChangeListener, 
				vrpn.TrackerRemote.VelocityChangeListener, 
				vrpn.TrackerRemote.AccelerationChangeListener
{
	Vector trackerUpdates;
	Vector velocityUpdates;
	Vector accelerationUpdates;
	
	boolean returnLastTracker;
	boolean returnLastVelocity;
	boolean returnLastAcceleration;
	
	
	public TrackerRemoteListener(TrackerRemote tracker)
	{
		tracker.addPositionChangeListener(this);
		tracker.addVelocityChangeListener(this);
		tracker.addAccelerationChangeListener(this);
		
		trackerUpdates = new Vector();
		velocityUpdates = new Vector();
		accelerationUpdates = new Vector();
		
		returnLastTracker = true;
		returnLastVelocity = true;
		returnLastAcceleration = true;
	}
	
	
	//** Empty the vector when the mode is set to last
	public synchronized void setModeLastTrackerUpdate()
	{
		returnLastTracker = true;
		
		if (!trackerUpdates.isEmpty())
		{
			Object temp = trackerUpdates.lastElement();
		
			trackerUpdates.removeAllElements();
			trackerUpdates.addElement(temp);
		}
	}
	
	
	public synchronized void setModeLastVelocityUpdate()
	{
		returnLastVelocity = true;
		
		if (!velocityUpdates.isEmpty())
		{
			Object temp = velocityUpdates.lastElement();
		
			velocityUpdates.removeAllElements();
			velocityUpdates.addElement(temp);
		}
	}
	
	
	public synchronized void setModeLastAccelerationUpdate()
	{
		returnLastAcceleration = true;
		
		if (!accelerationUpdates.isEmpty())
		{
			Object temp = accelerationUpdates.lastElement();
		
			accelerationUpdates.removeAllElements();
			accelerationUpdates.addElement(temp);
		}
	}
	
	
	public synchronized void setModeAllTrackerUpdates()
	{
		returnLastTracker = false;
	}
	
	
	public synchronized void setModeAllVelocityUpdates()
	{
		returnLastVelocity = false;
	}
	
	
	public synchronized void setModeAllAccelerationUpdates()
	{
		returnLastAcceleration = false;
	}
	
	
	public synchronized boolean getModeTrackerUpdate()
	{
		return returnLastTracker;
	}
	
	
	public synchronized boolean getModeVelocityUpdate()
	{
		return returnLastVelocity;
	}
	
	
	public synchronized boolean getModeAccelerationUpdate()
	{
		return returnLastAcceleration;
	}
	
	
	//** Empty the vector when all the updates are returned
	public synchronized Vector getTrackerUpdate()
	{
		if (trackerUpdates.isEmpty())
		{
			return null;
		}
		
		if (returnLastTracker)
		{
			Vector last = new Vector();
			
			last.addElement(trackerUpdates.lastElement());
			
			return last;
		}
		
		else
		{
			Vector all = new Vector();
			
			for (int i=0; i<trackerUpdates.size(); i++)
			{
				all.addElement(trackerUpdates.elementAt(i));
			}
			
			trackerUpdates.removeAllElements();
			
			return all;
		}
	}
	
	
	public synchronized Vector getVelocityUpdate()
	{
		if (velocityUpdates.isEmpty())
		{
			return null;
		}
		
		if (returnLastVelocity)
		{
			Vector last = new Vector();
			
			last.addElement(velocityUpdates.lastElement());
			
			return last;
		}
		
		else
		{
			Vector all = new Vector();
			
			for (int i=0; i<velocityUpdates.size(); i++)
			{
				all.addElement(velocityUpdates.elementAt(i));
			}
			
			velocityUpdates.removeAllElements();
			
			return all;
		}
	}
	
	
	public synchronized Vector getAcclerationUpdate()
	{
		if (accelerationUpdates.isEmpty())
		{
			return null;
		}
		
		if (returnLastAcceleration)
		{
			Vector last = new Vector();
			
			last.addElement(accelerationUpdates.lastElement());
			
			return last;
		}
		
		else
		{
			Vector all = new Vector();
			
			for (int i=0; i<accelerationUpdates.size(); i++)
			{
				all.addElement(accelerationUpdates.elementAt(i));
			}
			
			accelerationUpdates.removeAllElements();
			
			return all;
		}
	}
	
	
	public synchronized TrackerRemote.TrackerUpdate getLastTrackerUpdate()
	{
		TrackerRemote.TrackerUpdate tracker = (TrackerRemote.TrackerUpdate)(trackerUpdates.lastElement());
		
		return tracker;
	}
	
	
	public synchronized TrackerRemote.VelocityUpdate getLastVelocityUpdate()
	{
		TrackerRemote.VelocityUpdate vel = (TrackerRemote.VelocityUpdate)(velocityUpdates.lastElement());
		
		return vel;
	}
	
	
	public synchronized TrackerRemote.AccelerationUpdate getLastAccelerationUpdate()
	{
		TrackerRemote.AccelerationUpdate acc = (TrackerRemote.AccelerationUpdate)(accelerationUpdates.lastElement());
		
		return acc;
	}
	
	
	//** Keep only the last update if the mode is set to last
	public synchronized void trackerPositionUpdate (TrackerRemote.TrackerUpdate u, TrackerRemote tracker)
	{
		if (returnLastTracker)
		{
			trackerUpdates.removeAllElements();
		}
			
		trackerUpdates.addElement(u);
	}
	
	
	public synchronized void trackerVelocityUpdate (TrackerRemote.VelocityUpdate v, TrackerRemote tracker)
	{
		if (returnLastVelocity)
		{
			velocityUpdates.removeAllElements();
		}
		
		velocityUpdates.addElement(v);
	}
	
	
	public synchronized void trackerAccelerationUpdate (TrackerRemote.AccelerationUpdate a, TrackerRemote tracker )
	{
		if (returnLastAcceleration)
		{
			accelerationUpdates.removeAllElements();
		}
		
		accelerationUpdates.addElement(a);
	}
}
