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


public class ForceDeviceRemoteListener
	implements vrpn.ForceDeviceRemote.ForceChangeListener,
				vrpn.ForceDeviceRemote.ForceErrorListener,
				vrpn.ForceDeviceRemote.SCPChangeListener
{
	Vector forceUpdates;
	Vector errorUpdates;
	Vector SCPUpdates;
	
	boolean returnLastForce;
	boolean returnLastError;
	boolean returnLastSCP;
	
	
	public ForceDeviceRemoteListener(ForceDeviceRemote force)
	{
		force.addForceChangeListener(this);
		force.addForceErrorListener(this);
		force.addSCPChangeListener(this);
		
		forceUpdates = new Vector();
		errorUpdates = new Vector();
		SCPUpdates = new Vector();
		
		returnLastForce = true;
		returnLastError = true;
		returnLastSCP = true;
	}
	
	
	//** Empty the vector when the mode is set to last
	public synchronized void setModeLastForceUpdate()
	{
		returnLastForce = true;
		
		if (!forceUpdates.isEmpty())
		{
			Object temp = forceUpdates.lastElement();
		
			forceUpdates.removeAllElements();
			forceUpdates.addElement(temp);
		}
	}
	
	
	public synchronized void setModeLastForceErrorUpdate()
	{
		returnLastError = true;
		
		if (!errorUpdates.isEmpty())
		{
			Object temp = errorUpdates.lastElement();
		
			errorUpdates.removeAllElements();
			errorUpdates.addElement(temp);
		}
	}
	
	
	public synchronized void setModeLastSCPUpdate()
	{
		returnLastSCP = true;
		
		if (!SCPUpdates.isEmpty())
		{
			Object temp = SCPUpdates.lastElement();
		
			SCPUpdates.removeAllElements();
			SCPUpdates.addElement(temp);
		}
	}
	
	
	public synchronized void setModeAllForceUpdates()
	{
		returnLastForce = false;
	}
	
	
	public synchronized void setModeAllForceErrorUpdates()
	{
		returnLastError = false;
	}
	
	
	public synchronized void setModeAllSCPUpdates()
	{
		returnLastSCP = false;
	}
	
	
	public synchronized boolean getModeForceUpdate()
	{
		return returnLastForce;
	}
	
	
	public synchronized boolean getModeForceErrorUpdate()
	{
		return returnLastError;
	}
	
	
	public synchronized boolean getModeSCPUpdate()
	{
		return returnLastSCP;
	}
	
	
	//** Empty the vector when all the updates are returned
	public synchronized Vector getForceUpdate()
	{
		if (forceUpdates.isEmpty())
		{
			return null;
		}
		
		if (returnLastForce)
		{
			Vector last = new Vector();
			
			last.addElement(forceUpdates.lastElement());
			
			return last;
		}
		
		else
		{
			Vector all = new Vector();
			
			for (int i=0; i<forceUpdates.size(); i++)
			{
				all.addElement(forceUpdates.elementAt(i));
			}
			
			forceUpdates.removeAllElements();
			
			return all;
		}
	}
	
	
	public synchronized Vector getForceErrorUpdate()
	{
		if (errorUpdates.isEmpty())
		{
			return null;
		}
		
		if (returnLastError)
		{
			Vector last = new Vector();
			
			last.addElement(errorUpdates.lastElement());
			
			return last;
		}
		
		else
		{
			Vector all = new Vector();
			
			for (int i=0; i<errorUpdates.size(); i++)
			{
				all.addElement(errorUpdates.elementAt(i));
			}
			
			errorUpdates.removeAllElements();
			
			return all;
		}
	}
	
	
	public synchronized Vector getSCPUpdate()
	{
		if (SCPUpdates.isEmpty())
		{
			return null;
		}
		
		if (returnLastSCP)
		{
			Vector last = new Vector();
			
			last.addElement(SCPUpdates.lastElement());
			
			return last;
		}
		
		else
		{
			Vector all = new Vector();
			
			for (int i=0; i<SCPUpdates.size(); i++)
			{
				all.addElement(SCPUpdates.elementAt(i));
			}
			
			SCPUpdates.removeAllElements();
			
			return all;
		}
	}
	
	
	public synchronized ForceDeviceRemote.ForceChange getLastForceUpdate()
	{
		ForceDeviceRemote.ForceChange force = (ForceDeviceRemote.ForceChange)(forceUpdates.lastElement());
		
		return force;
	}
	
	
	public synchronized ForceDeviceRemote.ForceError getLastErrorUpdate()
	{
		ForceDeviceRemote.ForceError error = (ForceDeviceRemote.ForceError)(errorUpdates.lastElement());
		
		return error;
	}
	
	
	public synchronized ForceDeviceRemote.SCPChange getLastSCPUpdate()
	{
		ForceDeviceRemote.SCPChange scp = (ForceDeviceRemote.SCPChange)(SCPUpdates.lastElement());
		
		return scp;
	}
	
	
	//** Keep only the last update if the mode is set to last
	public synchronized void forceUpdate (ForceDeviceRemote.ForceChange f, ForceDeviceRemote force)
	{
		if (returnLastForce)
		{
			forceUpdates.removeAllElements();
		}
			
		forceUpdates.addElement(f);
	}
	
	
	public synchronized void forceError (ForceDeviceRemote.ForceError e, ForceDeviceRemote force)
	{
		if (returnLastError)
		{
			errorUpdates.removeAllElements();
		}
		
		errorUpdates.addElement(e);
	}
	
	
	public synchronized void scpUpdate (ForceDeviceRemote.SCPChange s, ForceDeviceRemote force )
	{
		if (returnLastSCP)
		{
			SCPUpdates.removeAllElements();
		}
		
		SCPUpdates.addElement(s);
	}
}
