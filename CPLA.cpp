#include "CPLA.h"
#include <bitutils.h>
#include <SDL.h>

CPLA::CPLA() {}
CPLA::~CPLA() {}

bool CPLA::isLoram() { return IS_BIT_SET(_latch, 0); }

bool CPLA::isHiram() { return IS_BIT_SET(_latch, 1); }

bool CPLA::isCharen() { return IS_BIT_SET(_latch, 2); }

bool CPLA::isGame() { return IS_BIT_SET(_latch, 3); }

bool CPLA::isExrom() { return IS_BIT_SET(_latch, 4); }

void CPLA::setGame(bool set) {
    if (set) {
        BIT_SET(_latch, 3);
    } else {
        BIT_CLEAR(_latch, 3);
    }
}

void CPLA::setExrom(bool set) {
    if (set) {
        BIT_SET(_latch, 4);
    } else {
        BIT_CLEAR(_latch, 4);
    }
}

void CPLA::setupMemoryMapping(uint8_t controlPort) {
    /*
        Bit 0 - LORAM: Configures RAM or ROM at $A000-$BFFF (see bankswitching)
        Bit 1 - HIRAM: Configures RAM or ROM at $E000-$FFFF (see bankswitching)
        Bit 2 - CHAREN: Configures I/O or ROM at $D000-$DFFF (see bankswitching)
    */
    // loram
    if (IS_BIT_SET(controlPort, 0)) {
        BIT_SET(_latch, 0);
    } else {
        BIT_CLEAR(_latch, 0);
    }

    // hiram
    if (IS_BIT_SET(controlPort, 1)) {
        BIT_SET(_latch, 1);
    } else {
        BIT_CLEAR(_latch, 1);
    }

    // charen
    if (IS_BIT_SET(controlPort, 2)) {
        BIT_SET(_latch, 2);
    } else {
        BIT_CLEAR(_latch, 3);
    }
}

int CPLA::mapAddressToType(uint16_t address) {
    if (address >= 0 && address <= 0x0fff) {
        // page 0-15
        return PLA_MAP_RAM;
    } else if (address >= 0x1000 && address <= 0x07fff) {
        // page 16-127
        if (_latch >= 16 && _latch <= 23) {
            return PLA_MAP_UNDEFINED;
        }
    } else if (address >= 0x8000 && address <= 0x09fff) {
        // page 128-159
        if ((_latch >= 15 && _latch <= 23) || _latch == 11 || _latch == 7 ||
            _latch == 3) {
            return PLA_MAP_CART_ROM_LO;
        }
    } else if (address >= 0xa000 && address <= 0x0bfff) {
        // page 160-191
        if (_latch == 31 || _latch == 27 || _latch == 15 || _latch == 11) {
            return PLA_MAP_BASIC_ROM;
        }
        if (_latch >= 16 && _latch <= 23) {
            return PLA_MAP_UNDEFINED;
        }
        if (_latch == 7 || _latch == 6 || _latch == 3 || _latch == 2) {
            return PLA_MAP_CART_ROM_HI;
        }
    } else if (address >= 0xc000 && address <= 0x0cfff) {
        // page 192-207
        if (_latch >= 16 && _latch <= 23) {
            return PLA_MAP_UNDEFINED;
        }
    } else if (address >= 0xd000 && address <= 0x0dfff) {
        // page 208-223
        if (_latch == 31 || _latch == 30 || _latch == 29 ||
            (_latch <= 23 && _latch >= 13) || (_latch <= 7 && _latch >= 5)) {
            return PLA_MAP_IO_DEVICES;
        }
        if ((_latch <= 27 && _latch >= 25) || (_latch <= 11 && _latch >= 9) ||
            _latch == 3 || _latch == 2) {
            return PLA_MAP_CHARSET_ROM;
        }
    } else if (address >= 0xe000 && address <= 0x0ffff) {
        // page 224-255
        if (_latch == 31 || _latch == 30 || _latch == 27 || _latch == 26 ||
            _latch == 15 || _latch == 14 || _latch == 11 || _latch == 10 ||
            _latch == 7 || _latch == 6 || _latch == 3 || _latch == 2) {
            return PLA_MAP_KERNAL_ROM;
        } else if (_latch <= 23 && _latch >= 16) {
            return PLA_MAP_CART_ROM_HI;
        }
    }

    // if anything, map ram
    return PLA_MAP_RAM;
}
