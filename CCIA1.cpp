//
// Created by valerino on 19/01/19.
//

#include "CCIA1.h"
#include <SDL.h>
#include "bitutils.h"

CCIA1::CCIA1(CMOS65xx *cpu, CPLA *pla)
    : CCIABase::CCIABase(cpu, pla, CIA1_REGISTERS_START, CIA_TRIGGERS_IRQ) {}

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

    // PRA ($dc00/rows) and PRB ($dc01/columns)
    // PRA also controls joy2, while PRB joy1
    // read keyboard matrix row in data port A
    switch (addr) {
    case 0xdc00:
        // PRA (controls joy2, among the other things. also controls keyboard
        // matrix rows)
        if (_joy2Hack) {
            // we want to intepret joy1 pressing as joy2 pressing, via
            // keyboard joy1 shortcuts
            // clear joy2 bits (1=not pressed), so joy2 results untouched
            BIT_SET(_prA, 0);
            BIT_SET(_prA, 1);
            BIT_SET(_prA, 2);
            BIT_SET(_prA, 3);
            BIT_SET(_prA, 4);

            // read keyboard instead, will detect eventually pressed joy1
            // shortcuts
            readKeyboardMatrixColumn(bt, _prA);
        } else {
            // default PRA read, no joy2 hack
            *bt = _prA;
            return;
        }
        break;

    case 0xdc01:
        // PRB (controls joy1, among the other things. also controls keyboard
        // matrix columns)
        // we will calculate columns using rows in PRA (some docs refer to
        // columns in port A and rows in port B, though... concept is the same)
        if (_joy2Hack) {
            // read PRB
            readKeyboardMatrixColumn(bt, _prA);

            // clear eventually present joystick 1 reads, then (or, both joy1
            // and 2 would result pressed at the same time)
            BIT_SET(*bt, 0);
            BIT_SET(*bt, 1);
            BIT_SET(*bt, 2);
            BIT_SET(*bt, 3);
            BIT_SET(*bt, 4);

            // return patched PRB read, with joy1 excluded
            _prB = *bt;
        } else {
            // default PRV read, no joy2 hack
            readKeyboardMatrixColumn(bt, _prA);
        }
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
        // default processing with the base class
        // @fixme: i think here is 'addr', but putting 'address' makes gorf
        // work......
        CCIABase::write(addr, bt);
        break;
    }
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
    uint16_t addr = CIA1_REGISTERS_START + (address & 0xf);
    return addr;
}
