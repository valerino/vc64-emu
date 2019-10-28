# vc64-emu
my attempt at a portable Commodore64 emulator.

## build
~~~bash
# clone this repo
git clone https://github.com/valerino/vc64-emu
cd vc64-emu

# clone also the following repos
# (no submodules on purpose, they're evil and just add complexity, period.)
git clone https://github.com/valerino/emushared
git clone https://github.com/valerino/v65xx

# cd into emushared, follow build instructions in its README.md (including setting up environment variables)
# then cd into v65xx and repeat the process
# finally, get back to vc64-emu folder and build, outputs build/vc-64 executable
mkdir build
cd build
cmake ..
make

# alternatively, you can use the provided buildscript right after cloning all of the repositories:
# perform default (release) build
#./build.sh 
# or alternatively build the debug version
# ./build.sh debug
# or alternatively cleanup the build
# ./build.sh clean
# or alternatively pull remote changes
# ./build.sh update
~~~

## usage
~~~
vc64 - a c64 emulator
        (c)opyleft, valerino, y2k19
usage: ./vc64-emu -f <file> [-dsh]
        -f: file to be loaded (PRG only is supported as now)
        -t: run cpu test in test/6502_functional_test.bin
        -j: 1|2, joystick in port 1 or 2 (default is 0, no joystick. either, arrows=directions, leftshift=fire).
                when joy2 is enabled, press ctrl-j to switch on/off keyboard (due to a dirty hack i used!).
        -d: debugger (if enabled, you may also use ctrl-d to break while running)
        -s: fullscreen
        -c: off|nospr|nobck (to disable hw collisions sprite/sprite, sprite/background, all. default is all collisions enabled)
        -h: this help
~~~

## STATUS
lot of stuff broken and partially implemented, many bugs.

follow my twitter https://twitter.com/valerino for updates, i usually post there about the development.

## references
* https://www.c64-wiki.com
* https://www.ktverkko.fi/~msmakela/8bit/cbm.html (display)
* https://www.c64-wiki.com/wiki/Memory_Map
* http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt (vic)

## Presentation at OUAS19 - Once Upon a Sprite, Milan, 26/10/2019
https://drive.google.com/open?id=1eG1gMYYNwa8CZyks_--0jkTqO06pdEii
