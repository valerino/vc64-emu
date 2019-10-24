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
        *bt = _timerMask;

        // clear bits 5,6 always
        // @todo handle bits 2,3,4
        BIT_CLEAR(_timerMask, 5);
        BIT_CLEAR(_timerMask, 6);

        // clear on read
        if (IS_BIT_SET(_timerMask, 0)) {
            BIT_CLEAR(_timerMask, 0);
        }
        if (IS_BIT_SET(_timerMask, 1)) {
            BIT_CLEAR(_timerMask, 1);
        }
        if (IS_BIT_SET(_timerMask, 2)) {
            BIT_CLEAR(_timerMask, 2);
        }
        if (IS_BIT_SET(_timerMask, 7)) {
            BIT_CLEAR(_timerMask, 7);
        }
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
        _timerALatch = (_timerALatch & 0xff) | ((int)bt << 8);
        if (!_timerARunning) {
            _timerA = _timerALatch;
        }
        break;

    case 0x06:
        // TB LO
        _timerBLatch = (_timerBLatch & 0xff00) | bt;
        break;

    case 0x07:
        // TB HI
        _timerBLatch = (_timerALatch & 0xff) | ((int)bt << 8);
        if (!_timerBRunning) {
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
        if (IS_BIT_SET(bt, 0)) {
            // timer a
            // signal irq through bit 7
            BIT_SET(_timerMask, 7);
        }
        if (IS_BIT_SET(bt, 1)) {
            // timer b
            // signal irq through bit 7
            BIT_SET(_timerMask, 7);
        }
        if (IS_BIT_SET(bt, 2)) {
            // TOD
            // signal irq through bit 7
            BIT_SET(_timerMask, 7);
        }

        _timerMask &= ~bt;
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
    //_cpu->memory()->writeByte(offset + _baseAddress, bt);
}

/**
 * @brief handle timer a/b underflow
 * @param timerType CIA_TIMER_A or CIA_TIMER_B
 */
void CCIABase::handleTimerUnderflow(int timerType) {
    bool running = false;
    int timer = 0;
    uint16_t latch = 0;
    uint8_t cr = 0;

    if (timerType == CIA_TIMER_A) {
        running = _timerARunning;
        timer = _timerA;
        latch = _timerALatch;
        cr = _crA;

        if (IS_BIT_SET(_crA, 2)) {
            // indicate underflow in port B bit 6
            BIT_TOGGLE(_prB, 6);
        } else {
            BIT_SET(_prB, 6);
        }
    } else {
        running = _timerBRunning;
        timer = _timerB;
        latch = _timerBLatch;
        cr = _crB;

        if (IS_BIT_SET(_crA, 2)) {
            // indicate underflow in port B bit 7
            BIT_TOGGLE(_prB, 7);
        } else {
            BIT_SET(_prB, 7);
        }
    }

    BIT_SET(_timerMask, timerType == CIA_TIMER_A ? 0 : 1);
    if (IS_BIT_SET(cr, 3)) {
        // stop timer after underflow
        BIT_CLEAR(cr, 0);
        running = false;
    } else {
        // timer restarts after underflow , reload latch
        running = true;
        timer = latch;
    }

    // set values back
    if (timerType == CIA_TIMER_A) {
        _timerARunning = running;
        _timerA = timer;
        _crA = cr;
    } else {
        _timerBRunning = running;
        _timerB = timer;
        _crB = cr;
    }
}

/**
 * @brief update timerA and timerB
 * @param cycleCount current cycle count
 * @param timerType CIA_TIMER_A or CIA_TIMER_B
 */
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
        // count elapsed cycles
        timer -= (cycleCount - _prevCycleCount);
        if (timerType == CIA_TIMER_A) {
            _timerA = timer;
        } else {
            _timerB = timer;
        }

        if (timer <= 0) {
            // signal underflow
            if (timerType == CIA_TIMER_A) {
                // timer b may count timer a underflows....
                _timerAUnderflows++;
                handleTimerUnderflow(CIA_TIMER_A);
            } else {
                handleTimerUnderflow(CIA_TIMER_B);
            }

            // trigger interrupt
            triggerInterrupt(timerType);
        }
    } else if (mode == CIA_TIMER_COUNT_TIMERA_UNDERFLOW &&
               timerType == CIA_TIMER_B) {

        // count timer a undersflows
        _timerB -= _timerAUnderflows;
        if (_timerB <= 0) {
            SDL_Log("timer b counts timer a underflow, triggered!");

            // signal underflow and trigger interrupt
            handleTimerUnderflow(CIA_TIMER_B);
            triggerInterrupt(CIA_TIMER_B);

            // reset timer a underflows
            _timerAUnderflows = 0;
        }
    }
    // TODO:other modes
}

int CCIABase::update(int cycleCount) {
    // update timers
    updateTimer(cycleCount, CIA_TIMER_A);
    updateTimer(cycleCount, CIA_TIMER_B);

    // update cycle count
    _prevCycleCount = cycleCount;
    return 0;
}

/**
 * @brief trigger nmi or irq, depending on cia (cia1=irq, cia2=nmi)
 * @param timerType CIA_TIMER_A or CIA_TIMER_B
 */
void CCIABase::triggerInterrupt(int timerType) {
    if (_connectedTo == CIA_TRIGGERS_IRQ) {
        // CIA1 triggers irq
        _cpu->irq();
    } else {
        // CIA2 triggers nmi
        // @todo: forget about it now .....
        // _cpu->nmi();
    }
}