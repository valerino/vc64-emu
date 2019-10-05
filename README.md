# vc64-emu
my attempt at a portable Commodore64 emulator.

## current status
### v65xx (cpu emulator)
[README](./v65xx/README.md)

### vc64 (the actual emulator)
* needs [BIOS files](./bios/README_bios.md)
* targets PAL c64
* CIA1 and CIA2 partially implemented
* VIC-II partially implemented (character and multicolor mode only)
* timers completely wrong :)

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

[x] usage: ./vc64 -f <file> [-dsh]
        -f: file to be loaded (PRG only is supported as now)
        -t: just run the cpu test, everything else is ignored
        -d: debugger
        -s: fullscreen
        -h: this help
~~~

#### STATUS
lot of stuff, many bugs, many things missing!

follow my twitter https://twitter.com/valerino for updates, i usually post there about the development
https://twitter.com/valerino

### emushared (helper library)
just generic helpers, if i plan to write other emulators

## global TODO
* split in different repos

## references
* https://www.c64-wiki.com
* https://www.ktverkko.fi/~msmakela/8bit/cbm.html (display)
* https://www.c64-wiki.com/wiki/Memory_Map
* http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt (vic)
