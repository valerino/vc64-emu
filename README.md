# vc64-emu
my attempt at a portable Commodore64 emulator.

## current status
### v65xx (cpu emulator)
[README](./v65xx/README.md)

#### TODO
test invalid opcodes (implemented but not tested)

### vc64 (the actual emulator)
* needs [BIOS files](./bios/README.md)
* targets PAL c64
* the emulator boots and BASIC works :)
* CIA1 and CIA2 partially implemented
* VIC-II partially implemented (character and multicolor mode only) 
#### TODO
finish :)

### emushared (helper library)
just generic helpers, if i plan to write other emulators

## global TODO
* split in different repos

## references
* https://www.c64-wiki.com
* https://www.ktverkko.fi/~msmakela/8bit/cbm.html (display)
* https://www.c64-wiki.com/wiki/Memory_Map
* http://www.zimmers.net/cbmpics/cbm/c64/vic-ii.txt (vic)
