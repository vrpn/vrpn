# Notes from Andrew Montag on how to compile VRPN for the Android, modified to remove
# his patch file (which is no longer needed because the changes have been pulled
# into the main source tree).  If you have the Android NDK in ~/Downloads and you
# have the directories created, you can run this as a shell script.

export ANDROID_SDK=$HOME/android-sdk-linux/

# install NDK
cd ~
tar xjvf ~/Downloads/android-ndk-r8b-linux-x86.tar.bz2
export ANDROID_NDK=$HOME/android-ndk-r8b
export NDK=$HOME/android-ndk-r8b

# get android-cmake
cd ~/vrac
hg clone https://code.google.com/p/android-cmake/
export ANDROID_CMAKE=$HOME/vrac/android-cmake

# make ndk tookchain
$NDK/build/tools/make-standalone-toolchain.sh --platform=android-5 --install-dir=$HOME/android-toolchain

export ANDROID_NDK_TOOLCHAIN_ROOT=$HOME/android-toolchain
export ANDTOOLCHAIN=$ANDROID_CMAKE/toolchain/android.toolchain.cmake

alias android-cmake='cmake -DCMAKE_TOOLCHAIN_FILE=$ANDTOOLCHAIN '

# download
cd ~/vrac/vrpn_
git clone git://git.cs.unc.edu/vrpn.git
cd vrpn

# compile
mkdir build
cd build
android-cmake .. -DVRPN_BUILD_JAVA=1 -DVRPN_BUILD_SERVER_LIBRARY=0 -DCMAKE_CXX_FLAGS=-fpermissive -DVRPN_BUILD_CLIENT_LIBRARY=1

