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
	
	
	public void setModeLastForceUpdate()
	{
		returnLastForce = true;
		
		Object temp = forceUpdates.lastElement();
		
		forceUpdates.removeAllElements();
		forceUpdates.addElement(temp);
	}
	
	
	public void setModeLastForceErrorUpdate()
	{
		returnLastError = true;
		
		Object temp = errorUpdates.lastElement();
		
		errorUpdates.removeAllElements();
		errorUpdates.addElement(temp);
	}
	
	
	public void setModeLastSCPUpdate()
	{
		returnLastSCP = true;
		
		Object temp = SCPUpdates.lastElement();
		
		SCPUpdates.removeAllElements();
		SCPUpdates.addElement(temp);
	}
	
	
	public void setModeAllForceUpdates()
	{
		returnLastForce = false;
	}
	
	
	public void setModeAllForceErrorUpdates()
	{
		returnLastError = false;
	}
	
	
	public void setModeAllSCPUpdates()
	{
		returnLastSCP = false;
	}
	
	
	public boolean getModeForceUpdate()
	{
		return returnLastForce;
	}
	
	
	public boolean getModeForceErrorUpdate()
	{
		return returnLastError;
	}
	
	
	public boolean getModeSCPUpdate()
	{
		return returnLastSCP;
	}
	
	
	public Vector getForceUpdate()
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
	
	
	public Vector getForceErrorUpdate()
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
	
	
	public Vector getSCPUpdate()
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
	
	
	public ForceDeviceRemote.ForceChange getLastForceUpdate()
	{
		ForceDeviceRemote.ForceChange force = (ForceDeviceRemote.ForceChange)(forceUpdates.lastElement());
		
		return force;
	}
	
	
	public ForceDeviceRemote.ForceError getLastErrorUpdate()
	{
		ForceDeviceRemote.ForceError error = (ForceDeviceRemote.ForceError)(errorUpdates.lastElement());
		
		return error;
	}
	
	
	public ForceDeviceRemote.SCPChange getLastSCPUpdate()
	{
		ForceDeviceRemote.SCPChange scp = (ForceDeviceRemote.SCPChange)(SCPUpdates.lastElement());
		
		return scp;
	}
	
	
	public void forceUpdate (ForceDeviceRemote.ForceChange f, ForceDeviceRemote force)
	{
		if (returnLastForce)
		{
			forceUpdates.removeAllElements();
		}
			
		forceUpdates.addElement(f);
	}
	
	
	public void forceError (ForceDeviceRemote.ForceError e, ForceDeviceRemote force)
	{
		if (returnLastError)
		{
			errorUpdates.removeAllElements();
		}
		
		errorUpdates.addElement(e);
	}
	
	
	public void scpUpdate (ForceDeviceRemote.SCPChange s, ForceDeviceRemote force )
	{
		if (returnLastSCP)
		{
			SCPUpdates.removeAllElements();
		}
		
		SCPUpdates.addElement(s);
	}
}
