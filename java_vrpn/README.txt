For notes on compiling under Android, see the "README_Android.txt" file

You can ignore this file if you use CMake, as it handles all this for you.

Notes on this java implementation (mixed Windows and Irix/Linux notes):

there are three main steps to building the java wrapping of vrpn.
note that this only wraps the high-level remote device classes.
you should have built vrpn before proceeding with these steps.

1) compile the java files and make the jar (resulting in `vrpn.jar`)
2) generate the native headers (resulting in several files `vrpn_*.h`)
3) compile the java native code (resulting in java_vrpn.dll or 
   libjava_vrpn.so)

Two general approaches to the Java parts (1 and 2): one requires a Unix-like
command line but is good for automation, while the other involves either
a command line or Eclipse, plus ant.

1. Java Parts
    - command line: run `sh make_header.sh` to compile the Java files, 
      make `vrpn.jar`, and generate native headers in one step.
    - Java-style:
        1. Compile the Java files. You can either build the Eclipse project
           or run `mkdir bin; find vrpn -name "*.java" | xargs javac -d bin`
           on the command line.
        2. Make the JAR. This can be done with `ant -f buildJAR.xml`
        3. Generate the native headers. Do this with
           `ant -f buildNativeHeaders.xml`

2. Native Code Parts
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
