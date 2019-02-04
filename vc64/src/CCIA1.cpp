//
// Created by valerino on 19/01/19.
//

#include "CCIA1.h"
#include <SDL.h>

CCIA1::CCIA1(CMOS65xx* cpu) : CCIA(cpu) {
    // set all keys to unpressed state
    for (int i = 0; i < sizeof(_keyboard); i++) {
        _keyboard[i] = false;
    }
}

CCIA1::~CCIA1() {

}

int CCIA1::run(int cycleCount) {
    // TODO: properly implement timer, for now we trigger an irq at every screen refresh
    if (cycleCount <= 0) {
        // trigger an interrupt
        _cpu->irq();
    }
    return 0;
}

void CCIA1::processDc01Read(uint8_t* bt) {
    // read dc00, this holds the row in the keyboard matrix
    // https://www.c64-wiki.com/wiki/Keyboard#Hardware
    uint8_t dc00;
    read(CIA1_REG_DATAPORT_A, &dc00);

    // index in the keyboard array is the c64 scancode (see CInput.cpp sdlKeycodeToc64Scancode(), scancodes goes from 0 to 0x3f)
    // https://www.lemon64.com/forum/viewtopic.php?t=32474
    uint8_t row[8]= {0};
    uint8_t rowByte = ~dc00;
    for (int i=0; i < sizeof(_keyboard); i++) {
        if (_keyboard[i]) {
            // if key is pressed
            uint8_t scancode = i;
            int num = (scancode & 0x3f) >> 3;
            row[num] = row[num] | ( 1 << (scancode & 7));
        }
    }

    // build the value to be returned from a $dc01 read
    uint8_t columnByte = 0;
    for (int i=0; i < 8; i++) {
        bool enabled = ((1 << i) & rowByte) != 0;
        if (enabled) {
            columnByte = columnByte | row[i];
        }
    }
    columnByte = ~columnByte;
    *bt = columnByte;
}

void CCIA1::read(uint16_t address, uint8_t *bt) {
    if (address == CIA1_REG_DATAPORT_B) {
        processDc01Read(bt);
        return;
    }

    // default
    _cpu->memory()->readByte(address, bt);
}

void CCIA1::write(uint16_t address, uint8_t bt) {

    // default
    _cpu->memory()->writeByte(address, bt);
}

void CCIA1::setKeyState(uint8_t scancode, bool pressed) {
    _keyboard[scancode] = pressed;
}
