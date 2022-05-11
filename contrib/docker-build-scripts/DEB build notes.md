
## Debain/Ubuntu build notes

`i686-pc-linux-gnu for Linux 32 bit<br>
x86_64-pc-linux-gnu for x64 Linux<br>
x86_64-w64-mingw32 for Win64<br>
x86_64-apple-darwin16 for macOS<br>
arm-linux-gnueabihf for Linux ARM 32 bit<br>
aarch64-linux-gnu for Linux ARM 64 bit<br>
armv7a-linux-android for Android ARM 32 bit<br>
aarch64-linux-android for Android ARM 64 bit<br>
i686-linux-android for Android x86 32 bit<br>
x86_64-linux-android for Android x86 64 bit`<br>

`apt install dpkg-dev<br>
dpkg-shlibdeps -O path/to/binary/file`<br>
<br>
`sudo dpkg -i package_name.deb<br>
sudo apt -f install`