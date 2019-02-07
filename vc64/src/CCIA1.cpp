//
// Created by valerino on 19/01/19.
//

#include "CCIA1.h"
#include <SDL.h>
#include <bitutils.h>

#ifndef NDEBUG
// debug-only flag
//#define DEBUG_CIA1
#endif

CCIA1::CCIA1(CMOS65xx* cpu) {
    // set all keys to unpressed state
    for (int i = 0; i < sizeof(_keyboard); i++) {
        _keyboard[i] = false;
    }
    _timerA = 0;
    _timerB = 0;
    _timerARunning = false;
    _timerBRunning = false;
    _cpu = cpu;
}

CCIA1::~CCIA1() {

}

int CCIA1::update(int cycleCount) {
    if (_timerARunning) {
        _timerA--;
        if (_timerA == 0) {
            // timer A expired
            _cpu->irq();
        }
    }
    if (_timerBRunning) {
        _timerB--;
        if (_timerB == 0) {
            // timer B expired
            _cpu->irq();
        }
    }
    return 0;
}

void CCIA1::processDc01Read(uint8_t* bt) {
    // read dc00, this holds the row in the keyboard matrix
    // https://www.c64-wiki.com/wiki/Keyboard#Hardware
    // https://sites.google.com/site/h2obsession/CBM/C128/keyboard-scan
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
    // check shadow
    uint16_t addr = checkShadowAddress(address);
    switch (addr) {
        case CIA1_REG_DATAPORT_B:
            // read keyboard matrix column
            processDc01Read(bt);
            return;

        default:
            break;
    }

    // finally read
    _cpu->memory()->readByte(addr, bt);
}

void CCIA1::write(uint16_t address, uint8_t bt) {
    // check shadow
    uint16_t addr = checkShadowAddress(address);

    switch (addr) {
        case CIA1_REG_TIMER_A_LO:
            _timerA = (_timerA & 0xff00) | bt;
            break;

        case CIA1_REG_TIMER_A_HI:
            _timerA = (_timerA & 0xff) | ( bt << 8);
            break;

        case CIA1_REG_TIMER_B_LO:
            _timerB = (_timerB & 0xff00) | bt;
            break;

        case CIA1_REG_TIMER_B_HI:
            _timerB = (_timerB & 0xff) | ( bt << 8);
            break;

        case CIA1_REG_CONTROL_TIMER_A:
            if (IS_BIT_SET(bt, 0)) {
                _timerARunning = true;
            }
            else {
                _timerARunning = false;
            }
            break;

        case CIA1_REG_CONTROL_TIMER_B:
            if (IS_BIT_SET(bt, 0)) {
                _timerBRunning = true;
            }
            else {
                _timerBRunning = false;
            }
            break;

        default:
            break;
    }

    // finally write
    _cpu->memory()->writeByte(addr, bt);
}

void CCIA1::setKeyState(uint8_t scancode, bool pressed) {
    _keyboard[scancode] = pressed;
}

/**
 * some addresses are shadowed and maps to other addresses
 * @param address the input address
 * @return the effective address
 */
uint16_t CCIA1::checkShadowAddress(uint16_t address) {
    // check for shadow addresses
    if (address >= 0xdc10 && address <= 0xdcff) {
        // these are shadows for $dc00-$dc10
        return (CIA1_REGISTERS_START + ((address % CIA1_REGISTERS_START) % 0x10));
    }
    return address;
}
