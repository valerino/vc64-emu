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

CCIA2::CCIA2(CMOS65xx* cpu) {
    _cpu = cpu;
    _timerA = 0;
    _timerB = 0;
    _timerARunning = false;
    _timerBRunning = false;
}

CCIA2::~CCIA2() {

}

int CCIA2::update(int cycleCount) {
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

int CCIA2::getVicBank(uint16_t* address) {
    int bank = 0;
    uint8_t dataReg;
    _cpu->memory()->readByte(CIA2_REG_DATAPORT_A, &dataReg);
    if (IS_BIT_SET(dataReg, 1) && IS_BIT_SET(dataReg, 0)) {
        // xxxxxx10
        bank = 0;
        // TODO: this is a hack!!!!!!!! correct value here is 0
        *address = 0xc000;
    }
    else if (IS_BIT_SET(dataReg, 1) && !(IS_BIT_SET(dataReg, 0))) {
        // xxxxxx11
        bank = 1;
        *address = 0x4000;
    }
    else if (!(IS_BIT_SET(dataReg, 1)) && IS_BIT_SET(dataReg, 0)) {
        // xxxxxx01
        bank = 2;
        *address = 0x8000;
    }
    else if (!(IS_BIT_SET(dataReg, 1)) && !(IS_BIT_SET(dataReg, 0))) {
        // xxxxxx00
        bank = 3;
        *address = 0xc000;
    }

#ifdef DEBUG_CIA2
    CLog::printRaw("\tVIC-II bank %d selected!\n", bank);
#endif
    return bank;
}

void CCIA2::read(uint16_t address, uint8_t *bt) {
    // finally read
    _cpu->memory()->readByte(address,bt);
}

void CCIA2::write(uint16_t address, uint8_t bt) {
    switch (address) {
        case CIA2_REG_DATAPORT_A: {
            uint16_t base;
            int bank = getVicBank(&base);
            CMemory *ram = (CMemory *) _cpu->memory();
            if (bank == 0) {
                // shadow character rom at $1000
                memcpy(ram->raw() + 0x1000, ram->charset(), MEMORY_CHARSET_SIZE);
#ifdef DEBUG_CIA2
                CLog::printRaw("\tmirroring charset ROM in RAM at $1000\n");
#endif
            } else if (bank == 1) {
                // shadow character rom at $9000
                memcpy(ram->raw() + 0x9000, ram->charset(), MEMORY_CHARSET_SIZE);
#ifdef DEBUG_CIA2
                CLog::printRaw("\tmirroring charset ROM in RAM at $9000\n");
#endif
            }
        }
            break;

        case CIA2_REG_TIMER_A_LO:
            _timerA = (_timerA & 0xff00) | bt;
            break;

        case CIA2_REG_TIMER_A_HI:
            _timerA = (_timerA & 0xff) | ( bt << 8);
            break;

        case CIA2_REG_TIMER_B_LO:
            _timerB = (_timerB & 0xff00) | bt;
            break;

        case CIA2_REG_TIMER_B_HI:
            _timerB = (_timerB & 0xff) | ( bt << 8);
            break;

        case CIA2_REG_CONTROL_TIMER_A:
            if (IS_BIT_SET(bt, 0)) {
                _timerARunning = true;
            }
            else {
                _timerARunning = false;
            }
            break;

        case CIA2_REG_CONTROL_TIMER_B:
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
    _cpu->memory()->writeByte(address,bt);
}

