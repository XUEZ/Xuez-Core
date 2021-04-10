
sudo apt install docker.io bash
sudo usermod -a -G docker dave

## reload users/group
sudo su $(who)

git clone https://github.com/xuez/Xuez-Core.git && cd Xuez-Core

Build scripts are:

contrib/docker-build-scripts/build_apk_32.sh
contrib/docker-build-scripts/build_apk_64.sh
contrib/docker-build-scripts/build_apk_64x86.sh
contrib/docker-build-scripts/build_ARM_linux_32.sh
contrib/docker-build-scripts/buildARMlinux_32.sh
contrib/docker-build-scripts/build_ARM_linux_64.sh
contrib/docker-build-scripts/build_linux_64.sh
contrib/docker-build-scripts/build_Win_32.sh
contrib/docker-build-scripts/build_Win_64.sh


## macOS build
Use homebrew
brew install qt automake cmake boost libevent

./configure --disable-tests --disable-gui-tests --disable-bench --enable-reduce-exports --disable-dependency-tracking --enable-c++17 --with-gui=qt5 --with-boost=/usr/local/Cellar/boost/1.71.0 --with-boost-libdir=/usr/local/Cellar/boost/1.71.0/lib CXXFLAGS="-pipe -O2 -std=c++17 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk \
-mmacosx-version-min=10.11 -Wno-unused-variable -Wno-unused-parameter -I/usr/local/Cellar/boost/1.71.0/include -I/usr/local/Cellar/libevent/2.1.12/include"

make -j4 && make deploy

mv src/xuezd src/xuez-cli src/xuez-wallet src/xuez-tx . && strip xuezd xuez-cli xuez-wallet xuez-tx
tar czf xuez-v$VERSION-macOS-cli.tgz xuezd xuez-cli xuez-wallet xuez-tx && rm xuezd xuez-cli xuez-wallet xuez-tx


Sign the Android APKs with - https://github.com/patrickfav/uber-apk-signer
java -jar ~/uber-apk-signer-1.2.1.jar --ks ~/Documents/xuez-apk-key/xuez-key --ksAlias xuez --apks .
