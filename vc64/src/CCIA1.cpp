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

CCIA1::CCIA1(CMOS65xx *cpu) { _cpu = cpu; }

CCIA1::~CCIA1() {}

int CCIA1::update(int cycleCount) {
    if (_timerARunning) {
        // check timerA mode
        if (_timerAMode == CIA_TIMER_COUNT_CPU_CYCLES) {
            _timerA = _timerA - (cycleCount - _prevCycleCount);
            if (_timerA <= 0) {
                if (_timerAIrqEnabled) {
                    // trigger irq
                    _cpu->irq();
                }
                _timerA = _timerALatch;
            }
        } else {
            // TODO: mode is count slopes at CNT pin
        }
    }
    if (_timerBRunning) {
        switch (_timerBMode) {
            case CIA_TIMER_COUNT_CPU_CYCLES:
                _timerB = _timerB - (cycleCount - _prevCycleCount);
                if (_timerB <= 0) {
                    if (_timerBIrqEnabled) {
                        // trigger irq
                        _cpu->irq();
                    }
                    _timerB = _timerBLatch;
                }
                break;
                // TODO: handle other modes
            default:
                break;
        }
    }
    _prevCycleCount = cycleCount;
    return 0;
}

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

void CCIA1::read(uint16_t address, uint8_t *bt) {
    // check shadow
    uint16_t addr = checkShadowAddress(address);
    switch (addr) {
        // PRB: data port B
        case 0xdc01:
            // read keyboard matrix row in data port A (PRA=$dc00)
            uint8_t pra;
            _cpu->memory()->readByte(0xdc00, &pra);

            // read column
            readKeyboardMatrixColumn(bt, pra);
            break;

            // TA LO: timer A lowbyte
        case 0xdc04:
            *bt = _timerA & 0xff;
            break;

            // TA HI: timer A hibyte
        case 0xdc05:
            *bt = (_timerA & 0xff00) >> 8;
            break;
            // TB LO: timer B lobyte
        case 0xdc06:
            *bt = _timerB & 0xff;
            break;

            // TB HI: timer B hibyte
        case 0xdc07:
            *bt = (_timerB & 0xff00) >> 8;
            break;

        default:
            // read memory
            _cpu->memory()->readByte(addr, bt);
    }
}

void CCIA1::write(uint16_t address, uint8_t bt) {
    // check shadow
    uint16_t addr = checkShadowAddress(address);

    switch (addr) {
        // TA LO: timer A lowbyte (set latch LO)
        case 0xdc04:
            _timerALatch = (_timerALatch & 0xff00) | bt;
            break;

            // TA HI: timer A hibyte (set latch HI)
        case 0xdc05:
            _timerALatch = (_timerALatch & 0xff) | (bt << 8);
            break;

            // TB LO: timer B lobyte (set latch LO)
        case 0xdc06:
            _timerBLatch = (_timerBLatch & 0xff00) | bt;
            break;

            // TB HI: timer B hibyte (set latch HI)
        case 0xdc07:
            _timerBLatch = (_timerBLatch & 0xff) | (bt << 8);
            break;

            // ICR: interrupt control and status
        case 0xdc0d:
            if (IS_BIT_SET(bt, 7)) {
                // INT MASK is being set
                // bit 0: timer A underflow
                // bit 1: timer B underflow
                // TODO others....
                // bit 7: bits 0..4 are being set(1) or being clearing(0). if set, check
                // if irq is enabled
                _timerAIrqEnabled = IS_BIT_SET(bt, 0);
                _timerBIrqEnabled = IS_BIT_SET(bt, 1);
                // TODO: handle other irq sources for bit 2..4
            }
            _cpu->memory()->writeByte(addr, bt);
            break;

            // CRA: control timer A
        case 0xdc0e:
            if (IS_BIT_SET(bt, 0)) {
                _timerARunning = true;
            } else {
                _timerARunning = false;

                // reset hi latch
                _timerALatch &= 0xff;
            }

            if (IS_BIT_SET(bt, 4)) {
                // load latch into the timer
                _timerA = _timerALatch;
            }

            // set timer mode
            if (IS_BIT_SET(bt, 5)) {
                _timerAMode = CIA_TIMER_COUNT_POSITIVE_CNT_SLOPES;
            } else {
                _timerAMode = CIA_TIMER_COUNT_CPU_CYCLES;
            }
            _cpu->memory()->writeByte(addr, bt);
            break;

            // CRB: control timer B
        case 0xdc0f:
            if (IS_BIT_SET(bt, 0)) {
                _timerBRunning = true;
            } else {
                _timerBRunning = false;

                // reset hi latch
                _timerBLatch &= 0xff;
            }

            if (IS_BIT_SET(bt, 4)) {
                // load latch into the timer
                _timerB = _timerBLatch;
            }

            // set timer mode
            if (!IS_BIT_SET(bt, 5) && !IS_BIT_SET(bt, 6)) {
                // 00
                _timerBMode = CIA_TIMER_COUNT_CPU_CYCLES;
            } else if (!IS_BIT_SET(bt, 5) && IS_BIT_SET(bt, 6)) {
                // 01
                _timerBMode = CIA_TIMER_COUNT_POSITIVE_CNT_SLOPES;
            } else if (IS_BIT_SET(bt, 5) && !IS_BIT_SET(bt, 6)) {
                // 10
                _timerBMode = CIA_TIMER_COUNT_TIMERA_UNDERFLOW;
            } else if (IS_BIT_SET(bt, 5) && IS_BIT_SET(bt, 6)) {
                // 11
                _timerBMode = CIA_TIMER_COUNT_TIMERA_UNDERFLOW_IF_CNT_HI;
            }
            _cpu->memory()->writeByte(addr, bt);
            break;

        default:
            // write memory
            _cpu->memory()->writeByte(addr, bt);
    }
}

void CCIA1::setKeyState(uint8_t scancode, bool pressed) { _kbMatrix[scancode] = pressed; }

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
