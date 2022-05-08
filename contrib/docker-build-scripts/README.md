### Install Docker, follow the instructions https://docs.docker.com/get-docker/

### Grab the code
`git clone https://github.com/xuez/Xuez-Core.git && cd Xuez-Core`<br>
or<br>
`git clone https://github.com/xuez/Xuez-Core.git -b $branch && cd Xuez-Core`

### Build scripts are:
`bash contrib/docker-build-scripts/build_linux_64.sh` <i>will build the Linux x64 version</i><br>
`bash contrib/docker-build-scripts/build_apk_32.sh`<br>
`bash contrib/docker-build-scripts/build_apk_64.sh`<br>
`bash contrib/docker-build-scripts/build_apk_64x86.sh`<br>
`bash contrib/docker-build-scripts/build_ARM_linux_32.sh` <i>Raspberry Pi and other SOCs with 32bit OS</i><br>
`bash contrib/docker-build-scripts/build_ARM_linux_64.sh` <i>Raspberry Pi and other SOCs with 64bit OS</i><br>
`bash contrib/docker-build-scripts/build_Win_32.sh`<br>
`bash contrib/docker-build-scripts/build_Win_64.sh`<br>

### macOS build
#### Use homebrew, and BigSur
`brew install libtool automake cmake boost libevent librsvg berkeley-db@4 qt@5`<br>
**boost version is 1.78.0 at the time this doc was created**

### build for x64 on BigSur
`./configure --disable-tests --disable-gui-tests --disable-bench --enable-reduce-exports --disable-dependency-tracking \
--enable-c++17 --with-gui=qt5 --with-boost=/usr/local/Cellar/boost/1.78.0_1 --with-boost-libdir=/usr/local/Cellar/boost/1.78.0_1/lib \
CXXFLAGS="-pipe -O2 -std=c++17 -Wno-unused-variable -Wno-unused-parameter -Wno-unused-command-line-argument -I/usr/local/Cellar/boost/1.78.0_1include -I/usr/local/Cellar/libevent/2.1.12/include"`

`make -j4 && make deploy`

`mv src/xuezd src/xuez-cli src/xuez-wallet src/xuez-tx . && strip xuezd xuez-cli xuez-wallet xuez-tx`
`tar czf xuez-v$VERSION-macOS-cli.tgz xuezd xuez-cli xuez-wallet xuez-tx && rm xuezd xuez-cli xuez-wallet xuez-tx`

### for macOS ARM/M1 Build on Monterey

`./configure --disable-tests --disable-gui-tests --disable-bench --enable-reduce-exports --disable-dependency-tracking \
--enable-c++17 --with-gui=qt5 --with-boost=/opt/homebrew/Cellar/boost/1.78.0_1 --with-boost-libdir=/opt/homebrew/Cellar/boost/1.78.0_1/lib \
CXXFLAGS="-pipe -O2 -std=c++17 -Wno-unused-variable -Wno-unused-parameter -Wno-unused-command-line-argument -I/opt/homebrew/Cellar/boost/1.78.0_1/include -I/opt/homebrew/Cellar/libevent/2.1.12/include"`

`make -j4 && make deploy`

### Sign the Android APKs with - https://github.com/patrickfav/uber-apk-signer
`java -jar ~/uber-apk-signer-1.2.1.jar --ks ~/Documents/xuez-apk-key/xuez-key --ksAlias xuez --apks .`
