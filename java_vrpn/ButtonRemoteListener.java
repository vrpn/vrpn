import vrpn.*;
import java.util.Vector;


public class ButtonRemoteListener
	implements vrpn.ButtonRemote.ButtonChangeListener
{
	Vector buttonUpdates;
	
	boolean returnLastButton;
	
	
	public ButtonRemoteListener(ButtonRemote button)
	{
		button.addButtonChangeListener(this);
		
		buttonUpdates = new Vector();
		
		returnLastButton = true;
	}
	
	
	public void setModeLastButtonUpdate()
	{
		returnLastButton = true;
		
		Object temp = buttonUpdates.lastElement();
		
		buttonUpdates.removeAllElements();
		buttonUpdates.addElement(temp);
	}
	
	
	public void setModeAllButtonUpdates()
	{
		returnLastButton = false;
	}
	
	
	public boolean getModeButtonUpdate()
	{
		return returnLastButton;
	}
	
	
	public Vector getButtonUpdate()
	{
		if (buttonUpdates.isEmpty())
		{
			return null;
		}
		
		if (returnLastButton)
		{
			Vector last = new Vector();
			
			last.addElement(buttonUpdates.lastElement());
			
			return last;
		}
		
		else
		{
			Vector all = new Vector();
			
			for (int i=0; i<buttonUpdates.size(); i++)
			{
				all.addElement(buttonUpdates.elementAt(i));
			}
			
			buttonUpdates.removeAllElements();
			
			return all;
		}
	}
	
	
	public ButtonRemote.ButtonUpdate getLastButtonUpdate()
	{
		ButtonRemote.ButtonUpdate button = (ButtonRemote.ButtonUpdate)(buttonUpdates.lastElement());
		
		return button;
	}
	
	
	public void buttonUpdate (ButtonRemote.ButtonUpdate u, ButtonRemote button)
	{
		if (returnLastButton)
		{
			buttonUpdates.removeAllElements();
		}
		
		buttonUpdates.addElement(u);
	}
}
