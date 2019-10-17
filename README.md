# vc64-emu
my attempt at a portable Commodore64 emulator.

## current status
### v65xx (cpu emulator)
[README](./v65xx/README.md)

### vc64 (the actual emulator)
* needs [BIOS files](./bios/README_bios.md)
* targets PAL c64

#### build
~~~bash
# outputs build/vc-64
mkdir build
cd build
cmake ..
make
~~~

#### usage
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

#### STATUS
lot of stuff broken and partially implemented, many bugs.

follow my twitter https://twitter.com/valerino for updates, i usually post there about the development.

### emushared (helper library)
just generic helpers, if i plan to write other emulators

## global TODO
* split in different repos

## references
* https://www.c64-wiki.com
* https://www.ktverkko.fi/~msmakela/8bit/cbm.html (display)
* https://www.c64-wiki.com/wiki/Memory_Map
* http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt (vic)
