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
	
	
	public void setModeLastTrackerUpdate()
	{
		returnLastTracker = true;
		
		Object temp = trackerUpdates.lastElement();
		
		trackerUpdates.removeAllElements();
		trackerUpdates.addElement(temp);
	}
	
	
	public void setModeLastVelocityUpdate()
	{
		returnLastVelocity = true;
		
		Object temp = velocityUpdates.lastElement();
		
		velocityUpdates.removeAllElements();
		velocityUpdates.addElement(temp);
	}
	
	
	public void setModeLastAccelerationUpdate()
	{
		returnLastAcceleration = true;
		
		Object temp = accelerationUpdates.lastElement();
		
		accelerationUpdates.removeAllElements();
		accelerationUpdates.addElement(temp);
	}
	
	
	public void setModeAllTrackerUpdates()
	{
		returnLastTracker = false;
	}
	
	
	public void setModeAllVelocityUpdates()
	{
		returnLastVelocity = false;
	}
	
	
	public void setModeAllAccelerationUpdates()
	{
		returnLastAcceleration = false;
	}
	
	
	public boolean getModeTrackerUpdate()
	{
		return returnLastTracker;
	}
	
	
	public boolean getModeVelocityUpdate()
	{
		return returnLastVelocity;
	}
	
	
	public boolean getModeAccelerationUpdate()
	{
		return returnLastAcceleration;
	}
	
	
	public Vector getTrackerUpdate()
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
	
	
	public Vector getVelocityUpdate()
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
	
	
	public Vector getAcclerationUpdate()
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
	
	
	public TrackerRemote.TrackerUpdate getLastTrackerUpdate()
	{
		TrackerRemote.TrackerUpdate tracker = (TrackerRemote.TrackerUpdate)(trackerUpdates.lastElement());
		
		return tracker;
	}
	
	
	public TrackerRemote.VelocityUpdate getLastVelocityUpdate()
	{
		TrackerRemote.VelocityUpdate vel = (TrackerRemote.VelocityUpdate)(velocityUpdates.lastElement());
		
		return vel;
	}
	
	
	public TrackerRemote.AccelerationUpdate getLastAccelerationUpdate()
	{
		TrackerRemote.AccelerationUpdate acc = (TrackerRemote.AccelerationUpdate)(accelerationUpdates.lastElement());
		
		return acc;
	}
	
	
	public void trackerPositionUpdate (TrackerRemote.TrackerUpdate u, TrackerRemote tracker)
	{
		if (returnLastTracker)
		{
			trackerUpdates.removeAllElements();
		}
			
		trackerUpdates.addElement(u);
	}
	
	
	public void trackerVelocityUpdate (TrackerRemote.VelocityUpdate v, TrackerRemote tracker)
	{
		if (returnLastVelocity)
		{
			velocityUpdates.removeAllElements();
		}
		
		velocityUpdates.addElement(v);
	}
	
	
	public void trackerAccelerationUpdate (TrackerRemote.AccelerationUpdate a, TrackerRemote tracker )
	{
		if (returnLastAcceleration)
		{
			accelerationUpdates.removeAllElements();
		}
		
		accelerationUpdates.addElement(a);
	}
}
