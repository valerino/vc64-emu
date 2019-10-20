#include <stdio.h>
#include <bitutils.h>
#include <SDL.h>
#include "CCIABase.h"

CCIABase::CCIABase(CMOS65xx *cpu, CPLA *pla, uint16_t baseAddress,
                   int connectedTo) {
    _cpu = cpu;
    _pla = pla;
    _baseAddress = baseAddress;
    _connectedTo = connectedTo;
}

CCIABase::~CCIABase() {}

uint8_t CCIABase::readPRA() { return _prA; }
uint8_t CCIABase::readPRB() { return _prB; }

void CCIABase::read(uint16_t address, uint8_t *bt) {
    uint8_t tmp = 0;
    int offset = address - _baseAddress;
    switch (offset) {
    case 0:
        // PRA port A
        *bt = _prA;
        break;

    case 1:
        // PRB port B
        *bt = _prB;
        break;

    case 2:
        // DDRA data direction port A
        *bt = _ddrA;
        break;

    case 3:
        // DDRB data direction port B
        *bt = _ddrB;
        break;

    case 0x04:
        // TA LO
        *bt = (_timerA & 0xff);
        break;

    case 0x05:
        // TA HI
        *bt = (_timerA & 0xff00) >> 8;
        break;

    case 0x06:
        // TB LO
        *bt = (_timerB & 0xff);
        break;

    case 0x07:
        // TB HI
        *bt = (_timerB & 0xff00) >> 8;
        break;

    case 0x8:
        // TOD 10THS (RTC 1/10 seconds)
        // bit 4-7 is always 0
        tmp = _tod10Ths & 0xf;
        *bt = tmp;
        break;

    case 0x9:
        // TOD SEC (RTC seconds)
        // bit 7 is always 0
        tmp = _todSec;
        BIT_CLEAR(tmp, 7);
        *bt = tmp;
        break;

    case 0xa:
        // TOD MIN (RTC minutes)
        // bit 7 is always 0
        tmp = _todMin;
        BIT_CLEAR(tmp, 7);
        *bt = _todMin;
        break;

    case 0xb:
        // TOD HR (RTC hours)
        *bt = _todHr;
        break;

    case 0xc:
        // SDR (serial shift register)
        *bt = _todSdr;
        break;

    case 0x0d: {
        // ICR interrupt control
        // @todo handle bits 2,3,4

        // timer irqs (A=bit0, B=bit1)
        if (IS_BIT_SET(_timerMask, 0)) {
            // timer a
            // signal irq through bit 7
            BIT_SET(_timerMask, 7);
        }
        if (IS_BIT_SET(_timerMask, 1)) {
            // timer b
            // signal irq through bit 7
            BIT_SET(_timerMask, 7);
        }

        // clear bits 5,6 always
        BIT_CLEAR(_timerMask, 5);
        BIT_CLEAR(_timerMask, 6);

        *bt = _timerMask;

        // flags cleared on read
        _timerMask = 0;
        break;
    }

    case 0xe:
        // CRA (control timer A)
        *bt = _crA;
        break;

    case 0xf:
        // CRB (control timer B)
        *bt = _crB;
        break;

    default:
        // read from memory
        SDL_Log("unhandled cia%d read at $%0x",
                _connectedTo == CIA_TRIGGERS_IRQ ? 1 : 2, address);
        break;
    }
}

void CCIABase::write(uint16_t address, uint8_t bt) {
    int offset = address - _baseAddress;
    switch (offset) {
    case 0:
        // PRA port A
        _prA = bt;
        break;

    case 1:
        // PRB port B
        _prB = bt;
        break;

    case 2:
        // DDRA data direction port A
        _ddrA = bt;
        break;

    case 3:
        // DDRB data direction port B
        _ddrB = bt;
        break;

    case 0x04:
        // TA LO
        _timerALatch = (_timerALatch & 0xff00) | bt;
        break;

    case 0x05:
        // TA HI
        if (_timerARunning) {
            _timerALatch = (_timerALatch & 0xff) | ((int)bt << 8);
        } else {
            _timerALatch = (_timerALatch & 0xff) | ((int)bt << 8);
            _timerA = _timerALatch;
        }
        break;

    case 0x06:
        // TB LO
        _timerBLatch = (_timerBLatch & 0xff00) | bt;
        break;

    case 0x07:
        // TB HI
        if (_timerBRunning) {
            _timerBLatch = (_timerBLatch & 0xff) | ((int)bt << 8);
        } else {
            _timerBLatch = (_timerBLatch & 0xff) | ((int)bt << 8);
            _timerB = _timerBLatch;
        }
        break;

    case 0x8:
        // TOD 10THS (RTC 1/10 seconds)
        _tod10Ths = bt;
        break;

    case 0x9:
        // TOD SEC (RTC seconds)
        _todSec = bt;
        break;

    case 0xa:
        // TOD MIN (RTC minutes)
        _todMin = bt;
        break;

    case 0xb:
        // TOD HR (RTC hours)
        _todHr = bt;
        break;

    case 0xc:
        // SDR (serial shift register)
        _todSdr = bt;
        break;

    case 0x0d: {
        // ICR interrupt control
        bool sSet = IS_BIT_SET(bt, 7);
        if (IS_BIT_SET(bt, 0)) {
            if (sSet) {
                BIT_SET(_timerMask, 0);
            } else {
                BIT_CLEAR(_timerMask, 0);
            }
        }

        if (IS_BIT_SET(bt, 1)) {
            if (sSet) {
                BIT_SET(_timerMask, 1);
            } else {
                BIT_CLEAR(_timerMask, 1);
            }
        }

        if (IS_BIT_SET(bt, 2)) {
            if (sSet) {
                BIT_SET(_timerMask, 2);
            } else {
                BIT_CLEAR(_timerMask, 3);
            }
        }

        if (IS_BIT_SET(bt, 3)) {
            if (sSet) {
                BIT_SET(_timerMask, 3);
            } else {
                BIT_CLEAR(_timerMask, 3);
            }
        }

        if (IS_BIT_SET(bt, 4)) {
            if (sSet) {
                BIT_SET(_timerMask, 4);
            } else {
                BIT_CLEAR(_timerMask, 4);
            }
        }

        //_timerMask = bt;
        break;
    }
    case 0x0e:
        // CRA
        // @todo handle other bits
        if (IS_BIT_SET(bt, 0)) {
            // timer started
            _timerARunning = true;
        } else {
            // stop timer
            _timerARunning = false;
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
        _crA = bt;
        break;

    case 0x0f:
        // CRB
        // @todo handle other bits
        if (IS_BIT_SET(bt, 0)) {
            // timer started
            _timerBRunning = true;
        } else {
            // stop timer
            _timerBRunning = false;
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

        _crB = bt;
        break;

    default:
        SDL_Log("unhandled cia%d write at $%0x",
                _connectedTo == CIA_TRIGGERS_IRQ ? 1 : 2, address);
        break;
    }

    // write to memory anyway
    // @fixme this is wrong
    //_cpu->memory()->writeByte(address, bt, true);
}

void CCIABase::updateTimer(int cycleCount, int timerType) {
    // check which timer we're updating
    bool running = false;
    int mode = 0;
    int timer = 0;
    if (timerType == CIA_TIMER_A) {
        running = _timerARunning;
        mode = _timerAMode;
        timer = _timerA;
    } else {
        running = _timerBRunning;
        mode = _timerBMode;
        timer = _timerB;
    }

    // check if the timer is running
    if (!running) {
        return;
    }

    // check timer mode
    if (mode == CIA_TIMER_COUNT_CPU_CYCLES) {
        timer -= (cycleCount - _prevCycleCount);
        if (timerType == CIA_TIMER_A) {
            _timerA = timer;
        } else {
            _timerB = timer;
        }

        if (timer <= 0) {
            // signal underflow in the mask (0=timer A, 1=timer B)
            if (timerType == CIA_TIMER_A) {
                _timerAUnderflows++;
                BIT_SET(_timerMask, 0);
                if (IS_BIT_SET(_crA, 3)) {
                    // stop timer after underflow
                    BIT_CLEAR(_crA, 0);
                    _timerARunning = false;
                } else {
                    // timer restarts after underflow , reload latch
                    _timerARunning = true;
                    _timerA = _timerALatch;
                }
            } else {
                BIT_SET(_timerMask, 1);
                if (IS_BIT_SET(_crB, 3)) {
                    // stop timer after underflow
                    BIT_CLEAR(_crB, 0);
                    _timerBRunning = false;
                } else {
                    // timer restarts after underflow , reload latch
                    _timerBRunning = true;
                    _timerB = _timerBLatch;
                }
            }

            // set bit7, interrupt occurred
            BIT_SET(_timerMask, 7);
            if (_connectedTo == CIA_TRIGGERS_IRQ) {
                // CIA1 triggers irq
                _cpu->irq();
            } else {
            }
        }
    } else if (mode == CIA_TIMER_COUNT_TIMERA_UNDERFLOW &&
               timerType == CIA_TIMER_B) {
        return;
        _timerB -= _timerAUnderflows;
        SDL_Log("timer b currently %d", _timerB);
        if (_timerB <= 0) {
            SDL_Log("timer b counts timer a underflow");
            BIT_SET(_timerMask, 1);
            if (IS_BIT_SET(_crB, 3)) {
                // stop timer after underflow
                BIT_CLEAR(_crB, 0);
                _timerBRunning = false;
            } else {
                // timer restarts after underflow , reload latch
                _timerBRunning = true;
                _timerB = _timerBLatch;
            }

            // set bit7, interrupt occurred
            BIT_SET(_timerMask, 7);
            if (_connectedTo == CIA_TRIGGERS_IRQ) {
                // CIA1 triggers irq
                _cpu->irq();
            } else {
                //_cpu->nmi();
            }
            _timerAUnderflows = 0;
        }
    }
    // TODO: mode is count slopes at CNT pin
}

int CCIABase::update(int cycleCount) {
    // update timers
    updateTimer(cycleCount, CIA_TIMER_A);
    updateTimer(cycleCount, CIA_TIMER_B);

    // @todo: handle cia2

    // update cycle count
    _prevCycleCount = cycleCount;
    return 0;
}
