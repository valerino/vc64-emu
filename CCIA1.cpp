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

    switch (addr) {
    case 0xdc00:
    case 0xdc01: {
        // PRA ($dc00/rows) and PRB ($dc01/columns)
        // PRA also controls joy2, while PRB joy1
        // read keyboard matrix row in data port A
        uint8_t pra = readPRA();
        if (addr == 0xdc00) {
            // we're reading PRA (joy2)
            if (_joy2Hack) {
                // we want to intepret joy2 pressing as joy1 pressing, via
                // keyboard joy1 signals
                // clear joy2 bits (1=not pressed)
                BIT_SET(pra, 0);
                BIT_SET(pra, 1);
                BIT_SET(pra, 2);
                BIT_SET(pra, 3);
                BIT_SET(pra, 4);
            } else {
                // default PRA read
                *bt = pra;
                return;
            }
        }

        // read column from port B, using row in port A
        // (some docs refer to columns in port A and rows in port B, though
        // ..... but it's still the same)
        readKeyboardMatrixColumn(bt, pra);
        break;
    }

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
        CCIABase::write(address, bt);
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
