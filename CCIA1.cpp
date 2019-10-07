//
// Created by valerino on 19/01/19.
//

#include "CCIA1.h"
#include <SDL.h>
#include "bitutils.h"

CCIA1::CCIA1(CMOS65xx *cpu)
    : CCIABase::CCIABase(cpu, CIA1_REGISTERS_START, CIA_TRIGGERS_IRQ) {}

/**
 * read keyboard matrix column
 * https://www.c64-wiki.com/wiki/Keyboard#Hardware
 * https://sites.google.com/site/h2obsession/CBM/C128/keyboard-scan
 * @param bt on successful return, the column byte
 * @param pra value of PRA ($dc00)
 */
void CCIA1::readKeyboardMatrixColumn(uint8_t *bt, uint8_t pra) {
    // c64 scancode is the index in the keyboard array (see CInput.cpp
    // sdlKeycodeToc64Scancode(), scancodes goes from 0 to 0x3f)
    // https://www.lemon64.com/forum/viewtopic.php?t=32474
    uint8_t row[8] = {0};
    uint8_t rowByte = ~pra;
    for (int i = 0; i < sizeof(_kbMatrix); i++) {
        if (_kbMatrix[i]) {
            // if key is pressed
            uint8_t scancode = i;
            int num = (scancode & 0x3f) >> 3;
            row[num] = row[num] | (1 << (scancode & 7));
        }
    }

    // build the value to be returned from a $dc01 read
    uint8_t columnByte = 0;
    for (int i = 0; i < 8; i++) {
        bool enabled = ((1 << i) & rowByte) != 0;
        if (enabled) {
            columnByte = columnByte | row[i];
        }
    }
    columnByte = ~columnByte;
    *bt = columnByte;
}

void CCIA1::enableJoy2Hack(bool enable) { _joy2Hack = enable; }

void CCIA1::read(uint16_t address, uint8_t *bt) {
    // in CIA1 some addresses d010-d0ff are repeated every 16 bytes
    uint16_t addr = checkShadowAddress(address);
    uint8_t pra;

    switch (addr) {
    case 0xdc00:
        // PRA
        if (_joy2Hack) {
            // read keyboard matrix row in data port A, thus emulating joy 2
            // (keyboard may got deactivated by code)
            pra = readPRA();

            // read column
            readKeyboardMatrixColumn(bt, pra);
        } else {
            // default
            CCIABase::read(addr, bt); //_cpu->memory()->readByte(0xdc00, &pra);
        }
        break;

    case 0xdc01:
        // PRB
        // read keyboard matrix row in data port A
        pra = readPRA();

        // read column
        readKeyboardMatrixColumn(bt, pra);
        break;

    default:
        // default processing with the base class
        CCIABase::read(addr, bt);
    }
}

void CCIA1::write(uint16_t address, uint8_t bt) {
    // in CIA1 some addresses d010-d0ff are repeated every 16 bytes
    uint16_t addr = checkShadowAddress(address);
    switch (addr) {

    default:
        break;
    }

    // finally write
    CCIABase::write(address, bt);
}

void CCIA1::setKeyState(uint8_t scancode, bool pressed) {
    _kbMatrix[scancode] = pressed;
}

/**
 * @brief certain addresses in cia1 are mirrored every 16 bytes
 * @param address the input address
 * @return the effective address
 */
uint16_t CCIA1::checkShadowAddress(uint16_t address) {
    // check for shadow addresses
    if (address >= 0xdc10 && address <= 0xdcff) {
        // these are shadows for $dc00-$dc10
        return (CIA1_REGISTERS_START +
                ((address % CIA1_REGISTERS_START) % 0x10));
    }
    return address;
}
