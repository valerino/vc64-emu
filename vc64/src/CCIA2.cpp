//
// Created by valerino on 19/01/19.
//

#include "CCIA2.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bitutils.h>
#include <CLog.h>
#include "CMemory.h"

#ifndef NDEBUG
// debug-only flag
//#define DEBUG_CIA2
#endif

CCIA2::CCIA2(CMOS65xx *cpu) {
    _cpu = cpu;
    _timerA = 0;
    _timerB = 0;
    _timerARunning = false;
    _timerBRunning = false;
}

CCIA2::~CCIA2() {}

int CCIA2::update(int cycleCount) {
    /*int cc = cycleCount - _prevCycleCount;
    if (cc < 16421) {
        return 0;
    }*/
    if (_timerARunning) {
        // check timerA mode
        if (_timerAMode == CIA_TIMER_COUNT_CPU_CYCLES) {
            _timerA = _timerA - (cycleCount - _prevCycleCount);
            if (_timerA <= 0) {
                if (_timerANmiEnabled) {
                    // trigger nmi
                    _cpu->nmi();
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
                    if (_timerBNmiEnabled) {
                        // trigger nmi
                        _cpu->nmi();
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

void CCIA2::read(uint16_t address, uint8_t *bt) {
    switch (address) {
        // TA LO: timer A lowbyte
        case 0xdd04:
            *bt = _timerA & 0xff;
            break;

            // TA HI: timer A hibyte
        case 0xdd05:
            *bt = (_timerA & 0xff00) >> 8;
            break;

            // TB LO: timer B lobyte
        case 0xdd06:
            *bt = _timerB & 0xff;
            break;

            // TB HI: timer B hibyte
        case 0xdd07:
            *bt = (_timerB & 0xff00) >> 8;
            break;

        default:
            // read memory
            _cpu->memory()->readByte(address, bt);
            break;
    }
}

int CCIA2::getVicBank(uint16_t *address) {
    int bank = 0;
    uint8_t dataReg;
    _cpu->memory()->readByte(0xdd00, &dataReg);
    if (!IS_BIT_SET(dataReg, 0) && !IS_BIT_SET(dataReg, 1)) {
        // xxxxxx00
        bank = 3;
        *address = 0xc000;
    } else if (!IS_BIT_SET(dataReg, 0) && IS_BIT_SET(dataReg, 1)) {
        // xxxxxx01
        bank = 2;
        *address = 0x8000;
    } else if (IS_BIT_SET(dataReg, 0) && !IS_BIT_SET(dataReg, 1)) {
        // xxxxxx10
        bank = 1;
        *address = 0x4000;
    } else if (IS_BIT_SET(dataReg, 0) && IS_BIT_SET(dataReg, 1)) {
        // xxxxxx11
        bank = 0;
        *address = 0x0;
    }

#ifdef DEBUG_CIA2
    CLog::printRaw("\tVIC-II bank %d selected!\n", bank);
#endif
    return bank;
}

void CCIA2::write(uint16_t address, uint8_t bt) {
    switch (address) {
            // TA LO: timer A lowbyte (set latch LO)
        case 0xdd04:
            _timerALatch = (_timerALatch & 0xff00) | bt;
            break;

            // TA HI: timer A hibyte (set latch HI)
        case 0xdd05:
            _timerALatch = (_timerALatch & 0xff) | (bt << 8);
            break;

            // TB LO: timer B lobyte (set latch LO)
        case 0xdd06:
            _timerBLatch = (_timerBLatch & 0xff00) | bt;
            break;

            // TB HI: timer B hibyte (set latch HI)
        case 0xdd07:
            _timerBLatch = (_timerBLatch & 0xff) | (bt << 8);
            break;

            // ICR: interrupt control and status
        case 0xdd0d:
            if (IS_BIT_SET(bt, 7)) {
                // INT MASK is being set
                // bit 0: timer A underflow
                // bit 1: timer B underflow
                // TODO others....
                // bit 7: bits 0..4 are being set(1) or being clearing(0). if set, check if irq is enabled
                _timerANmiEnabled = IS_BIT_SET(bt, 0);
                _timerBNmiEnabled = IS_BIT_SET(bt, 1);
            }
            _cpu->memory()->writeByte(address, bt);
            break;

            // CRA: control timer A
        case 0xdd0e:
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
            _cpu->memory()->writeByte(address, bt);
            break;

            // CRB: control timer B
        case 0xdd0f:
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
            _cpu->memory()->writeByte(address, bt);
            break;

        default:
            // write memory
            _cpu->memory()->writeByte(address, bt);
    }
}
