# v65xx
my MOS 6502/6510/65xxx emulator/interpreter!

# features
* clean code
    * since i myself am a newbie in writing emulators, i've explicitly choosen to prefer clean to performant code!

* passes the Klaus's functional tests from 6502.org!
    * https://github.com/Klaus2m5/6502_65C02_functional_tests
    * latest version as 1/2019 included in this repo
    * to test, enable *DEBUG_RUN_FUNCTIONAL_TESTS* in *include/CMOS65xx.h*
* emulates all undocumented instructions
    * TODO: implemented, but still to be tested!
    
* cycle accurate

* built-in debugger
    * call step() with debugging=true (and possibly with forceDebugging=true to interrupt while running, i.e. with an hotkey)
    * while in debugging mode use 'h' for help on commands

## references
* http://archive.6502.org/datasheets/mos_6510_mpu.pdf
* http://archive.6502.org/datasheets/rockwell_r650x_r651x.pdf (rockwell 65xx, clearer version of the original above)
* http://www.atarihq.com/danb/files/64doc.txt
* http://www.oxyron.de/html/opcodes02.html
* https://www.masswerk.at/6502/6502_instruction_set.html
* https://wiki.nesdev.com/w/index.php/Status_flags
* http://www.6502.org/tutorials/decimal_mode.html
* http://www.6502.org/tutorials/6502opcodes.html
* http://obelisk.me.uk/6502/reference.html
* http://www.thealmightyguru.com/Games/Hacking/Wiki/index.php/6502_Opcodes
* http://www.ffd2.com/fridge/docs/6502-NMOS.extra.opcodes (_best source for undocumented instructions implementation!_)