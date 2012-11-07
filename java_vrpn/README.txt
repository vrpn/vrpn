For notes on compiling under Android, see the "README_Android.txt" file

Notes on this java implementation (mixed Windows and Irix/Linux notes):

there are three main steps to building the java wrapping of vrpn.
note that this only wraps the high-level remote device classes.
you should have built vrpn before proceeding with these steps.

1) compile the java files
2) generate the native headers
3) compile the java native code


1) compile the java files (resulting in java_vrpn.jar)
	if you use eclipse, import the project in this directory and build.
	execute the ANT file buildJAR.xml.

	otherwise, this can be done on the command line with the
	'javac' and 'jar' tools.
	

2) generate the native headers (resulting in several files vrpn_*.h)
	if you use eclipse, execute the ANT file buildNativeHeaders.xml

	alternatively, from a unix-like command line, execute make_header.sh


3) compile the native java code (resulting in java_vrpn.dll or 
   libjava_vrpn.so)
	if you use visual studio 2005 (vc8), load java_vrpn.sln and build.
	if you use visual studio 6, load java_vrpn.dsw and build.
	if you use a unix-like command line, use the makefile.

==========================================================
This library needs to be in your path or executable directory.  Copy 
java_vrpn.dll / libjava_vrpn.so into the test directory (for instance).  
If not, you will get this error message: 
			no java_vrpn in java.library.path
			Error initializing java_vrpn.
 			-- Unable to find native library.
you can use the -D parmeter in java to tell the path to the native library,
e.g. java -Djava.library.path=$PWD ButtonTest

the resulting vrpn.jar file needs to be in your CLASSPATH or moved to
be in your CLASSPATH.  Alternatively, you can invoke the Java
VM with the classpath of the java_vrpn directory.

