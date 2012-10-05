package test.app1;

import java.net.InetAddress;
import java.net.UnknownHostException;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ToggleButton;

import eu.ensam.ii.vrpn.VrpnClient;

public class MainActivity extends Activity {
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    	
        /*
    	 * The VrpnClient must be initialized before setContentView, since the views will 
    	 * trigger Vrpn updates with their initial value upon creation.
    	 */
        
        /*
         * The address of the Vrpn server
         * When running on the emulator, 10.0.2.2 is loopback on the development machine
         */
        InetAddress addr = null;
		try {
			//addr = InetAddress.getByName("192.168.173.1");
			addr = InetAddress.getByName("10.0.2.2");
		} catch (UnknownHostException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		/*
		 * The Vrpn server port as defined in vrpn.cfg, on the line
		 * vrpn_Tracker_JsonNet 7777
		 */
        final int port = 7777;
        VrpnClient.getInstance().setupVrpnServer(addr, port);
        
        
        /*
         *  This layout in in res/layout/main.xml by default
         */
    	setContentView(R.layout.main);
    	
    	/*
    	 * Configure the button that controls the tracker
    	 */
    	final ToggleButton btn = (ToggleButton) findViewById(R.id.btnEnableTiltTracker);
    	btn.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				VrpnClient.getInstance().enableTiltTracker(btn.isChecked());
			}
		});
    }
}