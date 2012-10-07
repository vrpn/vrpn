package eu.ensam.ii.vrpn;

/**
 * This class must be used as an {@link android.app.Application Application} for applications that use the Vrpn widgets.
 * <p>

 * An application that wants to use the Vrpn widgets must define an
 * <em>application</em> tag with the name of this class in its manifest :
 * </p>
 * <ul>
 * <li>edit your application <em>AndroidManifext.xml</em> and open the <em>Application</em> tab</li>
 * <li>in the <em>Application toggle</em> section , check
 * <em>Define an &lt;application&gt; tag ...</em></li>
 * <li>in the <em>Application Attributes</em> section, type <em>eu.ensam.ii.vrpn.VrpnApplication</em> as the value of the
 * <em>Name</em> attribute or use the <em>Browse...<em>button</li>
 * </ul>
 * 
 */
public class VrpnApplication extends android.app.Application {
	/*
	 * The VrpnApplication object holds and initializes an instance of VrpnClient. 
	 */
	private VrpnClient _vrpnClient;

	@Override
	public void onCreate() {
		_vrpnClient = VrpnClient.getInstance();
		_vrpnClient.init(getApplicationContext());
	}

}
