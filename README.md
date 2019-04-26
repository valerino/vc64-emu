# vc64-emu
my attempt at a portable Commodore64 emulator.

## current status
### v65xx (cpu emulator)
[README](./v65xx/README.md)

### vc64 (the actual emulator)
* needs [BIOS files](./bios/README.md)
* targets PAL c64
* CIA1 and CIA2 partially implemented
* VIC-II partially implemented (character and multicolor mode only)

#### usage
~~~
vc64 - a c64 emulator
(c)opyleft, valerino, y2k19

[x] usage: ./vc64 -f <file> [-dsh]
        -f: file to be loaded (PRG only is supported as now)
        -d: debugger
        -s: fullscreen
        -h: this help
~~~

#### STATUS
* CIA1 and CIA2 partially implemented
* VIC-II partially implemented (character and multicolor mode only)
  * basic and multicolor mode works
  
    https://twitter.com/valerino/status/1092400004080328704?s=20
  
    https://twitter.com/valerino/status/1094047231068516352?s=20

* here's the emulator attempting to run ROX64 (llamasoft):
  
  https://twitter.com/valerino/status/1113904347157553152?s=20

* and here attempting to run AMC/Matrix (llamasoft):
  
  https://twitter.com/valerino/status/1114051388542205952?s=20

(yep ... it started as a Llamasoft tribute!)

#### TODO
implement sprites in VIC-II, fix banking (see the 2nd tweet), fix the timing in CIA1, loooot more :)

### emushared (helper library)
just generic helpers, if i plan to write other emulators

## global TODO
* split in different repos

## references
* https://www.c64-wiki.com
* https://www.ktverkko.fi/~msmakela/8bit/cbm.html (display)
* https://www.c64-wiki.com/wiki/Memory_Map
* http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt (vic)
